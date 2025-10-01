/******************************************************
 * ESP32 Drink Robot + Audio Steps + LCD/RTC (EN LCD)
 * - Pumps: P1 syrup (GPIO33), P2 water (32), P3 water (12), P4 water (14)
 * - Steps: S2 (cup), S3 (topping), S4 (ice), S1 (brew), Completed
 * - Audio: LittleFS + I2S DAC (GPIO25) [MP3 version]
 * - LCD (0x27) + RTC DS3231 (0x68) via I2C SDA=21, SCL=5
 * - LCD shows (English):
 *   1) Date/Time   2) Drink name
 *   3) Toppings + Sweetness  4) Current step
 * - RTC time is shown; if API returns a server time, RTC will sync to it.
 ******************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ===== AUDIO / FS (MP3) =====
#include <Arduino.h>
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// ===== LCD / RTC / I2C =====
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

/* ===================== AUDIO (MP3) ===================== */
AudioGeneratorMP3* g_mp3 = nullptr;
AudioFileSourceLittleFS* g_file = nullptr;
AudioOutputI2S* g_out = nullptr;

// MP3 file map per step
static const char* SND_PREP        = "/precup.mp3";
static const char* SND_TOPPING     = "/adding_topping.mp3";
static const char* SND_ICE         = "/adding_ice.mp3";
static const char* SND_BREW        = "/making_drink.mp3";
static const char* SND_COMPLETED   = "/completed.mp3";

// Idle song (play once when idle)
static const char* SND_IDLE_COMPRESS = "/compress.mp3";

// idle state
bool g_idleSongActive = false;     // currently playing compress.mp3?
bool g_idleSongPlayedOnce = false; // already played one full round this idle period?

void setAudioVolume(uint8_t percent) {
  if (!g_out) return;
  percent = constrain(percent, 0, 30);   // recommended 0–30 for INTERNAL_DAC
  g_out->SetGain(percent / 100.0f);
}

void audioStop() {
  if (g_mp3) {
    if (g_mp3->isRunning()) g_mp3->stop();
    delete g_mp3; g_mp3 = nullptr;
  }
  if (g_file) {
    g_file->close();
    delete g_file; g_file = nullptr;
  }
}

void audioInit() {
  static bool inited = false;
  if (inited) return;
  LittleFS.begin(true);
  g_out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  g_out->SetPinout(25, 26, -1);     // DAC1=GPIO25 (เราใช้ช่องนี้), DAC2=GPIO26

  // INTERNAL_DAC uses DAC1=GPIO25; mono is fine for announcements
  g_out->SetOutputModeMono(true);
  setAudioVolume(30);
  inited = true;
                          // ดูใน Serial ว่ามีไฟล์ .mp3 ครบไหม

}

bool audioPlay(const char* path, uint8_t volPercent = 8) {
  audioInit();
  // stop previous
  audioStop();

  g_file = new AudioFileSourceLittleFS(path);
  if (!g_file) return false;

  g_mp3  = new AudioGeneratorMP3();
  if (!g_mp3) { delete g_file; g_file = nullptr; return false; }

  setAudioVolume(volPercent);
  if (!g_mp3->begin(g_file, g_out)) {
    audioStop();
    return false;
  }

  // if not idle song, reset idle state
  if (strcmp(path, SND_IDLE_COMPRESS) != 0) {
    g_idleSongActive = false;
    g_idleSongPlayedOnce = false;
  }
  return true;
}

void audioLoop() {
  if (g_mp3 && g_mp3->isRunning()) {
    if (!g_mp3->loop()) {
      // finished
      g_mp3->stop();
      if (g_idleSongActive) {
        g_idleSongActive = false;
        g_idleSongPlayedOnce = true; // mark played once
      }
      audioStop();
    }
  }
}

void audioDelay(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    audioLoop();
    delay(5);
    yield();
  }
}

void startIdleSongIfNeeded() {
  if (!g_idleSongPlayedOnce && !g_idleSongActive) {
    if (audioPlay(SND_IDLE_COMPRESS, 8)) {
      g_idleSongActive = true;
    }
  }
}

void resetIdleSong() {
  g_idleSongActive = false;
  g_idleSongPlayedOnce = false;
  audioStop();
}

