#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

/* =======================
 * เซตฮาร์ดแวร์ / พิน
 * =======================*/
Servo cupServo;           // servo หลัก G13
Servo toppingServo;       // servo ท็อปปิ้ง G15  
Servo iceServo;          // servo น้ำแข็ง G05

const int SERVO_PIN = 13;      // PWM servo (ช่องพัลส์เซอร์โว)
const int TOPPING_SERVO_PIN = 15;  // servo สำหรับท็อปปิ้ง
const int ICE_SERVO_PIN = 5;       // servo สำหรับน้ำแข็ง
const int RELAY_PIN = 2;           // เปลี่ยนจาก G34 เป็น G2 (relay สำหรับเติมน้ำ active low)

// เซอร์โวหมุนรอบ (continuous rotation) — ปรับได้ตามรุ่นของคุณ
const int SERVO_US_CCW = 1800;    // ทวนเข็ม (ซ้าย) >1500 = วิ่งทางเดียว, <1500 = อีกทาง
const int SERVO_US_STOP = 1500;   // หยุด (กลาง)
const int SERVO_US_CW = 2000;     // ตามเข็ม (ขวา)

inline void servoStop() {
  cupServo.writeMicroseconds(SERVO_US_STOP);
}

inline void servoCCW() {
  cupServo.writeMicroseconds(SERVO_US_CCW);
}

inline void servoCW() {
  cupServo.writeMicroseconds(SERVO_US_CW);
}

// ฟังก์ชันสำหรับ servo ท็อปปิ้ง (หมุน 180-0 1 ครั้ง)
void toppingServoAction() {
  Serial.println("[topping] Moving servo 180-0 (1 time)");
  toppingServo.write(180);  // หมุนไป 180 องศา
  delay(1000);              // รอให้หมุนเสร็จ
  toppingServo.write(0);    // หมุนกลับ 0 องศา  
  delay(1000);              // รอให้หมุนเสร็จ
  Serial.println("[topping] Servo action completed");
}

// ฟังก์ชันสำหรับ servo น้ำแข็ง (หมุน 180-0 3 ครั้ง)
void iceServoAction() {
  Serial.println("[ice] Moving servo 180-0 (3 times)");
  for (int i = 0; i < 3; i++) {
    Serial.printf("[ice] Cycle %d/3\n", i + 1);
    iceServo.write(180);    // หมุนไป 180 องศา
    delay(1000);            // รอให้หมุนเสร็จ
    iceServo.write(0);      // หมุนกลับ 0 องศา
    delay(1000);            // รอให้หมุนเสร็จ
  }
  Serial.println("[ice] Servo action completed");
}

// ฟังก์ชันสำหรับเติมน้ำ (relay active low 5 วินาที)
void fillWaterAction() {
  Serial.println("[water] Starting water fill - relay ON (active low)");
  Serial.printf("[water] Relay pin G%d: HIGH->LOW\n", RELAY_PIN);
  digitalWrite(RELAY_PIN, LOW);   // เปิด relay (active low)
  Serial.printf("[water] Relay state: %d\n", digitalRead(RELAY_PIN));
  
  delay(5000);                    // รอ 5 วินาที
  
  digitalWrite(RELAY_PIN, HIGH);  // ปิด relay
  Serial.printf("[water] Relay pin G%d: LOW->HIGH\n", RELAY_PIN);
  Serial.printf("[water] Relay state: %d\n", digitalRead(RELAY_PIN));
  Serial.println("[water] Water fill completed - relay OFF");
}

/* =======================
 * TCRT5000 — เพิ่มตัวที่ 2 ที่ GPIO0 (G0)
 * =======================*/
// S1 = ตัวเดิม (GPIO4) — แนะนำ
// S2 = ตัวใหม่ (GPIO0) — ระวัง: เป็นขา strap, ต้อง HIGH ตอนบูต
const int TCRT1_PIN = 4;
const int TCRT2_PIN = 0;
const bool TCRT_ACTIVE_LOW = true;  // ส่วนมาก DO เป็น Active-Low
const unsigned long TCRT_DEBOUNCE_MS = 30;

