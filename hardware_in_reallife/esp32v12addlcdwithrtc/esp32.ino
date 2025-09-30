#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ==== [NEW] I2C / LCD / RTC ====
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS1307 rtc;

const char* TZ_ICT = "ICT-7";

// ==== สถานะสำหรับ LCD ====
String g_currentStep   = "idle";
String g_currentDrink  = "Unknown";
bool   g_online        = false;
int    g_queuePos      = -1;
unsigned long g_lastLcdUpdate = 0;
const unsigned long LCD_UPDATE_MS = 500;

// =======================
//  เซตฮาร์ดแวร์ / พิน
// =======================
Servo cupServo;            // เซอร์โวถาดหลัก G13 (continuous-rotation)
Servo prepServo1;          // เซอร์โวเตรียมแก้ว G23 (มาตรฐานองศา)
Servo prepServo2;          // เซอร์โวเตรียมแก้ว G22 (มาตรฐานองศา)
Servo toppingServo;        // เซอร์โว topping ที่ GPIO2 (attach/detach)
Servo iceServo;            // เซอร์โวสำหรับน้ำแข็ง G15

const int SERVO_PIN   = 13;  // ถาด
const int PREP_PIN_1  = 23;  // เตรียมแก้ว #1
const int PREP_PIN_2  = 22;  // เตรียมแก้ว #2
const int TOPPING_SERVO_PIN = 2;   // topping
const int ICE_SERVO_PIN     = 15;  // ice

// [คงตามที่ตั้งไว้] Brewing Control → G32 (Active LOW 5s)
const int BREW_CTRL_PIN      = 32;
const bool BREW_ACTIVE_LEVEL = LOW;
const unsigned long BREW_PULSE_MS = 5000;

// เซอร์โวถาด (continuous rotation)
const int SERVO_US_CCW  = 1700;
const int SERVO_US_STOP = 1500;
const int SERVO_US_CW   = 2000;

inline void servoStop() { cupServo.writeMicroseconds(SERVO_US_STOP); }
inline void servoCCW()  { cupServo.writeMicroseconds(SERVO_US_CCW); }
inline void servoCW()   { cupServo.writeMicroseconds(SERVO_US_CW);  }

/* =======================
 *  SENSORS
 * =======================*/
const int ULTRA1_TRIG = 16;
const int ULTRA1_ECHO = 17;
const int ULTRA2_TRIG = 19;
const int ULTRA2_ECHO = 18;

const float SOUND_CM_PER_US = 0.0343f;
const unsigned long ULTRA_TIMEOUT_US = 30000UL;

const float ULTRA1_THRESHOLD_CM = 11.5f;
const float ULTRA2_THRESHOLD_CM = 10.5f;

const float START_US1_SKIP_CM = 32.0f;

// ===== Preparing-cup (S2) rules =====
const float PREP_NEAR_CM = 5.0f;
const unsigned long PREP_CONFIRM_MS = 2000;

// ===== โมชันเซอร์โวเตรียมแก้ว =====
int prep_pos = 0;
const uint8_t        prep_stepSize     = 10;
const unsigned long  prep_stepDelayMs  = 20;
int shakeAmplitude = 80;
int shakeDelay    = 150;
int shakeTimes    = 5;
const int PREP_HOME_DEG = 0;

// ===== topping (S3) =====
const int TOPPING_MIN_DEG   = 0;
const int TOPPING_MAX_DEG   = 45;
const unsigned long TOPPING_PULSE_MS = 300;

// ===== ice (S4 @ G15) =====
const int ICE_MIN_DEG   = 0;
const int ICE_MAX_DEG   = 45;
const unsigned long ICE_UP_HOLD_MS   = 300;
const unsigned long ICE_DOWN_HOLD_MS = 200;
const int ICE_REPEAT_TIMES = 3;

/* =======================
 *  Wi-Fi / API config
 * =======================*/
const char* ssid        = "totoiot";
const char* password    = "123456789";
const char* BASE_URL    = "https://porametix.online";
const char* HARDWARE_ID = "esp32-001";
const char* API_KEY     = "odroid-hardware-key-1758367749";

/* =======================
 *  I2C Pins
 * =======================*/
const int I2C_SDA = 21;   // ตามคำขอ: SDA=G21
const int I2C_SCL = 33;   // ตามคำขอ: SCL=G33

/* =======================
 *  Timing
 * =======================*/
