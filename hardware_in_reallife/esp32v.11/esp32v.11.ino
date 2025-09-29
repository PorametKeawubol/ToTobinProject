#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

/* =======================
 *  เซตฮาร์ดแวร์ / พิน
 * =======================*/
Servo cupServo;            // เซอร์โวถาดหลัก G13 (continuous-rotation)
Servo prepServo1;          // เซอร์โวเตรียมแก้ว G23 (มาตรฐานองศา)
Servo prepServo2;          // เซอร์โวเตรียมแก้ว G22 (มาตรฐานองศา)
Servo toppingServo;        // เซอร์โว topping G15 (มาตรฐานองศา)

const int SERVO_PIN   = 13;  // ถาด
const int PREP_PIN_1  = 23;  // เตรียมแก้ว #1
const int PREP_PIN_2  = 22;  // เตรียมแก้ว #2
const int TOPPING_SERVO_PIN = 15; // topping (S3)

// เซอร์โวถาด (continuous rotation)
const int SERVO_US_CCW  = 1700;   // ทวนเข็ม (>1500)
const int SERVO_US_STOP = 1500;   // หยุด
const int SERVO_US_CW   = 2000;   // ตามเข็ม

inline void servoStop() { cupServo.writeMicroseconds(SERVO_US_STOP); }
inline void servoCCW()  { cupServo.writeMicroseconds(SERVO_US_CCW); }
inline void servoCW()   { cupServo.writeMicroseconds(SERVO_US_CW);  }

/* =======================
 *  SENSORS
 * =======================*/
// S1 = TRIG G16, ECHO G17   |  S2 = TRIG G19, ECHO G18
const int ULTRA1_TRIG = 16;
const int ULTRA1_ECHO = 17;
const int ULTRA2_TRIG = 19;
const int ULTRA2_ECHO = 18;

const float SOUND_CM_PER_US = 0.0343f;
const unsigned long ULTRA_TIMEOUT_US = 30000UL;

// เกณฑ์
const float ULTRA1_THRESHOLD_CM = 11.5f;  // ใช้ใน whichSensor()
const float ULTRA2_THRESHOLD_CM = 10.5f;

// เริ่มงานถ้าหน้า S1 ใกล้มากตั้งแต่แรก
const float START_US1_SKIP_CM = 30.0f;

// ===== Preparing-cup (S2) rules =====
const float PREP_NEAR_CM = 5.0f;            // ต้องใกล้กว่า 4 ซม.
const unsigned long PREP_CONFIRM_MS = 2000; // ค้างยืนยัน 2 วิ

// ===== โมชันเซอร์โวเตรียมแก้ว =====
int prep_pos = 0;
const uint8_t prep_stepSize   = 10;
const unsigned long prep_stepDelayMs = 20;
int shakeAmplitude = 80;   // ±40°
int shakeDelay    = 150;
int shakeTimes    = 5;
const int PREP_HOME_DEG = 0;

// ===== topping (S3) =====
const int TOPPING_MIN_DEG   = 10;
const int TOPPING_MAX_DEG   = 55;
const unsigned long TOPPING_PULSE_MS = 300;

/* =======================
 *  Wi-Fi / API config
 * =======================*/
const char* ssid        = "totoiot";
const char* password    = "123456789";
const char* BASE_URL    = "https://porametix.online";
const char* HARDWARE_ID = "esp32-001";
const char* API_KEY     = "odroid-hardware-key-1758367749";

/* =======================
 *  Timing
 * =======================*/
const unsigned long POLL_IDLE_MS   = 2000;
const unsigned long POLL_ERROR_MS  = 5000;
const unsigned long LOG_EVERY_MS   = 250;

// ป้องกัน “หมุนไม่หยุด” ระหว่างหา marker
const unsigned long FIND_TIMEOUT_MS = 30000;  // 30 วินาที

/* =======================
 *  HTTP helpers
 * =======================*/