// อ่านสถานะดิบของแต่ละเซนเซอร์
inline bool tcrt1Raw() {
  int v = digitalRead(TCRT1_PIN);
  return TCRT_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

inline bool tcrt2Raw() {
  int v = digitalRead(TCRT2_PIN);
  return TCRT_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

// true ถ้า "เซนเซอร์ใดก็ได้" ถูกทริกเกอร์ พร้อมระบุว่าเซนเซอร์ไหน (1 หรือ 2)
inline bool anySensorTriggered(int &which) {
  if (tcrt1Raw()) {
    which = 1;
    return true;
  }
  if (tcrt2Raw()) {
    which = 2;
    return true;
  }
  which = 0;
  return false;
}

/* =======================
 * Wi-Fi / API config
 * =======================*/
const char* ssid = "totoiot";
const char* password = "123456789";
const char* BASE_URL = "https://porametix.online";
const char* HARDWARE_ID = "esp32-001";
const char* API_KEY = "odroid-hardware-key-1758367749";

/* =======================
 * Timing
 * =======================*/
const unsigned long POLL_IDLE_MS = 2000;
const unsigned long POLL_ERROR_MS = 5000;
const unsigned long LOG_EVERY_MS = 250;

/* =======================
 * HTTP helpers
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
  Serial.printf("\nWiFi connected: %s IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

bool httpBeginWithCommon(const String& url) {
  secureClient.setInsecure();  // DEV: ข้าม cert; โปรดักชันควร setCACert()
  http.setTimeout(15000);
  return http.begin(secureClient, url);
}

bool postJson(const String& url, const String& json, const char* extraHeaderKey = nullptr, const char* extraHeaderVal = nullptr, int* outStatus = nullptr, String* outResp = nullptr) {
  if (!httpBeginWithCommon(url)) {
    Serial.println("[sim] http.begin failed (post)");
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  if (extraHeaderKey && extraHeaderVal)
    http.addHeader(extraHeaderKey, extraHeaderVal);

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
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

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
    orderId = String(doc["order"]["orderId"].as<const char*>());
    queuePos = doc["order"]["queuePosition"] | -1;
    return true;
  }
  orderId = "";
  queuePos = -1;
  return true;
}

/* ส่ง progress เฉพาะตอน "เจอเซนเซอร์" เท่านั้น */
bool postProgressExtended(const String& orderId, const String& step, const String& status, const String& message, bool tcrtHit, const char* eventTag, int sensorIndex /*1=GPIO4, 2=GPIO0, 0=unknown*/) {
  {
    // main
    DynamicJsonDocument doc(2048);
    doc["orderId"] = orderId;
    doc["hardwareId"] = HARDWARE_ID;
    doc["step"] = step;
    doc["status"] = status;
    doc["message"] = message;
    doc["tcrtHit"] = tcrtHit;
    if (eventTag) doc["event"] = eventTag;
    if (sensorIndex > 0) doc["sensor"] = sensorIndex;  // เพิ่มบอกว่า S1/S2

    String body;
    serializeJson(doc, body);
    String url = String(BASE_URL) + "/api/hardware/progress";
    int httpStatus = 0;
    String resp;
    bool ok = postJson(url, body, nullptr, nullptr, &httpStatus, &resp);
    if (ok) return true;
    Serial.printf("[sim] /progress failed (HTTP %d) → fallback /status\n", httpStatus);
  }

  {
    // fallback
    DynamicJsonDocument doc(2048);
    doc["orderId"] = orderId;
    doc["status"] = (step == "completed") ? "completed" : "brewing";
    doc["step"] = step;
    doc["message"] = message;
    doc["hardwareId"] = HARDWARE_ID;
    doc["tcrtHit"] = tcrtHit;
    if (eventTag) doc["event"] = eventTag;
    if (sensorIndex > 0) doc["sensor"] = sensorIndex;

    JsonObject led = doc.createNestedObject("ledState");
    led["preparing"] = (step == "preparing_cup");
    led["toppings"] = (step == "adding_toppings");
    led["ice"] = (step == "adding_ice");
    led["brewing"] = (step == "brewing_drink");
    led["completed"] = (step == "completed");
    doc["progress"] = 0;

    String body;
    serializeJson(doc, body);
    String url = String(BASE_URL) + "/api/hardware/status";
    int httpStatus = 0;
    String resp;
    bool ok = postJson(url, body, "X-API-Key", API_KEY, &httpStatus, &resp);
    if (!ok) Serial.printf("[sim] Fallback /status failed (HTTP %d)\n", httpStatus);
    return ok;
  }
}

/* =======================
 * Logic: หมุนต่อเนื่องจนเจอ TCRT
 * - หมุน CCW ตลอด
 * - เจอเซนเซอร์ใดก็ได้ → หยุด 3 วิ → หมุนต่อทันที
 * - คืนค่า true + ระบุ sensorIndex (1 หรือ 2)
 * =======================*/
bool spinUntilMarkerCCW(bool &tcrtHit, int &sensorIndex, unsigned long logEveryMs = LOG_EVERY_MS) {
  Serial.println("[servo] CCW until ANY TCRT marker...");
  tcrtHit = false;
  sensorIndex = 0;

  servoCCW();  // เริ่มหมุนทันที
  unsigned long tStart = millis();
  unsigned long tLastLog = 0;

  while (true) {
    int which = 0;
    bool trigAny = anySensorTriggered(which);
    if (trigAny) {
      // debounce เฉพาะตัวที่เจอ
      unsigned long pressedAt = millis();
      while (true) {
        int w2 = 0;
        bool stillTrig = anySensorTriggered(w2);
        if (!stillTrig || w2 != which || (millis() - pressedAt >= TCRT_DEBOUNCE_MS))
          break;
        delay(1);
      }

      int w3 = 0;
      if (anySensorTriggered(w3) && w3 == which) {
        // ยืนยัน hit - หยุดทันทีไม่หมุนต่อ
        tcrtHit = true;
        sensorIndex = which;
        servoStop();
        unsigned long tHit = millis() - tStart;
        Serial.printf("[tcrt] HIT S%d at %lums → STOP immediately\n", sensorIndex, tHit);
        return true;  // ส่งกลับทันทีไม่หมุนต่อ
      }
      // ถ้าไม่ยืนยัน ให้หมุนต่อ
    }

    // log เป็นช่วง ๆ
    if (millis() - tLastLog >= logEveryMs) {
      tLastLog = millis();
      Serial.printf("[dbg] spinning... t=%lums S1=%d S2=%d\n", tLastLog - tStart, tcrt1Raw() ? 1 : 0, tcrt2Raw() ? 1 : 0);
    }

    delay(1);
    yield();
  }
}

/* =======================
 * Brew steps
 * =======================*/
void brew(const String& orderId) {
  struct Step {
    const char* step;
    const char* msg;
  };

  const Step steps[] = {
    {"preparing_cup", "กำลังเตรียมแก้ว"},
    {"adding_toppings", "กำลังใส่ท็อปปิ้ง"},
    {"adding_ice", "กำลังใส่น้ำแข็ง"},
    {"brewing_drink", "กำลังชงเครื่องดื่ม"},
    {"completed", "เสร็จสิ้น"}
  };

  for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
    const String step = steps[i].step;
    const String msg = steps[i].msg;

    Serial.printf("[sim] %s → %s\n", orderId.c_str(), step.c_str());

    if (step == "completed") {
      servoStop();
      (void)postProgressExtended(orderId, step, "completed", msg, false, "done", 0);
      continue;
    }

    // ส่ง step เริ่มต้นก่อน (ยังไม่เจอ sensor)
    Serial.printf("[sim] Starting step: %s\n", step.c_str());
    (void)postProgressExtended(orderId, step, "in_progress", msg, false, "step_started", 0);

    // หมุน CCW จนกว่าจะเจอ sensor
    bool tcrtHit = false;
    int sensorIdx = 0;
    Serial.printf("[sim] Spinning until sensor detected for step: %s\n", step.c_str());
    (void)spinUntilMarkerCCW(tcrtHit, sensorIdx, LOG_EVERY_MS);

    if (tcrtHit) {
      // หยุด servo หลัก G13 ทันทีที่เจอ sensor
      servoStop();
      
      String msg2 = msg + String(" - เจอ sensor S") + sensorIdx;
      Serial.printf("[sim] Sensor S%d detected for step: %s\n", sensorIdx, step.c_str());
      (void)postProgressExtended(orderId, step, "in_progress", msg2, true, "sensor_detected", sensorIdx);

      // ทำงานพิเศษตาม step หลังจากเจอ sensor
      if (step == "adding_toppings") {
        Serial.println("[sim] Executing toppings action - servo G15");
        toppingServoAction();  // หมุน 180-0 1 ครั้ง
        Serial.println("[sim] Toppings step completed - resuming main servo");
      } 
      else if (step == "adding_ice") {
        Serial.println("[sim] Executing ice action - servo G05");
        iceServoAction();  // หมุน 180-0 3 ครั้ง
        Serial.println("[sim] Ice step completed - resuming main servo");
      }
      else if (step == "brewing_drink") {
        Serial.println("[sim] Executing brewing action - relay G2");
        fillWaterAction();  // เปิด relay 5 วินาที
        Serial.println("[sim] Brewing step completed - resuming main servo");
      }
      else {
        Serial.printf("[sim] No special action for step: %s - resuming main servo\n", step.c_str());
      }
      
      // หมุนต่อหลังจากทำงานพิเศษเสร็จ
      Serial.println("[servo] Resuming CCW rotation after special action...");
      servoCCW();  // เริ่มหมุนต่อ
      delay(1000); // หมุนต่อ 1 วินาทีแล้วเปลี่ยน step
      servoStop(); // หยุดก่อนเปลี่ยน step
      
      // ส่งสถานะว่า step นี้เสร็จแล้ว
      String completedMsg = msg + " - เสร็จสิ้น";
      (void)postProgressExtended(orderId, step, "completed", completedMsg, true, "step_completed", sensorIdx);
      
    } else {
      Serial.println("[error] Unexpected: spinUntilMarkerCCW returned without sensor hit");
    }
  }
}

/* =======================
 * Setup / Loop
 * =======================*/
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf("[sim] Starting hardware sim %s @ %s\n", HARDWARE_ID, BASE_URL);

  // I/O
  pinMode(TCRT1_PIN, INPUT_PULLUP);
  pinMode(TCRT2_PIN, INPUT_PULLUP);  // ระวัง: GPIO0 ต้อง HIGH ตอนบูต
  pinMode(RELAY_PIN, OUTPUT);        // ตั้งค่า relay pin เป็น output
  digitalWrite(RELAY_PIN, HIGH);     // เริ่มต้น relay ปิด (active low)
  Serial.printf("[setup] Relay pin G%d initialized HIGH (OFF)\n", RELAY_PIN);
  Serial.printf("[setup] Relay pin G%d initialized HIGH (OFF)\n", RELAY_PIN);

  // Servo setup
  cupServo.attach(SERVO_PIN, 500, 2500);          // servo หลัก G13
  toppingServo.attach(TOPPING_SERVO_PIN, 500, 2500);  // servo ท็อปปิ้ง G15
  iceServo.attach(ICE_SERVO_PIN, 500, 2500);           // servo น้ำแข็ง G05
  
  servoStop();
  
  // กำหนดตำแหน่งเริ่มต้นสำหรับ servo แบบ positional
  toppingServo.write(0);  // เริ่มต้นที่ 0 องศา
  iceServo.write(0);      // เริ่มต้นที่ 0 องศา

  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[sim] WiFi dropped, reconnecting...");
    servoStop();
    connectWiFi();
  }

  String orderId;
  int queuePos = -1;
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