#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ==== [NEW] I2C / LCD / RTC ====
#include <Wire.h>
#include <LiquidCrystal_I2C.h>   // ติดตั้ง lib LiquidCrystal_I2C
#include <RTClib.h>              // ติดตั้ง lib RTClib (รองรับ DS1307/DS3231)

// LCD ปกติ address มักเป็น 0x27 (บางบอร์ด 0x3F)
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS1307 rtc;

// โซนเวลาไทย (UTC+7)
const char* TZ_ICT = "ICT-7";

// สถานะที่จะแสดงบน LCD
String g_currentStep = "idle";
String g_currentOrderId = "";
bool   g_online = false;
int    g_queuePos = -1;
unsigned long g_lastLcdUpdate = 0;
const unsigned long LCD_UPDATE_MS = 250;   // เร่งให้ถี่ขึ้นเพื่อเห็นการเปลี่ยน step ไวขึ้น

// =======================
//  เซตฮาร์ดแวร์ / พิน
// =======================
Servo cupServo;            // เซอร์โวถาดหลัก G13 (continuous-rotation)
Servo prepServo1;          // เซอร์โวเตรียมแก้ว G23 (มาตรฐานองศา)
Servo prepServo2;          // เซอร์โวเตรียมแก้ว G22 (มาตรฐานองศา)
Servo toppingServo;        // เซอร์โว topping ที่ GPIO2 (แนวทางกันกระตุก: attach เฉพาะตอนใช้)
Servo iceServo;            // เซอร์โวสำหรับน้ำแข็ง G15 (แนวทางเดียวกัน)

const int SERVO_PIN   = 13;  // ถาด
const int PREP_PIN_1  = 23;  // เตรียมแก้ว #1
const int PREP_PIN_2  = 22;  // เตรียมแก้ว #2
const int TOPPING_SERVO_PIN = 2;   // topping (S3) → ใช้ attach/detach on-demand
const int ICE_SERVO_PIN     = 15;  // ice (S4) → ใช้ attach/detach on-demand

// [ตามคำขอ] ย้าย Brewing Control จาก G33 → G32
const int BREW_CTRL_PIN      = 32;
const bool BREW_ACTIVE_LEVEL = LOW;           // ต้องการลอจิก 0 ตอนทำงาน
const unsigned long BREW_PULSE_MS = 5000;     // ค้าง 5 วินาที

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
const float START_US1_SKIP_CM = 32.0f;

// ===== Preparing-cup (S2) rules =====
const float PREP_NEAR_CM = 5.0f;            // ต้องใกล้กว่า 5 ซม. ถึงนับว่ามีแก้ว
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
const int TOPPING_MIN_DEG   = 0;
const int TOPPING_MAX_DEG   = 45;
const unsigned long TOPPING_PULSE_MS = 300;

// ===== ice (S4 @ G15) =====
const int ICE_MIN_DEG   = 0;
const int ICE_MAX_DEG   = 45;
const unsigned long ICE_UP_HOLD_MS   = 300;  // ค้างตอน 45°
const unsigned long ICE_DOWN_HOLD_MS = 200;  // ค้างตอน 0°
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
 *  I2C Pins (ตามคำขอ)
 * =======================*/
const int I2C_SDA = 21;
const int I2C_SCL = 33;

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
    // ปั๊ม LCD ระหว่างรอ
    // (จะโชว์ OFFLINE จนกว่า WiFi จะมา)
    // แสดงเวลา RTC ต่อเนื่อง
    // เพื่อเห็นหน้าจอไม่ค้าง
    // (ไม่ต้องเปลี่ยนค่าอื่น)
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
 *  [NEW] Helpers: เวลา & LCD
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
      // ปั๊ม LCD ระหว่างรอ
      // ให้เวลาเดินบนจอได้จริง
      updateLCD();
      yield();
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

String ipOrOffline() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return String("OFFLINE");
}