WiFiClientSecure secureClient;
HTTPClient http;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("WiFi connecting");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
    if (millis() - t0 > 30000) {
      Serial.println("\nWiFi connect timeout, retrying...");
      WiFi.disconnect(true);
      delay(2000);
      WiFi.begin(ssid, password);
      t0 = millis();
    }
  }
  Serial.printf("\nWiFi connected: %s  IP: %s\n",
                WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

bool httpBeginWithCommon(const String& url) {
  secureClient.setInsecure();
  http.setTimeout(15000);
  return http.begin(secureClient, url);
}

bool postJson(const String& url, const String& json,
              const char* extraHeaderKey = nullptr, const char* extraHeaderVal = nullptr,
              int* outStatus = nullptr, String* outResp = nullptr) {
  if (!httpBeginWithCommon(url)) {
    Serial.println("[sim] http.begin failed (post)");
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  if (extraHeaderKey && extraHeaderVal) http.addHeader(extraHeaderKey, extraHeaderVal);

  Serial.printf("[sim] POST %s\n[sim] body: %s\n", url.c_str(), json.c_str());
  int code = http.POST((uint8_t*)json.c_str(), json.length());
  if (outStatus) *outStatus = code;

  String res = http.getString();
  if (outResp) *outResp = res;
  http.end();

  Serial.printf("[sim] HTTP %d, resp: %s\n", code, res.substring(0, 200).c_str());
  if (code <= 0) return false;
  return (code >= 200 && code < 300);
}

bool pollNextOrder(String& orderId, int& queuePos) {
  String url = String(BASE_URL) + "/api/hardware/orders?hardwareId=" + HARDWARE_ID;

  if (!httpBeginWithCommon(url)) {
    Serial.println("[sim] http.begin failed (orders)");
    return false;
  }
  http.addHeader("X-API-Key", API_KEY);

  int code = http.GET();
  if (code <= 0) {
    Serial.printf("[sim] Poll orders failed: %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    return false;
  }
  Serial.printf("[sim] pollNextOrder HTTP %d\n", code);
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  String payload = http.getString();
  http.end();
  Serial.printf("[sim] poll payload: %s\n", payload.substring(0, 200).c_str());

  DynamicJsonDocument doc(2048);
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[sim] JSON parse error (orders): %s\n", err.c_str());
    return false;
  }

  if (!doc["order"].isNull() && doc["order"]["orderId"].is<const char*>()) {
    orderId  = String(doc["order"]["orderId"].as<const char*>());
    queuePos = doc["order"]["queuePosition"] | -1;
    return true;
  }
  orderId = "";
  queuePos = -1;
  return true;
}

/* =======================
 *  Ultrasonic helpers
 * =======================*/
float readUltrasonicCm(int trigPin, int echoPin) {
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long dur = pulseIn(echoPin, HIGH, ULTRA_TIMEOUT_US);
  if (dur == 0) return NAN;
  float cm = (dur * SOUND_CM_PER_US) / 2.0f;
  return cm;
}

float readUltrasonicAverageCm(int trigPin, int echoPin, int samples = 5, int gapMs = 40) {
  float sum = 0.0f;
  int cnt = 0;
  for (int i = 0; i < samples; ++i) {
    float v = readUltrasonicCm(trigPin, echoPin);
    if (!isnan(v)) { sum += v; cnt++; }
    delay(gapMs);
    yield();
  }
  if (cnt == 0) return NAN;
  return sum / cnt;
}

bool ultrasonicTriggered(int trigPin, int echoPin, float thresholdCm) {
  float cm = readUltrasonicCm(trigPin, echoPin);
  if (isnan(cm)) return false;
  return (cm <= thresholdCm);
}

/* --- TCRT (S3,S4) --- */
const int  TCRT_S3 = 27;   // G27
const int  TCRT_S4 = 26;   // G26
const bool TCRT_ACTIVE_LOW = true;
const unsigned long TCRT_DEBOUNCE_MS = 30;

inline bool tcrtRaw(int pin) {
  int v = digitalRead(pin);
  return TCRT_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

/* Which sensor hit? */
int whichSensor() {
  if (ultrasonicTriggered(ULTRA1_TRIG, ULTRA1_ECHO, ULTRA1_THRESHOLD_CM)) return 1;
  if (ultrasonicTriggered(ULTRA2_TRIG, ULTRA2_ECHO, ULTRA2_THRESHOLD_CM)) return 2;
  if (tcrtRaw(TCRT_S3)) return 3;
  if (tcrtRaw(TCRT_S4)) return 4;
  return 0;
}

/* =======================
 *  Progress API
 * =======================*/
bool postProgressExtended(const String& orderId,
                          const String& step,
                          const String& status,
                          const String& message,
                          bool tcrtHit,
                          const char* eventTag,
                          int sensorIndex) {
  { // main
    DynamicJsonDocument doc(2048);
    doc["orderId"]     = orderId;
    doc["hardwareId"]  = HARDWARE_ID;
    doc["step"]        = step;
    doc["status"]      = status;
    doc["message"]     = message;
    doc["tcrtHit"]     = tcrtHit;
    if (eventTag) doc["event"] = eventTag;
    if (sensorIndex > 0) doc["sensor"] = sensorIndex;

    String body; serializeJson(doc, body);
    String url = String(BASE_URL) + "/api/hardware/progress";
    int httpStatus = 0; String resp;
    bool ok = postJson(url, body, nullptr, nullptr, &httpStatus, &resp);
    if (ok) return true;
    Serial.printf("[sim] /progress failed (HTTP %d) → fallback /status\n", httpStatus);
  }

  { // fallback
    DynamicJsonDocument doc(2048);
    doc["orderId"]     = orderId;
    doc["status"]      = (step == "completed") ? "completed" : "brewing";
    doc["step"]        = step;
    doc["message"]     = message;
    doc["hardwareId"]  = HARDWARE_ID;
    doc["tcrtHit"]     = tcrtHit;
    if (eventTag) doc["event"] = eventTag;
    if (sensorIndex > 0) doc["sensor"] = sensorIndex;

    JsonObject led = doc.createNestedObject("ledState");
    led["preparing"] = (step == "preparing_cup");
    led["toppings"]  = (step == "adding_toppings");
    led["ice"]       = (step == "adding_ice");
    led["brewing"]   = (step == "brewing_drink");
    led["completed"] = (step == "completed");
    doc["progress"]  = 0;

    String body; serializeJson(doc, body);
    String url = String(BASE_URL) + "/api/hardware/status";
    int httpStatus = 0; String resp;
    bool ok = postJson(url, body, "X-API-Key", API_KEY, &httpStatus, &resp);
    if (!ok) Serial.printf("[sim] Fallback /status failed (HTTP %d)\n", httpStatus);
    return ok;
  }
}

/* =======================
 *  Helper: รอเซนเซอร์เป้าหมาย + เพิ่ม timeout
 * =======================*/
bool waitForSensorHitSequence(const String& orderId,
                              int targetSensor,
                              unsigned long stopMs,
                              const char* reportStep,
                              bool skipOnly) {
  Serial.printf("[seq] Waiting for S%d ...\n", targetSensor);
  servoCCW();
  unsigned long tLastLog = 0;
  unsigned long tStart = millis();

  while (true) {
    // timeout กันหมุนยาว
    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.printf("[seq] Timeout waiting S%d → STOP\n", targetSensor);
      servoStop();
      return false;
    }

    int w = whichSensor();
    if (w == targetSensor) {
      unsigned long pressedAt = millis();
      while ( (whichSensor() == targetSensor) && (millis() - pressedAt < TCRT_DEBOUNCE_MS) ) {
        delay(1);
      }
      if (millis() - pressedAt >= TCRT_DEBOUNCE_MS) {
        if (skipOnly) {
          Serial.printf("[seq] S%d hit → SKIP (keep spinning)\n", targetSensor);
          return true;
        } else {
          Serial.printf("[seq] S%d hit → STOP %lums + report step '%s'\n", targetSensor, stopMs, reportStep);
          servoStop();
          (void)postProgressExtended(orderId, reportStep, "in_progress",
                                     String(reportStep) + " (sensor S" + targetSensor + ")",
                                     true, "marker_hit", targetSensor);

          // ถ้าเป็น S3 → topping servo pulse 10°→55°(0.3s)→10°
          if (targetSensor == 3 && reportStep && String(reportStep) == "adding_toppings") {
            toppingServo.write(TOPPING_MAX_DEG);
            delay(TOPPING_PULSE_MS);
            toppingServo.write(TOPPING_MIN_DEG);
          }

          delay(stopMs);
          servoCCW();
          return true;
        }
      }
    }

    if (millis() - tLastLog >= LOG_EVERY_MS) {
      tLastLog = millis();
      float cm1 = readUltrasonicCm(ULTRA1_TRIG, ULTRA1_ECHO);
      float cm2 = readUltrasonicCm(ULTRA2_TRIG, ULTRA2_ECHO);
      Serial.printf("[dbg] spin... US1=%s cm  US2=%s cm  S3=%d S4=%d\n",
                    isnan(cm1) ? "NaN" : String(cm1, 1).c_str(),
                    isnan(cm2) ? "NaN" : String(cm2, 1).c_str(),
                    tcrtRaw(TCRT_S3)?1:0, tcrtRaw(TCRT_S4)?1:0);
    }
    delay(1);
    yield();
  }
}

/* =======================
 *  S2 (preparing_cup)
 * =======================*/
void prepServosWriteBoth(int deg) {
  prepServo1.write(deg);
  prepServo2.write(deg);
}

void prepServosMoveTo(int targetDeg, int step = 10, unsigned long stepDelay = 20) {
  if (targetDeg < 0) targetDeg = 0;
  if (targetDeg > 180) targetDeg = 180;
  while (prep_pos != targetDeg) {
    if (prep_pos < targetDeg) prep_pos += step;
    else                      prep_pos -= step;
    if (prep_pos < 0)   prep_pos = 0;
    if (prep_pos > 180) prep_pos = 180;
    prepServosWriteBoth(prep_pos);
    delay(stepDelay);
    yield();
  }
}

bool gotoAndPrepareAtS2(const String& orderId) {
  Serial.println("[prep] Seeking S2 ...");
  servoCCW();
  unsigned long tLastLog = 0;
  unsigned long tStart = millis();

  // หมุนหา S2 (มี timeout กันหมุนยาว)
  while (true) {
    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.println("[prep] Timeout seeking S2 → STOP");
      servoStop();
      return false;
    }

    int w = whichSensor();
    if (w == 2) {
      unsigned long hitAt = millis();
      while ((whichSensor() == 2) && (millis() - hitAt < TCRT_DEBOUNCE_MS)) { delay(1); }
      if (millis() - hitAt >= TCRT_DEBOUNCE_MS) {
        servoStop();
        (void)postProgressExtended(orderId, "preparing_cup", "in_progress",
                                   "หยุดเพื่อเตรียมแก้ว (S2)", true, "marker_hit", 2);
        break;
      }
    }

    if (millis() - tLastLog >= LOG_EVERY_MS) {
      tLastLog = millis();
      float cm1 = readUltrasonicCm(ULTRA1_TRIG, ULTRA1_ECHO);
      float cm2 = readUltrasonicCm(ULTRA2_TRIG, ULTRA2_ECHO);
      Serial.printf("[dbg] spin→S2... US1=%s cm  US2=%s cm\n",
                    isnan(cm1) ? "NaN" : String(cm1,1).c_str(),
                    isnan(cm2) ? "NaN" : String(cm2,1).c_str());
    }
    delay(1);
    yield();
  }

  // === ควบคุม G23/G22 ระหว่างรอแก้วเข้าใกล้ ===
  prep_pos = 0;
  prepServosWriteBoth(prep_pos);

  Serial.printf("[prep] Waiting ULTRA2 < %.1f cm then hold %lu ms (servos active)\n", PREP_NEAR_CM, PREP_CONFIRM_MS);

  while (true) {
    float nowAvg = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);

    // ยังไม่มีแก้ว → ค่อย ๆ ไป 180° แล้วเขย่า
    if (isnan(nowAvg) || nowAvg >= PREP_NEAR_CM) {
      if (prep_pos < 180) {
        prep_pos += prep_stepSize;
        if (prep_pos > 180) prep_pos = 180;
        prepServosWriteBoth(prep_pos);
        delay(prep_stepDelayMs);
      } else {
        int halfAmp = shakeAmplitude / 2;
        for (int i = 0; i < shakeTimes; i++) {
          prepServosWriteBoth(prep_pos - halfAmp);
          delay(shakeDelay);
          prepServosWriteBoth(prep_pos + halfAmp);
          delay(shakeDelay);
        }
        prepServosWriteBoth(prep_pos);
      }

      static unsigned long lastLog = 0;
      if (millis() - lastLog >= 300) {
        lastLog = millis();
        Serial.printf("[prep] ULTRA2= %s cm (need < %.1f)  pos=%d\n",
                      isnan(nowAvg) ? "NaN" : String(nowAvg,1).c_str(),
                      PREP_NEAR_CM, prep_pos);
      }
    } else {
      // มีแก้ว → คอนเฟิร์ม 2 วิ
      Serial.printf("[prep] near %.1f cm detected → confirming...\n", nowAvg);
      delay(PREP_CONFIRM_MS);
      float confirm = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);
      if (!isnan(confirm) && confirm < PREP_NEAR_CM) {
        Serial.println("[prep] confirmed cup present → homing prep servos...");
        prepServosMoveTo(PREP_HOME_DEG, 10, 20);
        Serial.println("[prep] homed → continue to next step");
        servoCCW();
        return true;
      } else {
        Serial.println("[prep] confirm failed, keep working...");
      }
    }

    delay(10);
    yield();
  }
}