/* ===================== SERVOS ===================== */
Servo cupServo;    // G13 (CR)
Servo prepServo1;  // G23
Servo prepServo2;  // G22
Servo toppingServo;// G2
Servo iceServo;    // G15

const int SERVO_PIN   = 13;
const int PREP_PIN_1  = 23;
const int PREP_PIN_2  = 22;
const int TOPPING_SERVO_PIN = 2;
const int ICE_SERVO_PIN     = 15;

const int SERVO_US_CCW  = 1600;
const int SERVO_US_STOP = 1500;
const int SERVO_US_CW   = 2000;
inline void servoStop(){ cupServo.writeMicroseconds(SERVO_US_STOP); }
inline void servoCCW(){  cupServo.writeMicroseconds(SERVO_US_CCW); }
inline void servoCW(){   cupServo.writeMicroseconds(SERVO_US_CW);  }

/* ===================== PUMPS ===================== */
// P1 syrup, P2..P4 water bases
const int  PUMP1_PIN = 33; // syrup
const int  PUMP2_PIN = 32; // base
const int  PUMP3_PIN = 12; // base
const int  PUMP4_PIN = 14; // base
const bool PUMP_ACTIVE_LEVEL = LOW;

const unsigned long SYRUP_PULSE_MAX_MS = 2000;
const unsigned long BASE_TOTAL_MS_SMALL   = 2200;
const unsigned long BASE_TOTAL_MS_REGULAR = 3000;
const unsigned long BASE_TOTAL_MS_LARGE   = 3800;

inline void pumpOn(int pin)  { digitalWrite(pin, PUMP_ACTIVE_LEVEL); }
inline void pumpOff(int pin) { digitalWrite(pin, !PUMP_ACTIVE_LEVEL); }

void pulsePumpMs(int pin, unsigned long ms) {
  if (ms == 0) return;
  pumpOn(pin);
  audioDelay(ms);
  pumpOff(pin);
}

/* ===================== SENSORS ===================== */
const int ULTRA1_TRIG = 16;
const int ULTRA1_ECHO = 17;
const int ULTRA2_TRIG = 19;
const int ULTRA2_ECHO = 18;

const float SOUND_CM_PER_US = 0.0343f;
const unsigned long ULTRA_TIMEOUT_US = 30000UL;
const float ULTRA1_THRESHOLD_CM = 11.5f;
const float ULTRA2_THRESHOLD_CM = 10.0f;

const float START_US1_SKIP_CM = 32.0f;

const float PREP_NEAR_CM = 5.0f;
const unsigned long PREP_CONFIRM_MS = 2000;

int prep_pos = 0;
const uint8_t  prep_stepSize = 10;
const unsigned long prep_stepDelayMs = 20;
int shakeAmplitude = 80;
int shakeDelay = 150;
int shakeTimes = 5;
const int PREP_HOME_DEG = 0;

const int  TCRT_S3 = 27;
const int  TCRT_S4 = 26;
const bool TCRT_ACTIVE_LOW = true;
const unsigned long TCRT_DEBOUNCE_MS = 30;

inline bool tcrtRaw(int pin) {
  int v = digitalRead(pin);
  return TCRT_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

/* ===================== NET / API ===================== */
const char* ssid        = "totoiot";
const char* password    = "123456789";
const char* BASE_URL    = "https://porametix.online";
const char* HARDWARE_ID = "esp32-001";
const char* API_KEY     = "odroid-hardware-key-1758367749";

const unsigned long POLL_IDLE_MS   = 2000;
const unsigned long POLL_ERROR_MS  = 5000;
const unsigned long LOG_EVERY_MS   = 250;
const unsigned long FIND_TIMEOUT_MS = 30000;

WiFiClientSecure secureClient;
HTTPClient http;

/* ===================== ORDER STATE ===================== */
String g_orderId;
String g_drinkName;     // e.g., "green tea"
String g_size = "Regular";
bool   g_needIce = false;
bool   g_needPearls = false;
int    g_sweetness = 100; // 0..100
String g_currentStep = "";    // to show on LCD
String g_orderDetail = "";    // "Top: ... | Sweet: ..."

/* ===================== LCD & RTC ===================== */
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS3231 rtc;
bool g_rtc_ok = false;
bool g_rtc_synced_from_api = false;

String nowStr() {
  if (!g_rtc_ok) return "--/-- --:--";
  DateTime now = rtc.now();
  char buf[17];
  // DD/MM HH:MM
  snprintf(buf, sizeof(buf), "%02d/%02d %02d:%02d",
           now.day(), now.month(), now.hour(), now.minute());
  return String(buf);
}

void lcdClearRow(uint8_t row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < 20; ++i) lcd.print(' ');
  lcd.setCursor(0, row);
}