void updateLCD() {
  if (millis() - g_lastLcdUpdate < LCD_UPDATE_MS) return;
  g_lastLcdUpdate = millis();

  DateTime now = getNowFromRTCOrSystem();
  char dt1[21], dt2[21];
  snprintf(dt1, sizeof(dt1), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  snprintf(dt2, sizeof(dt2), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Row0: HW + IP/OFFLINE
  lcd.setCursor(0, 0);
  {
    String line = HARDWARE_ID;
    while (line.length() < 10) line += ' ';
    line += "IP:";
    String ip = ipOrOffline();
    while (line.length() < 14) line += ' ';
    if (ip.length() > 15) ip = ip.substring(0, 15);
    line += ip;
    if (line.length() > 20) line.remove(20);
    while (line.length() < 20) line += ' ';
    lcd.print(line);
  }

  // Row1: Step
  lcd.setCursor(0, 1);
  {
    String s = "Step: " + (g_currentStep.length() ? g_currentStep : String("-"));
    if (s.length() > 20) s.remove(20);
    while (s.length() < 20) s += ' ';
    lcd.print(s);
  }

  // Row2: OrderID (ตัดให้พอดี)
  lcd.setCursor(0, 2);
  {
    String o = g_currentOrderId.length() ? ("Order:" + g_currentOrderId) : "Order:-";
    if (o.length() > 20) o.remove(20);
    while (o.length() < 20) o += ' ';
    lcd.print(o);
  }

  // Row3: Date + Time (ICT)
  lcd.setCursor(0, 3);
  {
    String l = String(dt1) + " " + dt2;
    if (l.length() > 20) l.remove(20);
    while (l.length() < 20) l += ' ';
    lcd.print(l);
  }
}

// [NEW] หน่วงเวลาแบบไม่บล็อคการอัปเดตจอ
void waitMsWithLcd(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    updateLCD();
    delay(20);
    yield();
  }
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
  // อัปเดตสถานะไว้โชว์บน LCD ทันที
  g_currentStep = step;
  g_currentOrderId = orderId;
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
  waitMsWithLcd(100);
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
  waitMsWithLcd(100);
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
    updateLCD(); // ปั๊มจอทุกลูป

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
        updateLCD();
        yield();
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
          updateLCD(); // อัปเดตทันทีที่ประกาศ step

          // S3 → topping servo 0°→45°→0°
          if (targetSensor == 3 && reportStep && String(reportStep) == "adding_toppings") {
            toppingAttachNeutral();
            toppingServo.write(TOPPING_MAX_DEG);
            waitMsWithLcd(TOPPING_PULSE_MS);
            toppingServo.write(TOPPING_MIN_DEG);
            waitMsWithLcd(100);
            toppingDetachIdle();
          }

          // S4 → ice servo @ G15: ทำ 3 ครั้ง 0°→45°→0°
          if (targetSensor == 4 && reportStep && String(reportStep) == "adding_ice") {
            iceAttachNeutral();
            for (int i = 0; i < ICE_REPEAT_TIMES; ++i) {
              iceServo.write(ICE_MAX_DEG);
              waitMsWithLcd(ICE_UP_HOLD_MS);
              iceServo.write(ICE_MIN_DEG);
              waitMsWithLcd(ICE_DOWN_HOLD_MS);
            }
            iceDetachIdle();
          }

          // S1 → brewing_drink (G32 = LOW 5 วิ)
          if (targetSensor == 1 && reportStep && String(reportStep) == "brewing_drink") {
            digitalWrite(BREW_CTRL_PIN, BREW_ACTIVE_LEVEL);        // LOW
            waitMsWithLcd(BREW_PULSE_MS);                          // 5s พร้อมอัปเดตจอ
            digitalWrite(BREW_CTRL_PIN, !BREW_ACTIVE_LEVEL);       // HIGH
          }

          waitMsWithLcd(stopMs);
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
    waitMsWithLcd(stepDelay); // อัปเดตจอระหว่างขยับ
  }
}