const unsigned long POLL_IDLE_MS   = 2000;
const unsigned long POLL_ERROR_MS  = 5000;
const unsigned long LOG_EVERY_MS   = 250;
const unsigned long FIND_TIMEOUT_MS = 30000;

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

  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[sim] JSON parse error (orders): %s\n", err.c_str());
    return false;
  }

  if (!doc["order"].isNull()) {
    JsonObject ord = doc["order"].as<JsonObject>();
    if (ord["orderId"].is<const char*>()) {
      orderId  = String(ord["orderId"].as<const char*>());
      queuePos = ord["queuePosition"] | -1;

      // [NEW] ดึงชื่อเครื่องดื่ม
      String drink = "Unknown";
      if (ord["drinkName"].is<const char*>())      drink = ord["drinkName"].as<const char*>();
      else if (ord["name"].is<const char*>())      drink = ord["name"].as<const char*>();
      else if (ord["menu"].is<const char*>())      drink = ord["menu"].as<const char*>();
      else if (ord["title"].is<const char*>())     drink = ord["title"].as<const char*>();
      g_currentDrink = drink;

      // โชว์บน LCD ทันทีว่ามีออเดอร์ใหม่
      g_currentStep = "queued";
      updateLCD();

      return true;
    }
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
  // [NEW] อัปเดตสถานะไว้โชว์บน LCD + แสดงทันที
  g_currentStep = step; 
  updateLCD();

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
 *  TOPPING & ICE SERVO HELPERS (attach on-demand)
 * =======================*/
void toppingAttachNeutral() {
  toppingServo.setPeriodHertz(50);
  toppingServo.attach(TOPPING_SERVO_PIN, 500, 2500);
  toppingServo.write(TOPPING_MIN_DEG);
  delay(100);
}
void toppingDetachIdle() {
  toppingServo.detach();
  pinMode(TOPPING_SERVO_PIN, OUTPUT);
  digitalWrite(TOPPING_SERVO_PIN, LOW);
}

void iceAttachNeutral() {
  iceServo.setPeriodHertz(50);
  iceServo.attach(ICE_SERVO_PIN, 500, 2500);
  iceServo.write(ICE_MIN_DEG);
  delay(100);
}
void iceDetachIdle() {
  iceServo.detach();
  pinMode(ICE_SERVO_PIN, OUTPUT);
  digitalWrite(ICE_SERVO_PIN, LOW);
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

          // [NEW] อัปเดตจอทันทีเมื่อเข้าสเต็ปแต่ละตัว
          updateLCD();

          if (targetSensor == 3 && reportStep && String(reportStep) == "adding_toppings") {
            toppingAttachNeutral();
            toppingServo.write(TOPPING_MAX_DEG);
            delay(TOPPING_PULSE_MS);
            toppingServo.write(TOPPING_MIN_DEG);
            delay(100);
            toppingDetachIdle();
          }

          if (targetSensor == 4 && reportStep && String(reportStep) == "adding_ice") {
            iceAttachNeutral();
            for (int i = 0; i < ICE_REPEAT_TIMES; ++i) {
              iceServo.write(ICE_MAX_DEG);
              delay(ICE_UP_HOLD_MS);
              iceServo.write(ICE_MIN_DEG);
              delay(ICE_DOWN_HOLD_MS);
            }
            iceDetachIdle();
          }

          if (targetSensor == 1 && reportStep && String(reportStep) == "brewing_drink") {
            digitalWrite(BREW_CTRL_PIN, BREW_ACTIVE_LEVEL);  // LOW
            delay(BREW_PULSE_MS);                            // 5s
            digitalWrite(BREW_CTRL_PIN, !BREW_ACTIVE_LEVEL); // HIGH
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
      // [NEW] ขณะหมุนหา ก็ดันอัปเดตจอให้วิ่งต่อเนื่อง
      updateLCD();
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
        updateLCD(); // [NEW]
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
      updateLCD(); // [NEW]
    }
    delay(1);
    yield();
  }

  prep_pos = 0;
  prepServosWriteBoth(prep_pos);

  Serial.printf("[prep] Waiting ULTRA2 < %.1f cm then hold %lu ms (servos active)\n", PREP_NEAR_CM, PREP_CONFIRM_MS);

  while (true) {
    float nowAvg = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);

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
        updateLCD(); // [NEW]
      }
    } else {
      Serial.printf("[prep] near %.1f cm detected → confirming...\n", nowAvg);
      delay(PREP_CONFIRM_MS);
      float confirm = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);
      if (!isnan(confirm) && confirm < PREP_NEAR_CM) {
        Serial.println("[prep] confirmed cup present → homing prep servos...");
        prepServosMoveTo(PREP_HOME_DEG, 10, 20);
        Serial.println("[prep] homed → continue to next step");
        servoCCW();
        updateLCD(); // [NEW]
        return true;
      } else {
        Serial.println("[prep] confirm failed, keep working...");
        updateLCD(); // [NEW]
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
  g_currentStep = "waiting_pickup"; // [NEW] โชว์ว่ารอหยิบแก้ว
  updateLCD();

  Serial.println("[pickup] Measuring baseline on ULTRA1]...");
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
      updateLCD(); // [NEW]
    }

    if (picked) {
      (void)postProgressExtended(orderId, "completed", "completed",
                                 "ลูกค้าหยิบแก้วแล้ว (ULTRA1 เพิ่ม ≥ 4cm)", false, "cup_taken", 1);
      g_currentStep = "completed";
      updateLCD(); // [NEW]
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

  (void)postProgressExtended(orderId, "start", "in_progress", "เริ่มต้น", false, "step_started", 0);
  updateLCD(); // [NEW]

  float us1Start = readUltrasonicAverageCm(ULTRA1_TRIG, ULTRA1_ECHO, 3, 25);
  Serial.printf("[start] US1 start = %s cm\n", isnan(us1Start) ? "NaN" : String(us1Start,1).c_str());

  if (!isnan(us1Start) && us1Start < START_US1_SKIP_CM) {
    Serial.println("[start] US1 < 30cm → skip S1 and go preparing_cup (S2)");
    if (!gotoAndPrepareAtS2(orderId)) {
      Serial.println("[start] Could not reach S2 in time, aborting this cycle.");
      servoStop();
      return;
    }
  } else {
    if (!waitForSensorHitSequence(orderId, 1, 0, "", true)) {
      Serial.println("[start] S1 wait timeout → continue to S2 anyway");
    }
    if (!gotoAndPrepareAtS2(orderId)) {
      Serial.println("[start] Could not reach S2 in time, aborting this cycle.");
      servoStop();
      return;
    }
  }

  // S3
  waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);
  updateLCD(); // [NEW]

  // S4
  waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);
  updateLCD(); // [NEW]

  // S1 brew
  waitForSensorHitSequence(orderId, 1, HOLD_MS, "brewing_drink", false);
  updateLCD(); // [NEW]

  // รอหยิบแก้ว
  waitForPickupAfterBrew(orderId, 4.0f);

  servoStop();
  Serial.println("[seq] Sequence completed → STOP");
}

