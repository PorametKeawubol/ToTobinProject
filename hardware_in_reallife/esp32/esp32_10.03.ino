/******************************************************
 * ESP32 Drink Robot + Audio Steps (LittleFS + I2S DAC)
 * - เล่นเสียงตามสเต็ป: S2(เตรียมแก้ว) / S3(ท็อปปิ้ง) / S4(น้ำแข็ง)
 *   / S1(ชงเครื่องดื่ม) / Completed(ลูกค้าหยิบแก้ว)
 * - ใช้ไฟล์ใน LittleFS:
 *   /precup.wav, /adding_topping.wav, /adding_ice.wav,
 *   /making_drink.wav, /completed.wav, (/totosong_8k_adpcm.wav - optional)
 ******************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ====== AUDIO / FS ======
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

// ===== [AUDIO] Globals =====
AudioGeneratorWAV* g_wav = nullptr;
AudioFileSourceLittleFS* g_file = nullptr;
AudioOutputI2S* g_out = nullptr;

// ปรับความดัง (0-30% แนะนำเพื่อกันแตก)
void setAudioVolume(uint8_t percent) {
  if (!g_out) return;
  percent = constrain(percent, 0, 30);
  g_out->SetGain(percent / 100.0f);
}

// เริ่มระบบเสียง + LittleFS (เรียกครั้งเดียวใน setup)
void audioInit() {
  static bool inited = false;
  if (inited) return;
  LittleFS.begin(true);
  g_out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  g_out->SetPinout(25, -1, -1);  // DAC1 -> GPIO25
  setAudioVolume(8);             // เริ่มเบา ๆ
  inited = true;
}

// เล่นไฟล์ใหม่ (หยุดไฟล์เดิมอัตโนมัติ)
bool audioPlay(const char* path, uint8_t volPercent = 8) {
  audioInit();
  // stop เดิม
  if (g_wav) { g_wav->stop(); delete g_wav; g_wav = nullptr; }
  if (g_file){ g_file->close(); delete g_file; g_file = nullptr; }

  g_file = new AudioFileSourceLittleFS(path);
  if (!g_file) return false;

  g_wav  = new AudioGeneratorWAV();
  if (!g_wav) { delete g_file; g_file = nullptr; return false; }

  setAudioVolume(volPercent);
  if (!g_wav->begin(g_file, g_out)) {
    delete g_wav;  g_wav = nullptr;
    g_file->close(); delete g_file; g_file = nullptr;
    return false;
  }
  return true;
}

// ให้ออดิโอทำงานต่อเนื่อง (ต้องเรียกบ่อย ๆ)
void audioLoop() {
  if (g_wav && g_wav->isRunning()) {
    if (!g_wav->loop()) {
      g_wav->stop();
      if (g_out) g_out->SetGain(0.0f);
      delete g_wav;  g_wav  = nullptr;
      if (g_file) { g_file->close(); delete g_file; g_file = nullptr; }
    }
  }
}

// delay แบบไม่ทำให้เสียงสะดุด
void audioDelay(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    audioLoop();
    delay(5);
    yield();
  }
}

/* =======================
 *  เซตฮาร์ดแวร์ / พิน
 * =======================*/
Servo cupServo;            // เซอร์โวถาดหลัก G13 (continuous-rotation)
Servo prepServo1;          // เซอร์โวเตรียมแก้ว G23 (มาตรฐานองศา)
Servo prepServo2;          // เซอร์โวเตรียมแก้ว G22 (มาตรฐานองศา)
Servo toppingServo;        // เซอร์โว topping ที่ GPIO2 (attach เฉพาะตอนใช้)
Servo iceServo;            // เซอร์โวสำหรับน้ำแข็ง G15 (attach เฉพาะตอนใช้)

const int SERVO_PIN   = 13;  // ถาด
const int PREP_PIN_1  = 23;  // เตรียมแก้ว #1
const int PREP_PIN_2  = 22;  // เตรียมแก้ว #2
const int TOPPING_SERVO_PIN = 2;   // topping (S3)
const int ICE_SERVO_PIN     = 15;  // ice (S4)

// พินปั๊ม: P1=น้ำเชื่อม (เดิม), P2..P4=น้ำ (เพิ่มใหม่)
const int  PUMP1_PIN = 33;  // น้ำเชื่อม (P1) — เดิมคือ BREW_CTRL_PIN
const int  PUMP2_PIN = 32;  // น้ำเบส (P2)  NEW
const int  PUMP3_PIN = 12;  // น้ำเบส (P3)  NEW
const int  PUMP4_PIN = 14;  // น้ำเบส (P4)  NEW