bool gotoAndPrepareAtS2(const String& orderId) {
  Serial.println("[prep] Seeking S2 ...");
  servoCCW();
  unsigned long tLastLog = 0;
  unsigned long tStart = millis();

  // หมุนหา S2 (มี timeout)
  while (true) {
    updateLCD();

    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.println("[prep] Timeout seeking S2 → STOP");
      servoStop();
      return false;
    }

    int w = whichSensor();
    if (w == 2) {
      unsigned long hitAt = millis();
      while ((whichSensor() == 2) && (millis() - hitAt < TCRT_DEBOUNCE_MS)) { delay(1); updateLCD(); }
      if (millis() - hitAt >= TCRT_DEBOUNCE_MS) {
        servoStop();
        (void)postProgressExtended(orderId, "preparing_cup", "in_progress",
                                   "หยุดเพื่อเตรียมแก้ว (S2)", true, "marker_hit", 2);
        updateLCD();
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
    updateLCD();

    float nowAvg = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);

    // ยังไม่มีแก้ว → ค่อย ๆ ไป 180° แล้วเขย่า
    if (isnan(nowAvg) || nowAvg >= PREP_NEAR_CM) {
      if (prep_pos < 180) {
        prep_pos += prep_stepSize;
        if (prep_pos > 180) prep_pos = 180;
        prepServosWriteBoth(prep_pos);
        waitMsWithLcd(prep_stepDelayMs);
      } else {
        int halfAmp = shakeAmplitude / 2;
        for (int i = 0; i < shakeTimes; i++) {
          prepServosWriteBoth(prep_pos - halfAmp);
          waitMsWithLcd(shakeDelay);
          prepServosWriteBoth(prep_pos + halfAmp);
          waitMsWithLcd(shakeDelay);
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
      waitMsWithLcd(PREP_CONFIRM_MS);
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
  Serial.println("[pickup] Measuring baseline on ULTRA1]...");
  float baseline = NAN;
  for (int attempt = 0; attempt < 3 && isnan(baseline); ++attempt) {
    baseline = readUltrasonicAverageCm(ULTRA1_TRIG, ULTRA1_ECHO, 8, 35);
  }
  if (isnan(baseline)) baseline = readUltrasonicCm(ULTRA1_TRIG, ULTRA1_ECHO);
  Serial.printf("[pickup] baseline= %s cm\n", isnan(baseline) ? "NaN" : String(baseline, 1).c_str());

  unsigned long tLastLog = 0;
  while (true) {
    updateLCD();

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

    waitMsWithLcd(80);
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
  updateLCD();

  // --- เช็ค ULTRA1 ตอนเริ่ม: ถ้าใกล้มาก (<30cm) ข้าม S1 ---
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

  // S3 → หยุด + adding_toppings
  waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);

  // S4 → หยุด + adding_ice
  waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);

  // S1 → หยุด + brewing_drink
  waitForSensorHitSequence(orderId, 1, HOLD_MS, "brewing_drink", false);

  // รอให้ลูกค้ายกแก้ว → completed
  waitForPickupAfterBrew(orderId, 4.0f);

  // เสร็จแล้วหยุด
  servoStop();
  Serial.println("[seq] Sequence completed → STOP");
  updateLCD();
}

/* =======================
 *  Setup / Loop
 * =======================*/
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf("[sim] Starting hardware sim %s @ %s\n", HARDWARE_ID, BASE_URL);

  // I2C begin (SDA=G21, SCL=G33)
  Wire.begin(I2C_SDA, I2C_SCL);

  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(3,0); lcd.print("Booting...");
  lcd.setCursor(2,1); lcd.print("LCD 2004A Ready");
  lcd.setCursor(5,2); lcd.print("I2C 0x27");
  lcd.setCursor(4,3); lcd.print("RTC init...");

  // RTC begin
  if (!rtc.begin()) {
    Serial.println("[rtc] RTC not found. LCD will use system time.");
    lcd.setCursor(2,3); lcd.print("RTC not found    ");
  } else {
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println("[rtc] RTC started with compile time.");
    }
    lcd.setCursor(6,3); lcd.print("RTC OK     ");
  }

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

  // ถาด (CR servo)
  cupServo.setPeriodHertz(50);
  cupServo.attach(SERVO_PIN, 500, 2500);
  servoStop();

  // Servos เตรียมแก้ว
  prepServo1.attach(PREP_PIN_1, 500, 2500);
  prepServo2.attach(PREP_PIN_2, 500, 2500);

  // topping/ice: ห้าม attach ตอนบูต
  pinMode(TOPPING_SERVO_PIN, OUTPUT);
  digitalWrite(TOPPING_SERVO_PIN, LOW);
  pinMode(ICE_SERVO_PIN, OUTPUT);
  digitalWrite(ICE_SERVO_PIN, LOW);

  // ควบคุม brewing_drink (G32): ตั้งเป็น HIGH ไว้ก่อน
  pinMode(BREW_CTRL_PIN, OUTPUT);
  digitalWrite(BREW_CTRL_PIN, !BREW_ACTIVE_LEVEL);   // HIGH

  // ตำแหน่งเริ่มต้นของ G23/G22
  prep_pos = 0;
  prepServosWriteBoth(0);

  connectWiFi();
  g_online = (WiFi.status() == WL_CONNECTED);

  // ซิงค์เวลาเมื่อออนไลน์ แล้วเขียนลง RTC
  trySyncTimeAndWriteRTC();

  // หน้าจอหลังบูต
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
    g_currentOrderId = orderId;
    (void)postProgressExtended(orderId, "queued", "in_progress", "รับออเดอร์เข้าคิว", false, "order_received", 0);
    updateLCD();

    Serial.printf("[sim] Received order %s (#%d)\n", orderId.c_str(), queuePos);
    brew(orderId);
    Serial.printf("[sim] Completed order %s\n", orderId.c_str());
    servoStop();

    // ความปลอดภัย: แน่ใจว่า GPIO2/GPIO15/G32 ไม่ถูกขับทิ้งไว้ผิดสถานะ
    toppingDetachIdle();
    iceDetachIdle();
    digitalWrite(BREW_CTRL_PIN, !BREW_ACTIVE_LEVEL); // คืน HIGH
  } else {
    servoStop();
    waitMsWithLcd(POLL_IDLE_MS); // ระหว่าง idle ก็อัปเดตจอ
  }

  updateLCD();
}