/* =======================
 *  เวลา & LCD Helpers
 * =======================*/
DateTime getNowFromRTCOrSystem() {
  if (rtc.isrunning()) {
    return rtc.now();
  } else {
    time_t t = time(nullptr);
    if (t < 1700000000) {
      return DateTime(F(__DATE__), F(__TIME__));
    }
    struct tm *tm_info = localtime(&t);
    return DateTime(1900 + tm_info->tm_year, 1 + tm_info->tm_mon, tm_info->tm_mday,
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  }
}

void trySyncTimeAndWriteRTC() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[time] Sync via NTP...");
    setenv("TZ", TZ_ICT, 1);
    tzset();
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    for (int i = 0; i < 10; ++i) {
      time_t now = time(nullptr);
      if (now > 1700000000) break;
      delay(500);
    }
    time_t now = time(nullptr);
    if (now > 1700000000) {
      struct tm *tm_info = localtime(&now);
      DateTime dt(1900 + tm_info->tm_year, 1 + tm_info->tm_mon, tm_info->tm_mday,
                  tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
      if (!rtc.begin()) {
        Serial.println("[time] RTC not found to write (but system time synced).");
      } else {
        rtc.adjust(dt);
        Serial.println("[time] RTC updated from NTP.");
      }
    } else {
      Serial.println("[time] NTP sync failed.");
    }
  }
}

void lcdPrintCentered(uint8_t row, const String& s) {
  String msg = s;
  if (msg.length() > 20) msg.remove(20);
  int pad = (20 - msg.length()) / 2;
  String line = String(' ', pad) + msg;
  while (line.length() < 20) line += ' ';
  lcd.setCursor(0, row);
  lcd.print(line);
}

String ipOrOffline() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return String("OFFLINE");
}