// โลจิกปั๊ม: ACTIVE = LOW
const bool PUMP_ACTIVE_LEVEL = LOW;

// เวลาจ่ายน้ำเชื่อม (ความหวาน) — 100% = 2000ms
const unsigned long SYRUP_PULSE_MAX_MS = 2000;   // P1

// เวลาจ่ายน้ำ "เบส" รวม (แบ่งให้ P2..P4 ตามสูตร)
const unsigned long BASE_TOTAL_MS_SMALL   = 2200;
const unsigned long BASE_TOTAL_MS_REGULAR = 3000;
const unsigned long BASE_TOTAL_MS_LARGE   = 3800;

/* เซอร์โวถาด (continuous rotation) */
const int SERVO_US_CCW  = 1600;   // ทวนเข็ม (>1500)
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
const uint8_t  prep_stepSize    = 10;
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
 *  Timing
 * =======================*/
const unsigned long POLL_IDLE_MS   = 2000;
const unsigned long POLL_ERROR_MS  = 5000;
const unsigned long LOG_EVERY_MS   = 250;

// ป้องกัน “หมุนไม่หยุด” ระหว่างหา marker
const unsigned long FIND_TIMEOUT_MS = 30000;  // 30 วินาที

/* =======================
 *  สถานะออเดอร์ปัจจุบัน
 * =======================*/
String g_orderId;
String g_drinkName;          // NEW: ชื่อเครื่องดื่มจาก order
String g_size;               // NEW: ขนาด (Small/Regular/Large)
bool   g_needIce     = false;
bool   g_needPearls  = false;
int    g_sweetness   = 100;  // 0..100

/* =======================
 *  สูตรเครื่องดื่ม (P1=น้ำเชื่อม ห้ามใช้ในสูตร; ใช้เฉพาะ sweetness)
 *  ฟอร์แมต: {ชื่อ, {P1,P2,P3,P4}}  (ตั้ง P1=0 เสมอ)
 * =======================*/
struct Recipe {
  const char* name;
  uint8_t pct[4]; // P1..P4
};

Recipe RECIPES[] = {
  {"ชาไทย",     {  0, 100,   0,   0}}, // P2
  {"thai tea",   {  0, 100,   0,   0}},

  {"ชาเขียว",   {  0,   0, 100,   0}}, // P3
  {"green tea",  {  0,   0, 100,   0}},

  {"กาแฟ",      {  0,  50,  50,   0}}, // P2+P3
  {"coffee",     {  0,  50,  50,   0}},

  {"ชาผลไม้",   {  0,   0,   0, 100}}, // P4
  {"fruit tea",  {  0,   0,   0, 100}},

  {"โกโก้",      {  0,   0, 100,   0}}, // ตัวอย่าง: ใช้ P3
  {"cocoa",      {  0,   0, 100,   0}},

  {"ชานม",       {  0,  60,  40,   0}},
  {"milk tea",   {  0,  60,  40,   0}},
};

int findRecipeIndex(const String& drink) {
  String q = drink; q.toLowerCase();
  for (size_t i = 0; i < sizeof(RECIPES)/sizeof(RECIPES[0]); ++i) {
    String name = RECIPES[i].name; name.toLowerCase();
    if (name == q) return (int)i;
  }
  return -1;
}

