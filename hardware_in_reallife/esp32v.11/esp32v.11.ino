#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

/* =======================
 *  เซตฮาร์ดแวร์ / พิน
 * =======================*/
Servo cupServo;            // เซอร์โวถาดหลัก G13
Servo prepServo1;          // เซอร์โวเตรียมแก้ว G23
Servo prepServo2;          // เซอร์โวเตรียมแก้ว G22

const int SERVO_PIN   = 13;  // PWM servo (ถาด)
const int PREP_PIN_1  = 23;  // เซอร์โวเตรียมแก้วตัวที่ 1
const int PREP_PIN_2  = 22;  // เซอร์โวเตรียมแก้วตัวที่ 2

// เซอร์โวหมุนรอบ (continuous rotation) — ปรับได้ตามรุ่นของคุณ (เซอร์โวถาด)
const int SERVO_US_CCW  = 1700;   // ทวนเข็ม (>1500)
const int SERVO_US_STOP = 1500;   // หยุด
const int SERVO_US_CW   = 2000;   // ตามเข็ม

inline void servoStop() { cupServo.writeMicroseconds(SERVO_US_STOP); }
inline void servoCCW()  { cupServo.writeMicroseconds(SERVO_US_CCW); }
inline void servoCW()   { cupServo.writeMicroseconds(SERVO_US_CW);  }

/* =======================
 *  SENSORS
 *  - S1, S2 = Ultrasonic
 *  - S3, S4 = TCRT (Active-Low)
 * =======================*/

/* --- Ultrasonic (S1,S2) --- */
// Mapping (ตัวอย่างการต่อจริง ให้ยึดตามที่คุณใช้):
// S1 = TRIG G16, ECHO G17   |  S2 = TRIG G19, ECHO G18
const int ULTRA1_TRIG = 16;
const int ULTRA1_ECHO = 17;
const int ULTRA2_TRIG = 19;
const int ULTRA2_ECHO = 18;

const float SOUND_CM_PER_US = 0.0343f;          // ~0.0343 cm/us (ที่ ~20-25°C)
const unsigned long ULTRA_TIMEOUT_US = 30000UL; // 30ms (~5m) เผื่อไว้

// >>> เกณฑ์ระยะของแต่ละตัว
const float ULTRA1_THRESHOLD_CM = 10.5f;  // S1 ทริก ≤ 11.5 ซม.
const float ULTRA2_THRESHOLD_CM = 10.0f;  // S2 ทริก ≤ 12 ซม. (ใช้ใน whichSensor เท่านั้น)

// ===== Preparing-cup (S2) rules =====
const float PREP_NEAR_CM = 3.8f;            // ต้องใกล้กว่า 4 ซม. จึงถือว่ามีแก้ว
const unsigned long PREP_CONFIRM_MS = 2000; // ค้างยืนยัน 2 วิ

// ===== โมชันของเซอร์โวเตรียมแก้ว (G23/G22) =====
int prep_pos = 0;
const uint8_t prep_stepSize   = 10;   // ก้าวหมุนทีละ 10°
const unsigned long prep_stepDelayMs = 20;   // หน่วงต่อก้าว
int shakeAmplitude = 80;   // แกว่งรวม 80° (±40°)
int shakeDelay    = 150;   // หน่วงต่อการแกว่งครั้งละฝั่ง (ms)
int shakeTimes    = 5;     // จำนวนครั้งต่อรอบเขย่า

float readUltrasonicCm(int trigPin, int echoPin) {
  // ป้องกัน echo ติดค้าง
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);

  // ส่งทริก 10us
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // วัดช่วงเวลาพัลส์
  unsigned long dur = pulseIn(echoPin, HIGH, ULTRA_TIMEOUT_US);
  if (dur == 0) return NAN;  // time out

  // ระยะทาง (ไป-กลับ) ⇒ /2
  float cm = (dur * SOUND_CM_PER_US) / 2.0f;
  return cm;
}