void lcdPrintFit(uint8_t row, const String& s) {
  lcdClearRow(row);
  if (s.length() <= 20) { lcd.print(s); return; }
  lcd.print(s.substring(0, 20));
}

String toppingsStrEN() {
  String t = "";
  if (g_needPearls) t += "Pearls";
  if (g_needIce) {
    if (t.length()) t += " ";
    t += "Ice";
  }
  if (t.length() == 0) t = "-";
  return t;
}

void lcdShow(const String& stepLabelEN) {
  lcd.clear();
  // 1) Date/Time
  lcd.setCursor(0,0); lcd.print(nowStr());
  // 2) Drink name (+size)
  String line1 = g_drinkName.length()? g_drinkName : "-waiting-";
  if (g_size.length()) line1 += " (" + g_size + ")";
  lcdPrintFit(1, line1);
  // 3) Toppings + Sweetness
  lcd.setCursor(0,2);
  String line2 = "Toppings: " + toppingsStrEN() + "  Sweet: " + String(g_sweetness) + "%";
  lcdPrintFit(2, line2);
  // 4) Step
  lcdPrintFit(3, "Step: " + stepLabelEN);
}

void setStepAndLcd(const String& stepLabelEN) {
  g_currentStep = stepLabelEN;
  lcdShow(g_currentStep);
}

/* ===================== RECIPES ===================== */
/* P1 = syrup (always 0 in recipes), P2..P4 = base waters */
struct Recipe { const char* name; uint8_t pct[4]; /* P1..P4 */ };
Recipe RECIPES[] = {
  {"thai tea",   {0,100,  0,  0}}, {"ชาไทย",     {0,100,  0,  0}},
  {"green tea",  {0,  0,100,  0}}, {"ชาเขียว",   {0,  0,100,  0}},
  {"coffee",     {0, 50, 50,  0}}, {"กาแฟ",      {0, 50, 50,  0}},
  {"fruit tea",  {0,  0,  0,100}}, {"ชาผลไม้",   {0,  0,  0,100}},
  {"cocoa",      {0,  0,100,  0}}, {"โกโก้",      {0,  0,100,  0}},
  {"milk tea",   {0, 60, 40,  0}}, {"ชานม",       {0, 60, 40,  0}},
};

int findRecipeIndex(const String& drink) {
  String q = drink; q.toLowerCase();
  for (size_t i = 0; i < sizeof(RECIPES)/sizeof(RECIPES[0]); ++i) {
    String nm = RECIPES[i].name; nm.toLowerCase();
    if (nm == q) return (int)i;
  }
  return -1;
}

unsigned long baseTotalMsForSize(const String& sizeStr) {
  String s = sizeStr; s.toLowerCase();
  if (s.indexOf("small") >= 0) return BASE_TOTAL_MS_SMALL;
  if (s.indexOf("large") >= 0) return BASE_TOTAL_MS_LARGE;
  return BASE_TOTAL_MS_REGULAR;
}

unsigned long syrupMsFromSweetness(int sweetness) {
  if (sweetness < 0) sweetness = 0;
  if (sweetness > 100) sweetness = 100;
  return (unsigned long)((sweetness * SYRUP_PULSE_MAX_MS) / 100);
}

/* ===================== HTTP HELPERS ===================== */
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