unsigned long baseTotalMsForSize(const String& sizeStr) {
  String s = sizeStr; s.toLowerCase();
  if (s.indexOf("small")   >= 0) return BASE_TOTAL_MS_SMALL;
  if (s.indexOf("large")   >= 0) return BASE_TOTAL_MS_LARGE;
  return BASE_TOTAL_MS_REGULAR;
}

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
    audioLoop();
    delay(500);
    if (millis() - t0 > 30000) {
      Serial.println("\nWiFi connect timeout, retrying...");
      WiFi.disconnect(true);
      audioDelay(2000);
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

/* ตรวจชื่อท็อปปิ้ง (ไทย/อังกฤษ) */
bool isIceWord(const String& s) {
  String t = s; t.toLowerCase();
  return (t.indexOf("น้ำแข็ง") >= 0) || (t.indexOf("ice") >= 0);
}
bool isPearlWord(const String& s) {
  String t = s; t.toLowerCase();
  return (t.indexOf("ไข่มุก") >= 0) || (t.indexOf("pearl") >= 0) || (t.indexOf("boba") >= 0) || (t.indexOf("tapioca") >= 0);
}

/* เวลาเติมน้ำเชื่อมตามความหวาน: 0..100 → 0..2000ms (เชิงเส้น) */
unsigned long syrupMsFromSweetness(int sweetness) {
  if (sweetness < 0) sweetness = 0;
  if (sweetness > 100) sweetness = 100;
  return (unsigned long)((sweetness * SYRUP_PULSE_MAX_MS) / 100);
}

/* ดึงออเดอร์ + ตั้งค่าสถานะ */
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
  Serial.printf("[sim] Poll response: %s\n", payload.c_str());

  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[sim] JSON parse error (orders): %s\n", err.c_str());
    return false;
  }

  orderId      = "";
  queuePos     = -1;
  g_needIce    = false;
  g_needPearls = false;
  g_sweetness  = 100;
  g_drinkName  = "";
  g_size       = "Regular";

  JsonObject ord = doc["order"];
  if (!ord.isNull()) {
    if (ord["orderId"].is<const char*>()) {
      orderId  = String(ord["orderId"].as<const char*>());
    }
    queuePos = ord["queuePosition"] | -1;

    if (ord["drinkName"].is<const char*>()) {
      g_drinkName = String(ord["drinkName"].as<const char*>());
    }
    if (ord["size"].is<const char*>()) {
      g_size = String(ord["size"].as<const char*>());
    }

    // toppings[]
    if (ord["toppings"].is<JsonArray>()) {
      for (JsonVariant v : ord["toppings"].as<JsonArray>()) {
        if (v.is<const char*>()) {
          String item = v.as<const char*>();
          if (isIceWord(item))    g_needIce = true;
          if (isPearlWord(item))  g_needPearls = true;
        }
      }
    }

    // sweetness
    if (ord["sweetness"].is<int>()) {
      g_sweetness = ord["sweetness"].as<int>();
    }
  }

  g_orderId = orderId;

  Serial.printf("[sim] parsed: orderId=%s, queue=%d, drink='%s', size='%s', needIce=%d, needPearls=%d, sweetness=%d%%, syrupMs=%lums\n",
    g_orderId.c_str(), queuePos, g_drinkName.c_str(), g_size.c_str(),
    (int)g_needIce, (int)g_needPearls, g_sweetness, syrupMsFromSweetness(g_sweetness));

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
    audioDelay(gapMs);   // เดิม delay(gapMs)
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
 *  Progress API (เดิม)
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
 *  TOPPING & ICE SERVO HELPERS (attach on-demand)
 * =======================*/
void toppingAttachNeutral() {
  toppingServo.setPeriodHertz(50);
  toppingServo.attach(TOPPING_SERVO_PIN, 500, 2500);
  toppingServo.write(TOPPING_MIN_DEG);
  audioDelay(100);
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
  audioDelay(100);
}
void iceDetachIdle() {
  iceServo.detach();
  pinMode(ICE_SERVO_PIN, OUTPUT);
  digitalWrite(ICE_SERVO_PIN, LOW);
}

/* =======================
 *  Pump helpers (ใหม่)
 * =======================*/
inline void pumpOn(int pin)  { digitalWrite(pin, PUMP_ACTIVE_LEVEL); }
inline void pumpOff(int pin) { digitalWrite(pin, !PUMP_ACTIVE_LEVEL); }

// จ่ายปั๊มเดี่ยวแบบหน่วงเวลา (blocking) — ใช้ audioDelay เพื่อไม่ให้เสียงสะดุด
void pulsePumpMs(int pin, unsigned long ms) {
  if (ms == 0) return;
  pumpOn(pin);
  audioDelay(ms);      // เดิม delay(ms)
  pumpOff(pin);
}