/* =======================
 *  หลัง brewing_drink: รอให้ ULTRA1 เพิ่ม ≥ 4 ซม. → completed
 * =======================*/
void waitForPickupAfterBrew(const String& orderId, float pickupDeltaCm = 4.0f) {
  servoStop();
  Serial.println("[pickup] Measuring baseline on ULTRA1...");
  float baseline = NAN;
  for (int attempt = 0; attempt < 3 && isnan(baseline); ++attempt) {
    baseline = readUltrasonicAverageCm(ULTRA1_TRIG, ULTRA1_ECHO, 8, 35);
  }
  if (isnan(baseline)) baseline = readUltrasonicCm(ULTRA1_TRIG, ULTRA1_ECHO);
  Serial.printf("[pickup] baseline= %s cm\n", isnan(baseline) ? "NaN" : String(baseline, 1).c_str());

  unsigned long tLastLog = 0;
  while (true) {
    float nowAvg = readUltrasonicAverageCm(ULTRA1_TRIG, ULTRA1_ECHO, 3, 30);
    if (isnan(baseline) && !isnan(nowAvg)) {
      baseline = nowAvg;
      Serial.printf("[pickup] baseline recovered = %.1f cm\n", baseline);
    }
    bool picked = (!isnan(baseline) && !isnan(nowAvg)) ? ((nowAvg - baseline) >= pickupDeltaCm) : false;

    if (millis() - tLastLog >= 300) {
      tLastLog = millis();
      Serial.printf("[pickup] check... base= %s cm, now= %s cm, Δ= %s cm, need ≥ %.1f\n",
                    isnan(baseline) ? "NaN" : String(baseline, 1).c_str(),
                    isnan(nowAvg) ? "NaN" : String(nowAvg, 1).c_str(),
                    (isnan(baseline) || isnan(nowAvg)) ? "NaN" : String(nowAvg - baseline, 1).c_str(),
                    pickupDeltaCm);
    }

    if (picked) {
      (void)postProgressExtended(orderId, "completed", "completed",
                                 "ลูกค้าหยิบแก้วแล้ว (ULTRA1 เพิ่ม ≥ 4cm)", false, "cup_taken", 1);
      Serial.println("[pickup] Cup taken → COMPLETED");
      servoStop();
      return;
    }

    delay(80);
    yield();
  }
}