bool httpGetJson(const String& url, String& payloadOut, int& codeOut) {
  if (!httpBeginWithCommon(url)) {
    Serial.println("[sim] http.begin failed (GET)");
    return false;
  }
  http.addHeader("X-API-Key", API_KEY);
  int code = http.GET();
  codeOut = code;
  if (code <= 0) {
    Serial.printf("[sim] GET failed: %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    return false;
  }
  payloadOut = http.getString();
  http.end();
  return (code == HTTP_CODE_OK);
}

/* Try to sync RTC from any time field in API once per boot */
void trySyncRtcFromApi(const DynamicJsonDocument& doc) {
  if (!g_rtc_ok || g_rtc_synced_from_api) return;

  const char* keys[] = {"serverTime", "server_time", "time", "now"};
  String iso;
  for (auto k : keys) {
    if (doc[k].is<const char*>()) { iso = String(doc[k].as<const char*>()); break; }
    if (doc["order"][k].is<const char*>()) { iso = String(doc["order"][k].as<const char*>()); break; }
  }
  if (iso.length() == 0) return;

  // RTClib DateTime can parse common ISO8601 like "2025-01-02T03:04:05"
  DateTime dt(iso.c_str());
  if (dt.year() >= 2020 && dt.year() <= 2099) {
    rtc.adjust(dt);
    g_rtc_synced_from_api = true;
    Serial.printf("[rtc] Synced from API: %04d-%02d-%02d %02d:%02d:%02d\n",
                  dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  } else {
    Serial.println("[rtc] API time present but could not parse/invalid.");
  }
}

/* ===================== ORDER POLLING ===================== */
bool isIceWord(const String& s)   { String t=s; t.toLowerCase(); return (t.indexOf("ice")>=0) || (t.indexOf("น้ำแข็ง")>=0); }
bool isPearlWord(const String& s) { String t=s; t.toLowerCase(); return (t.indexOf("pearl")>=0) || (t.indexOf("boba")>=0) || (t.indexOf("tapioca")>=0) || (t.indexOf("ไข่มุก")>=0); }

bool pollNextOrder(String& orderId, int& queuePos) {
  String url = String(BASE_URL) + "/api/hardware/orders?hardwareId=" + HARDWARE_ID;
  String payload; int code;
  bool ok = httpGetJson(url, payload, code);
  if (!ok) return false;

  Serial.printf("[sim] pollNextOrder HTTP %d\n", code);
  Serial.printf("[sim] Poll response: %s\n", payload.c_str());

  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[sim] JSON parse error (orders): %s\n", err.c_str());
    return false;
  }

  // Try syncing RTC from API (once)
  trySyncRtcFromApi(doc);

  orderId      = "";
  queuePos     = -1;
  g_needIce    = false;
  g_needPearls = false;
  g_sweetness  = 100;
  g_drinkName  = "";
  g_size       = "Regular";

  JsonObject ord = doc["order"];
  if (!ord.isNull()) {
    if (ord["orderId"].is<const char*>()) orderId  = String(ord["orderId"].as<const char*>());
    queuePos = ord["queuePosition"] | -1;

    if (ord["drinkName"].is<const char*>()) g_drinkName = String(ord["drinkName"].as<const char*>());
    if (ord["size"].is<const char*>())      g_size      = String(ord["size"].as<const char*>());

    if (ord["toppings"].is<JsonArray>()) {
      for (JsonVariant v : ord["toppings"].as<JsonArray>()) {
        if (v.is<const char*>()) {
          String item = v.as<const char*>();
          if (isIceWord(item))    g_needIce = true;
          if (isPearlWord(item))  g_needPearls = true;
        }
      }
    }
    if (ord["sweetness"].is<int>()) g_sweetness = ord["sweetness"].as<int>();
  }

  g_orderId = orderId;

  // Prepare LCD order detail in English
  String tops = toppingsStrEN();
  g_orderDetail = "Toppings:" + tops + "  Sweet:" + String(g_sweetness) + "%";

  // Refresh LCD snapshot
  lcdShow(g_currentStep.length()? g_currentStep : String("idle"));

  return true;
}

/* ===================== ULTRASONIC HELPERS ===================== */
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
  float sum = 0.0f; int cnt = 0;
  for (int i = 0; i < samples; ++i) {
    float v = readUltrasonicCm(trigPin, echoPin);
    if (!isnan(v)) { sum += v; cnt++; }
    audioDelay(gapMs);
  }
  if (cnt == 0) return NAN;
  return sum / cnt;
}

bool ultrasonicTriggered(int trigPin, int echoPin, float thresholdCm) {
  float cm = readUltrasonicCm(trigPin, echoPin);
  if (isnan(cm)) return false;
  return (cm <= thresholdCm);
}