void updateLCD() {
  if (millis() - g_lastLcdUpdate < LCD_UPDATE_MS) return;
  g_lastLcdUpdate = millis();

  DateTime now = getNowFromRTCOrSystem();
  char dt1[21], dt2[21];
  snprintf(dt1, sizeof(dt1), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  snprintf(dt2, sizeof(dt2), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Row0: HW + IP/OFFLINE (ท้าย 5 ตัว)
  lcd.setCursor(0, 0);
  {
    String line = HARDWARE_ID;
    while (line.length() < 12) line += ' ';
    line += "IP:";
    while (line.length() < 15) line += ' ';
    String ip = ipOrOffline();
    String ipShort = ip;
    if (ipShort.length() > 5) ipShort = ipShort.substring(ipShort.length()-5);
    line += ipShort;
    line.remove(20);
    lcd.print(line);
  }

  // Row1: Step
  lcd.setCursor(0, 1);
  {
    String s = "Step: " + g_currentStep;
    if (s.length() > 20) s.remove(20);
    while (s.length() < 20) s += ' ';
    lcd.print(s);
  }

  // Row2: Drink (แทน Order)
  lcd.setCursor(0, 2);
  {
    String o = "Drink:" + (g_currentDrink.length() ? g_currentDrink : String("-"));
    if (o.length() > 20) o.remove(20);
    while (o.length() < 20) o += ' ';
    lcd.print(o);
  }

  // Row3: Date + Time
  lcd.setCursor(0, 3);
  {
    char buf[21];
    snprintf(buf, sizeof(buf), "%s %s", dt1, dt2);
    String l = String(buf);
    if (l.length() > 20) l.remove(20);
    while (l.length() < 20) l += ' ';
    lcd.print(l);
  }
}

/* =======================
 *  Setup / Loop
 * =======================*/
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf("[sim] Starting hardware sim %s @ %s\n", HARDWARE_ID, BASE_URL);

  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcdPrintCentered(0, "Booting...");
  lcdPrintCentered(1, "LCD 2004A Ready");
  lcdPrintCentered(2, "I2C 0x27");
  lcdPrintCentered(3, "RTC init...");

  if (!rtc.begin()) {
    Serial.println("[rtc] RTC not found. LCD will use system time.");
    lcdPrintCentered(3, "RTC not found");
  } else {
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println("[rtc] RTC started with compile time.");
    }
    lcdPrintCentered(3, "RTC OK");
  }

  pinMode(ULTRA1_TRIG, OUTPUT);
  pinMode(ULTRA1_ECHO, INPUT);
  pinMode(ULTRA2_TRIG, OUTPUT);
  pinMode(ULTRA2_ECHO, INPUT);
  digitalWrite(ULTRA1_TRIG, LOW);
  digitalWrite(ULTRA2_TRIG, LOW);

  pinMode(TCRT_S3, INPUT_PULLUP);
  pinMode(TCRT_S4, INPUT_PULLUP);

  cupServo.setPeriodHertz(50);
  cupServo.attach(SERVO_PIN, 500, 2500);
  servoStop();

  prepServo1.attach(PREP_PIN_1, 500, 2500);
  prepServo2.attach(PREP_PIN_2, 500, 2500);

  pinMode(TOPPING_SERVO_PIN, OUTPUT);
  digitalWrite(TOPPING_SERVO_PIN, LOW);
  pinMode(ICE_SERVO_PIN, OUTPUT);
  digitalWrite(ICE_SERVO_PIN, LOW);

  pinMode(BREW_CTRL_PIN, OUTPUT);
  digitalWrite(BREW_CTRL_PIN, !BREW_ACTIVE_LEVEL);

  prep_pos = 0;
  prepServosWriteBoth(0);

  connectWiFi();
  g_online = (WiFi.status() == WL_CONNECTED);

  trySyncTimeAndWriteRTC();

  lcd.clear();
  updateLCD();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (g_online) {
      Serial.println("[sim] WiFi dropped, reconnecting...");
    }
    g_online = false;
    servoStop();
    connectWiFi();
    g_online = (WiFi.status() == WL_CONNECTED);
    if (g_online) {
      trySyncTimeAndWriteRTC();
    }
  }

  String orderId; int queuePos = -1;
  bool ok = pollNextOrder(orderId, queuePos);
  if (!ok) {
    Serial.println("[sim] Error polling orders, retry soon...");
    servoStop();
    updateLCD();
    delay(POLL_ERROR_MS);
    return;
  }

  g_queuePos = queuePos;

  if (orderId.length() > 0) {
    Serial.printf("[sim] Received order %s (#%d) drink=%s\n",
                  orderId.c_str(), queuePos, g_currentDrink.c_str());

    // บอกสถานะก่อนเริ่มจริง
    g_currentStep = "start";
    updateLCD();

    brew(orderId);

    Serial.printf("[sim] Completed order %s\n", orderId.c_str());
    servoStop();

    toppingDetachIdle();
    iceDetachIdle();
    digitalWrite(BREW_CTRL_PIN, !BREW_ACTIVE_LEVEL); // คืน HIGH
  } else {
    servoStop();
    delay(POLL_IDLE_MS);
  }

  updateLCD();
}