/* =======================
 *  Brew sequence
 * =======================*/
void brew(const String& orderId) {
  const unsigned long HOLD_MS = 2000;

  // เริ่ม
  (void)postProgressExtended(orderId, "start", "in_progress", "เริ่มต้น", false, "step_started", 0);

  // --- NEW: เช็ค ULTRA1 ตอนเริ่ม ถ้าใกล้มาก (<30cm) ให้ "ข้าม S1" ทันที ---
  float us1Start = readUltrasonicAverageCm(ULTRA1_TRIG, ULTRA1_ECHO, 3, 25);
  Serial.printf("[start] US1 start = %s cm\n", isnan(us1Start) ? "NaN" : String(us1Start,1).c_str());

  if (!isnan(us1Start) && us1Start < START_US1_SKIP_CM) {
    Serial.println("[start] US1 < 30cm → skip S1 and go preparing_cup (S2)");
    // ไปหา S2 เลย (ฟังก์ชันนี้จะสั่งหมุนเอง)
    if (!gotoAndPrepareAtS2(orderId)) {
      // ถ้าหา S2 ไม่เจอในเวลา → ป้องกันค้าง
      Serial.println("[start] Could not reach S2 in time, aborting this cycle.");
      servoStop();
      return;
    }
  } else {
    // ปกติ: หมุนและรอชน S1 (skip) แต่มี timeout
    if (!waitForSensorHitSequence(orderId, 1, 0, "", true)) {
      Serial.println("[start] S1 wait timeout → continue to S2 anyway");
    }
    // จากนั้นเข้าสเต็ป S2 ตามปกติ
    if (!gotoAndPrepareAtS2(orderId)) {
      Serial.println("[start] Could not reach S2 in time, aborting this cycle.");
      servoStop();
      return;
    }
  }

  // S3 → หยุด + adding_toppings (สั่ง G15 10°→55°(0.3s)→10° ภายใน waitForSensorHitSequence)
  waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);

  // S4 → หยุด + adding_ice
  waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);

  // S1 → หยุด + brewing_drink
  waitForSensorHitSequence(orderId, 1, HOLD_MS, "brewing_drink", false);

  // รอให้ลูกค้ายกแก้ว (ULTRA1 เพิ่ม ≥ 4 ซม.) → completed
  waitForPickupAfterBrew(orderId, 4.0f);

  // เสร็จแล้วหยุด
  servoStop();
  Serial.println("[seq] Sequence completed → STOP");
}