int whichSensor() {
  if (ultrasonicTriggered(ULTRA1_TRIG, ULTRA1_ECHO, ULTRA1_THRESHOLD_CM)) return 1;
  if (ultrasonicTriggered(ULTRA2_TRIG, ULTRA2_ECHO, ULTRA2_THRESHOLD_CM)) return 2;
  if (tcrtRaw(TCRT_S3)) return 3;
  if (tcrtRaw(TCRT_S4)) return 4;
  return 0;
}

/* ===================== PROGRESS API ===================== */
bool postProgressExtended(const String& orderId,
                          const String& step,
                          const String& status,
                          const String& message,
                          bool tcrtHit,
                          const char* eventTag,
                          int sensorIndex) {
  // map to English LCD step keywords
  String stepEN = step;
  if (stepEN == "preparing_cup") stepEN = "preparing";
  else if (stepEN == "adding_toppings") stepEN = "toppings";
  else if (stepEN == "adding_ice") stepEN = "ice";
  else if (stepEN == "brewing_drink") stepEN = "brewing";
  else if (stepEN == "completed") stepEN = "completed";
  setStepAndLcd(stepEN);

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

/* ===================== TOPPING & ICE SERVO HELPERS ===================== */
const int TOPPING_MIN_DEG = 0;
const int TOPPING_MAX_DEG = 45;
const unsigned long TOPPING_PULSE_MS = 300;

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

const int ICE_MIN_DEG = 0;
const int ICE_MAX_DEG = 45;
const unsigned long ICE_UP_HOLD_MS   = 300;
const unsigned long ICE_DOWN_HOLD_MS = 200;
const int ICE_REPEAT_TIMES = 3;

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

/* ===================== BASE DISPENSER ===================== */
void dispenseBaseForCurrentOrder() {
  int idx = findRecipeIndex(g_drinkName);
  uint8_t p1=0, p2=0, p3=0, p4=0;
  if (idx >= 0) {
    p1 = RECIPES[idx].pct[0];
    p2 = RECIPES[idx].pct[1];
    p3 = RECIPES[idx].pct[2];
    p4 = RECIPES[idx].pct[3];
  } else {
    p1 = 0; p2 = 100; p3 = 0; p4 = 0; // default to P2
  }
  p1 = 0; // never use P1 for base

  int sum = p2+p3+p4;
  if (sum <= 0) {
    Serial.println("[base] recipe sum <=0 → skip base dispensing");
    return;
  }

  unsigned long totalMs = baseTotalMsForSize(g_size);
  unsigned long t2 = ((unsigned long)p2 * totalMs) / sum;
  unsigned long t3 = ((unsigned long)p3 * totalMs) / sum;
  unsigned long t4 = ((unsigned long)p4 * totalMs) / sum;

  Serial.printf("[base] drink='%s' size='%s' total=%lums → P2=%lums P3=%lums P4=%lums (ratio %d:%d:%d)\n",
      g_drinkName.c_str(), g_size.c_str(), totalMs, t2, t3, t4, p2, p3, p4);

  pulsePumpMs(PUMP2_PIN, t2);
  pulsePumpMs(PUMP3_PIN, t3);
  pulsePumpMs(PUMP4_PIN, t4);
}

/* ===================== PREP @ S2 ===================== */
void prepServosWriteBoth(int deg) { prepServo1.write(deg); prepServo2.write(deg); }

void prepServosMoveTo(int targetDeg, int step = 10, unsigned long stepDelay = 20) {
  targetDeg = constrain(targetDeg, 0, 180);
  while (prep_pos != targetDeg) {
    if (prep_pos < targetDeg) prep_pos += step;
    else                      prep_pos -= step;
    prep_pos = constrain(prep_pos, 0, 180);
    prepServosWriteBoth(prep_pos);
    audioDelay(stepDelay);
  }
}

bool gotoAndPrepareAtS2(const String& orderId) {
  Serial.println("[prep] Seeking S2 ...");
  servoCCW();
  unsigned long tLastLog = 0, tStart = millis();

  while (true) {
    audioLoop();
    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.println("[prep] Timeout seeking S2 → STOP");
      servoStop(); return false;
    }
    int w = whichSensor();
    if (w == 2) {
      unsigned long hitAt = millis();
      while ((whichSensor() == 2) && (millis() - hitAt < TCRT_DEBOUNCE_MS)) { audioLoop(); delay(1); }
      if (millis() - hitAt >= TCRT_DEBOUNCE_MS) {
        servoStop();
        (void)postProgressExtended(orderId, "preparing_cup", "in_progress",
                                   "stop at S2 to prepare cup", true, "marker_hit", 2);
        audioPlay(SND_PREP, 10);     // MP3
        setStepAndLcd("preparing");
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
    delay(1); yield();
  }

  // wait cup near
  prep_pos = 0; prepServosWriteBoth(prep_pos);
  Serial.printf("[prep] Waiting ULTRA2 < %.1f cm then hold %lu ms\n", PREP_NEAR_CM, PREP_CONFIRM_MS);

  while (true) {
    audioLoop();
    float nowAvg = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);

    if (isnan(nowAvg) || nowAvg >= PREP_NEAR_CM) {
      if (prep_pos < 180) {
        prep_pos += prep_stepSize; prep_pos = min(prep_pos, 180);
        prepServosWriteBoth(prep_pos);
        audioDelay(prep_stepDelayMs);
      } else {
        int halfAmp = shakeAmplitude / 2;
        for (int i = 0; i < shakeTimes; i++) {
          prepServosWriteBoth(prep_pos - halfAmp); audioDelay(shakeDelay);
          prepServosWriteBoth(prep_pos + halfAmp); audioDelay(shakeDelay);
        }
        prepServosWriteBoth(prep_pos);
      }
      static unsigned long lastLog = 0;
      if (millis() - lastLog >= 300) {
        lastLog = millis();
        Serial.printf("[prep] ULTRA2= %s cm (need < %.1f)  pos=%d\n",
                      isnan(nowAvg) ? "NaN" : String(nowAvg,1).c_str(), PREP_NEAR_CM, prep_pos);
      }
    } else {
      Serial.printf("[prep] near %.1f cm detected → confirming...\n", nowAvg);
      audioDelay(PREP_CONFIRM_MS);
      float confirm = readUltrasonicAverageCm(ULTRA2_TRIG, ULTRA2_ECHO, 4, 25);
      if (!isnan(confirm) && confirm < PREP_NEAR_CM) {
        Serial.println("[prep] confirmed cup present → homing prep servos...");
        prepServosMoveTo(PREP_HOME_DEG, 10, 20);
        Serial.println("[prep] homed → continue");
        servoCCW(); return true;
      } else {
        Serial.println("[prep] confirm failed, keep working...");
      }
    }
    delay(10); yield();
  }
}

/* ===================== POST-BREW PICKUP ===================== */
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
      Serial.printf("[pickup] base=%s cm, now=%s cm, Δ=%s cm, need≥%.1f\n",
        isnan(baseline) ? "NaN" : String(baseline,1).c_str(),
        isnan(nowAvg) ? "NaN" : String(nowAvg,1).c_str(),
        (isnan(baseline)||isnan(nowAvg)) ? "NaN" : String(nowAvg - baseline,1).c_str(),
        pickupDeltaCm);
    }

    if (picked) {
      (void)postProgressExtended(orderId, "completed", "completed",
                                 "customer took the cup (ULTRA1 +4cm)", false, "cup_taken", 1);
      audioPlay(SND_COMPLETED, 10); // MP3
      setStepAndLcd("completed");   // FINAL step shown as "completed"
      Serial.println("[pickup] Cup taken → COMPLETED");
      servoStop();
      return;
    }
    audioDelay(80); yield();
  }
}