// จ่าย “เบส” ตามสูตรของออเดอร์ปัจจุบัน (P2..P4) โดยแบ่งเวลา BASE_TOTAL_MS ตามสัดส่วน
void dispenseBaseForCurrentOrder() {
  // หา recipe
  int idx = findRecipeIndex(g_drinkName);
  uint8_t p1=0, p2=0, p3=0, p4=0;
  if (idx >= 0) {
    p1 = RECIPES[idx].pct[0];
    p2 = RECIPES[idx].pct[1];
    p3 = RECIPES[idx].pct[2];
    p4 = RECIPES[idx].pct[3];
  } else {
    // ถ้าไม่พบสูตร: default เป็นน้ำเปล่าที่ P2 ทั้งหมด
    p1 = 0; p2 = 100; p3 = 0; p4 = 0;
  }

  // ห้ามใช้ P1 ในการทำเบส
  p1 = 0;

  // normalize ให้แค่ P2..P4
  int sum = p2 + p3 + p4;
  if (sum <= 0) {
    Serial.println("[base] recipe sum <=0 → skip base dispensing");
    return;
  }

  unsigned long totalMs = baseTotalMsForSize(g_size);
  unsigned long t2 = (unsigned long)( ( (unsigned long)p2 * totalMs ) / sum );
  unsigned long t3 = (unsigned long)( ( (unsigned long)p3 * totalMs ) / sum );
  unsigned long t4 = (unsigned long)( ( (unsigned long)p4 * totalMs ) / sum );

  Serial.printf("[base] drink='%s' size='%s' total=%lums → P2=%lums P3=%lums P4=%lums (ratio %d:%d:%d)\n",
      g_drinkName.c_str(), g_size.c_str(), totalMs, t2, t3, t4, p2, p3, p4);

  // จ่ายแบบ sequential
  pulsePumpMs(PUMP2_PIN, t2);
  pulsePumpMs(PUMP3_PIN, t3);
  pulsePumpMs(PUMP4_PIN, t4);
}

/* =======================
 *  Helper: รอเซนเซอร์เป้าหมาย + timeout
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
    audioLoop();  // ให้เสียงทำงานตลอดการรอ

    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.printf("[seq] Timeout waiting S%d → STOP\n", targetSensor);
      servoStop();
      return false;
    }

    int w = whichSensor();
    if (w == targetSensor) {
      unsigned long pressedAt = millis();
      while ((whichSensor() == targetSensor) && (millis() - pressedAt < TCRT_DEBOUNCE_MS)) {
        audioLoop(); 
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

          // === เรียกเสียงตามสเต็ป ===
          if (targetSensor == 3 && reportStep && String(reportStep) == "adding_toppings") {
            audioPlay("/adding_topping.wav", 10);
            toppingAttachNeutral();
            toppingServo.write(TOPPING_MAX_DEG);
            audioDelay(TOPPING_PULSE_MS);
            toppingServo.write(TOPPING_MIN_DEG);
            audioDelay(100);
            toppingDetachIdle();
          }

          if (targetSensor == 4 && reportStep && String(reportStep) == "adding_ice") {
            audioPlay("/adding_ice.wav", 10);
            iceAttachNeutral();
            for (int i = 0; i < ICE_REPEAT_TIMES; ++i) {
              iceServo.write(ICE_MAX_DEG);
              audioDelay(ICE_UP_HOLD_MS);
              iceServo.write(ICE_MIN_DEG);
              audioDelay(ICE_DOWN_HOLD_MS);
            }
            iceDetachIdle();
          }

          if (targetSensor == 1 && reportStep && String(reportStep) == "brewing_drink") {
            audioPlay("/making_drink.wav", 10);

            // 1) จ่ายเบส
            dispenseBaseForCurrentOrder();

            // 2) จ่ายน้ำเชื่อมตามความหวาน
            unsigned long syrupMs = syrupMsFromSweetness(g_sweetness);
            if (syrupMs > 0) {
              Serial.printf("[syrup] sweetness=%d%% → P1 pulse=%lums\n", g_sweetness, syrupMs);
              pumpOn(PUMP1_PIN);
              audioDelay(syrupMs);
              pumpOff(PUMP1_PIN);
            } else {
              Serial.println("[syrup] sweetness=0% → skip syrup (no pulse)");
            }
          }

          if (stopMs > 0) audioDelay(stopMs);
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
    // ใช้ audioDelay เพื่อไม่ให้เสียงสะดุด ถ้าช่วงนี้มีการเล่นเสียง
    audioDelay(stepDelay);
  }
}

bool gotoAndPrepareAtS2(const String& orderId) {
  Serial.println("[prep] Seeking S2 ...");
  servoCCW();
  unsigned long tLastLog = 0;
  unsigned long tStart = millis();

  while (true) {
    audioLoop();

    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.println("[prep] Timeout seeking S2 → STOP");
      servoStop();
      return false;
    }

    int w = whichSensor();
    if (w == 2) {
      unsigned long hitAt = millis();
      while ((whichSensor() == 2) && (millis() - hitAt < TCRT_DEBOUNCE_MS)) { audioLoop(); delay(1); }
      if (millis() - hitAt >= TCRT_DEBOUNCE_MS) {
        servoStop();
        (void)postProgressExtended(orderId, "preparing_cup", "in_progress",
                                   "หยุดเพื่อเตรียมแก้ว (S2)", true, "marker_hit", 2);
        audioPlay("/precup.wav", 10);   // เล่นเสียงเตรียมแก้ว
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
    audioLoop();

    float nowAvg = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);

    if (isnan(nowAvg) || nowAvg >= PREP_NEAR_CM) {
      if (prep_pos < 180) {
        prep_pos += prep_stepSize;
        if (prep_pos > 180) prep_pos = 180;
        prepServosWriteBoth(prep_pos);
        audioDelay(prep_stepDelayMs);
      } else {
        int halfAmp = shakeAmplitude / 2;
        for (int i = 0; i < shakeTimes; i++) {
          prepServosWriteBoth(prep_pos - halfAmp);
          audioDelay(shakeDelay);
          prepServosWriteBoth(prep_pos + halfAmp);
          audioDelay(shakeDelay);
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
      Serial.printf("[prep] near %.1f cm detected → confirming...\n", nowAvg);
      audioDelay(PREP_CONFIRM_MS);
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
    audioLoop();

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
      audioPlay("/completed.wav", 10);   // เสียงงานเสร็จ
      Serial.println("[pickup] Cup taken → COMPLETED");
      servoStop();
      return;
    }

    audioDelay(80);
    yield();
  }
}

/* =======================
 *  Brew sequence
 * =======================*/