// อ่านหลายครั้งแล้วหา "ค่าเฉลี่ย" เพื่อความนิ่ง
float readUltrasonicAverageCm(int trigPin, int echoPin, int samples = 5, int gapMs = 40) {
  float sum = 0.0f;
  int cnt = 0;
  for (int i = 0; i < samples; ++i) {
    float v = readUltrasonicCm(trigPin, echoPin);
    if (!isnan(v)) {
      sum += v;
      cnt++;
    }
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

/* Which sensor hit?
 * - 1 = Ultrasonic S1 (ระยะ ≤ ULTRA1_THRESHOLD_CM)
 * - 2 = Ultrasonic S2 (ระยะ ≤ ULTRA2_THRESHOLD_CM)
 * - 3 = TCRT S3 (G27) Active-Low
 * - 4 = TCRT S4 (G26) Active-Low
 * - 0 = none
 */
int whichSensor() {
  if (ultrasonicTriggered(ULTRA1_TRIG, ULTRA1_ECHO, ULTRA1_THRESHOLD_CM)) return 1;
  if (ultrasonicTriggered(ULTRA2_TRIG, ULTRA2_ECHO, ULTRA2_THRESHOLD_CM)) return 2;
  if (tcrtRaw(TCRT_S3)) return 3;
  if (tcrtRaw(TCRT_S4)) return 4;
  return 0;
}

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
  secureClient.setInsecure();   // DEV: ข้าม cert; โปรดักชันควร setCACert()
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

/* ส่ง progress */
bool postProgressExtended(const String& orderId,
                          const String& step,
                          const String& status,
                          const String& message,
                          bool tcrtHit,
                          const char* eventTag,
                          int sensorIndex /*1..4, 0=unknown*/) {
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
 *  Helper: รอเซนเซอร์ที่ “ระบุหมายเลข” ให้ชน แล้วหยุด/ดีเลย์ตามต้องการ
 * =======================*/
bool waitForSensorHitSequence(const String& orderId,
                              int targetSensor,
                              unsigned long stopMs,
                              const char* reportStep,
                              bool skipOnly) {
  Serial.printf("[seq] Waiting for S%d ...\n", targetSensor);
  servoCCW();
  unsigned long tLastLog = 0;

  while (true) {
    int w = whichSensor();
    if (w == targetSensor) {
      // debounce: ต้องคงอยู่ >= TCRT_DEBOUNCE_MS
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
          delay(stopMs);
          servoCCW();  // หมุนต่อ
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
 *  NEW (สำหรับ S2): เซอร์โว G23/G22 วิ่งเฉพาะตอนเตรียมแก้ว
 *  - ถึง S2 → หยุดถาด + แจ้ง preparing_cup
 *  - ระหว่างยังไม่มีแก้ว (ULTRA2 >= 4 ซม.) → หมุนไปถึง 180° แล้ว "เขย่า" ไป–มา
 *  - เมื่อ ULTRA2 < 4 ซม. คอนเฟิร์ม 2 วิ → หยุดเซอร์โว G23/G22 และหมุนถาดต่อ
 * =======================*/
const int PREP_HOME_DEG = 0;  // องศาเริ่มต้น
void prepServosWriteBoth(int deg) {
  prepServo1.write(deg);
  prepServo2.write(deg);
}

// วิ่งจาก prep_pos ไปยัง target แบบเป็นขั้น ๆ
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

void prepServosStop() {
  // สำหรับ SG90/STD servo แค่ค้างองศาปัจจุบัน; ถ้าเป็น continuous ให้คุม writeMicroseconds(1500)
  // ที่นี่ใช้แบบมาตรฐานองศา:
  // ไม่ต้องทำอะไรเพิ่มเติมก็ได้ หรือจะคงที่ pos ปัจจุบัน
}

bool gotoAndPrepareAtS2(const String& orderId) {
  Serial.println("[prep] Seeking S2 ...");
  servoCCW();
  unsigned long tLastLog = 0;

  // หมุนหา S2
  while (true) {
    int w = whichSensor();
    if (w == 2) {
      unsigned long hitAt = millis();
      while ((whichSensor() == 2) && (millis() - hitAt < TCRT_DEBOUNCE_MS)) { delay(1); }
      if (millis() - hitAt >= TCRT_DEBOUNCE_MS) {
        // พบ S2 → หยุดถาด + แจ้งเริ่ม preparing_cup
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

  // === เริ่มควบคุมเซอร์โวเตรียมแก้ว (G23/G22) ===
  prep_pos = 0;
  prepServosWriteBoth(prep_pos);

  Serial.printf("[prep] Waiting ULTRA2 < %.1f cm then hold %lu ms (servos active)\n", PREP_NEAR_CM, PREP_CONFIRM_MS);

  while (true) {
    // อ่านระยะ
    float nowAvg = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);

    // ถ้ายังไม่มีแก้ว → ขยับเซอร์โวตามพฤติกรรมตัวอย่าง
    if (isnan(nowAvg) || nowAvg >= PREP_NEAR_CM) {
      // 1) ค่อย ๆ ไต่ไปจนถึง 180°
      if (prep_pos < 180) {
        prep_pos += prep_stepSize;
        if (prep_pos > 180) prep_pos = 180;
        prepServosWriteBoth(prep_pos);
        delay(prep_stepDelayMs);
      } else {
        // 2) ถึง 180 แล้ว → เขย่าไป–มา
        int halfAmp = shakeAmplitude / 2;
        for (int i = 0; i < shakeTimes; i++) {
          prepServosWriteBoth(prep_pos - halfAmp);
          delay(shakeDelay);
          prepServosWriteBoth(prep_pos + halfAmp);
          delay(shakeDelay);
        }
        // กลับตำแหน่งเดิมที่ 180
        prepServosWriteBoth(prep_pos);
      }

      // debug log เป็นช่วง ๆ
      static unsigned long lastLog = 0;
      if (millis() - lastLog >= 300) {
        lastLog = millis();
        Serial.printf("[prep] ULTRA2= %s cm (need < %.1f)  pos=%d\n",
                      isnan(nowAvg) ? "NaN" : String(nowAvg,1).c_str(),
                      PREP_NEAR_CM, prep_pos);
      }
    } else {
      // มีแก้วเข้ามาแล้ว → คอนเฟิร์ม 2 วิ
      Serial.printf("[prep] near %.1f cm detected → confirming...\n", nowAvg);
      delay(PREP_CONFIRM_MS);
      // อ่านซ้ำยืนยัน
      float confirm = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);
      if (!isnan(confirm) && confirm < PREP_NEAR_CM) {
        // วิ่งกลับจุดเริ่มต้นอย่างนุ่ม ๆ แล้วค่อยไปต่อ
        Serial.println("[prep] confirmed cup present → homing prep servos...");
        prepServosMoveTo(PREP_HOME_DEG, /*step*/10, /*delay*/20);
        Serial.println("[prep] homed → continue to next step");

        // หมุนถาดไปขั้นถัดไป
        servoCCW();
        return true;
      } else {
        Serial.println("[prep] confirm failed (not near anymore), keep working...");
      }
    }

    delay(10);
    yield();
  }
}

/* =======================
 *  รอให้ลูกค้ายกแก้วออกหลัง brewing_drink (ULTRA1 เพิ่ม ≥ 4 ซม.)
 * =======================*/
void waitForPickupAfterBrew(const String& orderId, float pickupDeltaCm = 4.0f) {
  // หยุดมอเตอร์ก่อนวัด
  servoStop();

  // อ่าน baseline
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
  servoCCW();

  // S1 (Ultrasonic #1) → ข้าม
  waitForSensorHitSequence(orderId, 1, 0, "", true);

  // S2 (Ultrasonic #2) → หยุดถาด + เซอร์โว G23/G22 ทำงานจนมีแก้วเข้าใกล้ < 4 ซม. (คอนเฟิร์ม 2 วิ) → หมุนต่อ
  gotoAndPrepareAtS2(orderId);

  // S3 (TCRT G27) →หยุด + adding_toppings
  waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);

  // S4 (TCRT G26) →หยุด + adding_ice
  waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);

  // S1 (Ultrasonic #1) →หยุด + brewing_drink
  waitForSensorHitSequence(orderId, 1, HOLD_MS, "brewing_drink", false);

  // หลัง brewing_drink: รอให้ลูกค้ายกแก้ว (ULTRA1 เพิ่ม ≥ 4 ซม.) → completed
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

  // TCRT pins (Active-Low)
  pinMode(TCRT_S3, INPUT_PULLUP);
  pinMode(TCRT_S4, INPUT_PULLUP);

  // Attach servos
  cupServo.attach(SERVO_PIN, 500, 2500);
  prepServo1.attach(PREP_PIN_1); // เซอร์โวมาตรฐานองศา
  prepServo2.attach(PREP_PIN_2);
  prepServosWriteBoth(0);        // เริ่มที่ 0°
  servoStop();                   // ถาดหยุด

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