/* ===================== SEQ HELPER: waitForSensorHitSequence ===================== */
bool waitForSensorHitSequence(const String& orderId,
                              int targetSensor,
                              unsigned long stopMs,
                              const char* reportStep,
                              bool skipOnly) {
  Serial.printf("[seq] Waiting for S%d ...\n", targetSensor);
  servoCCW();
  unsigned long tLastLog = 0, tStart = millis();

  while (true) {
    audioLoop();
    if (millis() - tStart > FIND_TIMEOUT_MS) {
      Serial.printf("[seq] Timeout waiting S%d → STOP\n", targetSensor);
      servoStop(); return false;
    }

    int w = whichSensor();
    if (w == targetSensor) {
      unsigned long pressedAt = millis();
      while ((whichSensor() == targetSensor) && (millis() - pressedAt < TCRT_DEBOUNCE_MS)) { audioLoop(); delay(1); }
      if (millis() - pressedAt >= TCRT_DEBOUNCE_MS) {
        if (skipOnly) {
          Serial.printf("[seq] S%d hit → SKIP (keep spinning)\n", targetSensor);
          return true;
        } else {
          Serial.printf("[seq] S%d hit → STOP %lums + report '%s'\n", targetSensor, stopMs, reportStep);
          servoStop();
          (void)postProgressExtended(orderId, reportStep, "in_progress",
                                     String(reportStep) + " (sensor S" + targetSensor + ")", true, "marker_hit", targetSensor);

          if (targetSensor == 3 && String(reportStep) == "adding_toppings") {
            audioPlay(SND_TOPPING, 10); // MP3
            setStepAndLcd("toppings");
            toppingAttachNeutral();
            toppingServo.write(TOPPING_MAX_DEG);
            audioDelay(TOPPING_PULSE_MS);
            toppingServo.write(TOPPING_MIN_DEG);
            audioDelay(100);
            toppingDetachIdle();
          }

          if (targetSensor == 4 && String(reportStep) == "adding_ice") {
            audioPlay(SND_ICE, 10); // MP3
            setStepAndLcd("ice");
            iceAttachNeutral();
            for (int i = 0; i < ICE_REPEAT_TIMES; ++i) {
              iceServo.write(ICE_MAX_DEG);  audioDelay(ICE_UP_HOLD_MS);
              iceServo.write(ICE_MIN_DEG);  audioDelay(ICE_DOWN_HOLD_MS);
            }
            iceDetachIdle();
          }

          if (targetSensor == 1 && String(reportStep) == "brewing_drink") {
            audioPlay(SND_BREW, 10); // MP3
            setStepAndLcd("brewing");
            // 1) base
            dispenseBaseForCurrentOrder();
            // 2) syrup
            unsigned long syrupMs = syrupMsFromSweetness(g_sweetness);
            if (syrupMs > 0) {
              Serial.printf("[syrup] sweetness=%d%% → P1 pulse=%lums\n", g_sweetness, syrupMs);
              pumpOn(PUMP1_PIN); audioDelay(syrupMs); pumpOff(PUMP1_PIN);
            } else {
              Serial.println("[syrup] sweetness=0% → skip syrup");
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
                    isnan(cm1) ? "NaN" : String(cm1,1).c_str(),
                    isnan(cm2) ? "NaN" : String(cm2,1).c_str(),
                    tcrtRaw(TCRT_S3)?1:0, tcrtRaw(TCRT_S4)?1:0);
    }
    delay(1); yield();
  }
}

/* ===================== BREW SEQUENCE ===================== */
void brew(const String& orderId) {
  const unsigned long HOLD_MS = 2000;

  // Start: S1 check
  float us1Start = readUltrasonicAverageCm(ULTRA1_TRIG, ULTRA1_ECHO, 3, 25);
  Serial.printf("[start] US1 start = %s cm\n", isnan(us1Start) ? "NaN" : String(us1Start,1).c_str());

  if (!isnan(us1Start) && us1Start < START_US1_SKIP_CM) {
    Serial.println("[start] US1 < 30cm → skip S1, go to S2");
    if (!gotoAndPrepareAtS2(orderId)) { Serial.println("[start] Could not reach S2 in time."); servoStop(); return; }
  } else {
    if (!waitForSensorHitSequence(orderId, 1, 0, "", true)) {
      Serial.println("[start] S1 wait timeout → continue to S2 anyway");
    }
    if (!gotoAndPrepareAtS2(orderId)) { Serial.println("[start] Could not reach S2 in time."); servoStop(); return; }
  }

  // Toppings/Ice
  if (g_needIce && g_needPearls) {
    waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);
    waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);
  } else if (g_needPearls) {
    waitForSensorHitSequence(orderId, 3, HOLD_MS, "adding_toppings", false);
  } else if (g_needIce) {
    waitForSensorHitSequence(orderId, 4, HOLD_MS, "adding_ice", false);
  } else {
    Serial.println("[logic] No ice & no pearls → skip S3/S4");
  }

  // Brew @ S1 (base + syrup)
  unsigned long holdAfterBrew = (syrupMsFromSweetness(g_sweetness) > 0) ? HOLD_MS : 0;
  waitForSensorHitSequence(orderId, 1, holdAfterBrew, "brewing_drink", false);

  // Wait pickup -> will set FINAL step to "completed"
  waitForPickupAfterBrew(orderId, 4.0f);

  servoStop();
  Serial.println("[seq] Sequence completed → STOP");
}

/* ===================== SETUP / LOOP ===================== */
void setup() {
  Serial.begin(115200);
  audioInit();
  audioLoop();

  // I2C (RTC + LCD) : SDA=21, SCL=5
  Wire.begin(21, 5);

  // RTC begin
  g_rtc_ok = rtc.begin();
  if (!g_rtc_ok) {
    Serial.println("[lcd] RTC DS3231 not found at 0x68");
  } else {
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // LCD init
  lcd.init();
  lcd.backlight();
  lcdPrintFit(0, "ESP32 Drink Robot");
  lcdPrintFit(1, "WiFi connecting...");
  lcdPrintFit(2, "");
  lcdPrintFit(3, "");

  // Ultrasonic pins
  pinMode(ULTRA1_TRIG, OUTPUT); pinMode(ULTRA1_ECHO, INPUT);
  pinMode(ULTRA2_TRIG, OUTPUT); pinMode(ULTRA2_ECHO, INPUT);
  digitalWrite(ULTRA1_TRIG, LOW);
  digitalWrite(ULTRA2_TRIG, LOW);

  // TCRT pins
  pinMode(TCRT_S3, INPUT_PULLUP);
  pinMode(TCRT_S4, INPUT_PULLUP);

  // Cup servo
  cupServo.setPeriodHertz(50);
  cupServo.attach(SERVO_PIN, 500, 2500);
  servoStop();

  // Prep servos
  prepServo1.attach(PREP_PIN_1, 500, 2500);
  prepServo2.attach(PREP_PIN_2, 500, 2500);

  // topping/ice pins idle
  pinMode(TOPPING_SERVO_PIN, OUTPUT); digitalWrite(TOPPING_SERVO_PIN, LOW);
  pinMode(ICE_SERVO_PIN, OUTPUT);     digitalWrite(ICE_SERVO_PIN, LOW);

  // Pumps
  pinMode(PUMP1_PIN, OUTPUT); pumpOff(PUMP1_PIN);
  pinMode(PUMP2_PIN, OUTPUT); pumpOff(PUMP2_PIN);
  pinMode(PUMP3_PIN, OUTPUT); pumpOff(PUMP3_PIN);
  pinMode(PUMP4_PIN, OUTPUT); pumpOff(PUMP4_PIN);

  // Prep servos home
  prep_pos = 0; prepServosWriteBoth(0);

  connectWiFi();

  // Initial LCD snapshot (English)
  setStepAndLcd("completed");

  // idle music flags
  g_idleSongActive = false;
  g_idleSongPlayedOnce = false;
}

void loop() {
  audioLoop();

  // Update clock line every ~1s
  static unsigned long lastClock = 0;
  if (millis() - lastClock > 1000) {
    lastClock = millis();
    lcd.setCursor(0,0);
    String t = nowStr();
    lcd.print(t);
    int pad = 20 - t.length();
    while (pad-- > 0) lcd.print(' ');
  }

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

    // idle case: try to start compress.mp3 once
    startIdleSongIfNeeded();

    audioDelay(POLL_ERROR_MS);
    return;
  }

  if (orderId.length() > 0) {
    // got order → cut idle music and reset flags
    resetIdleSong();

    Serial.printf("[sim] Received order %s (#%d)\n", orderId.c_str(), queuePos);
    setStepAndLcd("queued");

    // optional chime here if needed
    // audioPlay("/totosong_8k_adpcm.mp3", 6);

    brew(orderId);
    Serial.printf("[sim] Completed order %s\n", orderId.c_str());
    servoStop();

    // Safety off
    toppingDetachIdle();
    iceDetachIdle();
    pumpOff(PUMP1_PIN);
    pumpOff(PUMP2_PIN);
    pumpOff(PUMP3_PIN);
    pumpOff(PUMP4_PIN);

    // Keep LCD showing "completed" as the last step.
  } else {
    // no order: play compress.mp3 exactly once then remain silent
    startIdleSongIfNeeded();

    servoStop();
    audioDelay(POLL_IDLE_MS);
  }
}