void brew(const String& orderId) {
  const unsigned long HOLD_MS = 2000;

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

  // ===== ตามท็อปปิ้ง =====
  if (g_needIce && g_needPearls) {
    waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false); // S3
    waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);      // S4
  } else if (g_needPearls) {
    waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);
  } else if (g_needIce) {
    waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);
  } else {
    Serial.println("[logic] No ice & no pearls → skip S3/S4");
  }

  // เติม “เบส” + “น้ำเชื่อม” (sweetness) ที่ S1
  unsigned long holdAfterBrew = (syrupMsFromSweetness(g_sweetness) > 0) ? HOLD_MS : 0;
  waitForSensorHitSequence(orderId, 1, holdAfterBrew, "brewing_drink", false);

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
  audioInit();  // เตรียมระบบเสียง/FS
  audioLoop();

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

  // ถาด (CR servo)
  cupServo.setPeriodHertz(50);
  cupServo.attach(SERVO_PIN, 500, 2500);
  servoStop(); // หยุดไว้ก่อน

  // Servos เตรียมแก้ว
  prepServo1.attach(PREP_PIN_1, 500, 2500);
  prepServo2.attach(PREP_PIN_2, 500, 2500);

  // topping/ice: ห้าม attach ตอนบูต
  pinMode(TOPPING_SERVO_PIN, OUTPUT);
  digitalWrite(TOPPING_SERVO_PIN, LOW);
  pinMode(ICE_SERVO_PIN, OUTPUT);
  digitalWrite(ICE_SERVO_PIN, LOW);

  // ปั๊มน้ำ P1..P4
  pinMode(PUMP1_PIN, OUTPUT); pumpOff(PUMP1_PIN);
  pinMode(PUMP2_PIN, OUTPUT); pumpOff(PUMP2_PIN);
  pinMode(PUMP3_PIN, OUTPUT); pumpOff(PUMP3_PIN);
  pinMode(PUMP4_PIN, OUTPUT); pumpOff(PUMP4_PIN);

  // ตำแหน่งเริ่มต้นของ G23/G22
  prep_pos = 0;
  prepServosWriteBoth(0);

  connectWiFi();

  // (ทางเลือก) เล่นจิงเกิลเบา ๆ ตอนบูต
  // audioPlay("/totosong_8k_adpcm.wav", 6);
}

void loop() {
  audioLoop();   // ให้เครื่องเสียงทำงานทุกเฟรม

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
    audioDelay(POLL_ERROR_MS);
    return;
  }

  if (orderId.length() > 0) {
    Serial.printf("[sim] Received order %s (#%d)\n", orderId.c_str(), queuePos);

    // (ทางเลือก) แจ้งเสียงเมื่อมีออเดอร์เข้า
    // audioPlay("/totosong_8k_adpcm.wav", 6);

    brew(orderId);
    Serial.printf("[sim] Completed order %s\n", orderId.c_str());
    servoStop();

    // ความปลอดภัย: แน่ใจว่า PUMPs/servos ไม่ถูกขับทิ้งไว้ผิดสถานะ
    toppingDetachIdle();
    iceDetachIdle();
    pumpOff(PUMP1_PIN);
    pumpOff(PUMP2_PIN);
    pumpOff(PUMP3_PIN);
    pumpOff(PUMP4_PIN);
  } else {
    servoStop();
    audioDelay(POLL_IDLE_MS);
  }
}