/* =======================
 *  Setup / Loop
 * =======================*/
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf("[sim] Starting hardware sim %s @ %s\n", HARDWARE_ID, BASE_URL);

  // Ultrasonic pins
  pinMode(ULTRA1_TRIG, OUTPUT);
  pinMode(ULTRA1_ECHO, INPUT);
  pinMode(ULTRA2_TRIG, OUTPUT);
  pinMode(ULTRA2_ECHO, INPUT);
  digitalWrite(ULTRA1_TRIG, LOW);
  digitalWrite(ULTRA2_TRIG, LOW);

  // TCRT pins
  pinMode(TCRT_S3, INPUT_PULLUP);
  pinMode(TCRT_S4, INPUT_PULLUP);

  // --- สำคัญ: ตั้งความถี่/ช่วงพัลส์ + หยุดเซอร์โวทันทีหลัง attach เพื่อลดการหมุนตอนรีเซต ---
  cupServo.setPeriodHertz(50);
  cupServo.attach(SERVO_PIN, 500, 2500);
  servoStop(); // หยุดไว้ก่อน

  prepServo1.attach(PREP_PIN_1, 500, 2500);
  prepServo2.attach(PREP_PIN_2, 500, 2500);

  toppingServo.setPeriodHertz(50);
  toppingServo.attach(TOPPING_SERVO_PIN, 500, 2500);
  toppingServo.write(TOPPING_MIN_DEG);

  // ตำแหน่งเริ่มต้นของ G23/G22
  prep_pos = 0;
  prepServosWriteBoth(0);

  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[sim] WiFi dropped, reconnecting...");
    servoStop();
    connectWiFi();
  }

  String orderId; int queuePos = -1;
  bool ok = pollNextOrder(orderId, queuePos);
  if (!ok) {
    Serial.println("[sim] Error polling orders, retry soon...");
    servoStop();
    delay(POLL_ERROR_MS);
    return;
  }

  if (orderId.length() > 0) {
    Serial.printf("[sim] Received order %s (#%d)\n", orderId.c_str(), queuePos);
    brew(orderId);
    Serial.printf("[sim] Completed order %s\n", orderId.c_str());
    servoStop();
  } else {
    servoStop();
    delay(POLL_IDLE_MS);
  }
}
