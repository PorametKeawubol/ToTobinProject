#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// API Configuration
const char *baseURL = "https://porametix.online/api";
const char *hardwareAPIKey = "dev-hardware-key";
const char *hardwareId = "esp32-001";

// Hardware pins (adjust based on your setup)
const int LED_STATUS = 2;     // Status LED (onboard)
const int LED_PREPARING = 4;  // LED สำหรับ preparing_cup (สีแดง)
const int LED_TOPPINGS = 5;   // LED สำหรับ adding_toppings (สีเหลือง)
const int LED_ICE = 18;       // LED สำหรับ adding_ice (สีน้ำเงิน)
const int LED_BREWING = 19;   // LED สำหรับ brewing_drink (สีเขียว)
const int LED_COMPLETED = 21; // LED สำหรับ completed (สีขาว)
const int SENSOR_PIN = 34;    // Sensor pin (สำหรับอนาคต)

// Timing constants
const unsigned long STATE_DURATION = 20000; // 20 วินาที ต่อ state

// State variables
String currentOrderId = "";
bool isCurrentlyBrewing = false;
unsigned long brewingStartTime = 0;
unsigned long lastPollTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastCommandCheck = 0;
const unsigned long POLL_INTERVAL = 5000;          // 5 seconds
const unsigned long HEARTBEAT_INTERVAL = 30000;    // 30 seconds
const unsigned long COMMAND_CHECK_INTERVAL = 2000; // 2 seconds

void setup()
{
 Serial.begin(115200);

 // Initialize pins
 pinMode(LED_STATUS, OUTPUT);
 pinMode(LED_PREPARING, OUTPUT);
 pinMode(LED_TOPPINGS, OUTPUT);
 pinMode(LED_ICE, OUTPUT);
 pinMode(LED_BREWING, OUTPUT);
 pinMode(LED_COMPLETED, OUTPUT);
 pinMode(SENSOR_PIN, INPUT);

 // Turn off all outputs
 digitalWrite(LED_STATUS, LOW);
 digitalWrite(LED_PREPARING, LOW);
 digitalWrite(LED_TOPPINGS, LOW);
 digitalWrite(LED_ICE, LOW);
 digitalWrite(LED_BREWING, LOW);
 digitalWrite(LED_COMPLETED, LOW);

 // Connect to WiFi
 connectWiFi();

 Serial.println("TotoBin ESP32 Hardware ready!");
 Serial.println("Hardware ID: " + String(hardwareId));

 // Test all LEDs on startup
 testAllLEDs();
}

void loop()
{
 unsigned long currentTime = millis();

 // Check WiFi connection
 if (WiFi.status() != WL_CONNECTED)
 {
  Serial.println("WiFi disconnected, reconnecting...");
  connectWiFi();
  return;
 }

 // Send heartbeat
 if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL)
 {
  sendHeartbeat();
  lastHeartbeat = currentTime;
 }

 // Check for commands from API
 if (currentTime - lastCommandCheck >= COMMAND_CHECK_INTERVAL)
 {
  checkForCommands();
  lastCommandCheck = currentTime;
 }

 // Poll for orders if not brewing
 if (!isCurrentlyBrewing && currentTime - lastPollTime >= POLL_INTERVAL)
 {
  pollForOrders();
  lastPollTime = currentTime;
 }

 delay(100);
}

void connectWiFi()
{
 WiFi.begin(ssid, password);
 Serial.print("Connecting to WiFi");

 while (WiFi.status() != WL_CONNECTED)
 {
  delay(500);
  Serial.print(".");
 }

 Serial.println("");
 Serial.println("WiFi connected!");
 Serial.println("IP address: " + WiFi.localIP().toString());
}

void pollForOrders()
{
 HTTPClient http;
 String url = String(baseURL) + "/hardware/orders?hardwareId=" + hardwareId;

 http.begin(url);
 http.addHeader("X-API-Key", hardwareAPIKey);

 int httpCode = http.GET();

 if (httpCode == 200)
 {
  String payload = http.getString();
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  if (doc["success"] && !doc["order"].isNull())
  {
   // New order received
   String orderId = doc["order"]["id"];
   String drinkName = doc["order"]["drinkName"];

   Serial.println("New order received:");
   Serial.println("Order ID: " + orderId);
   Serial.println("Drink: " + drinkName);

   currentOrderId = orderId;
   startBrewing();
  }
 }
 else
 {
  Serial.println("Error polling orders: " + String(httpCode));
 }

 http.end();
}

void startBrewing()
{
 if (currentOrderId == "")
  return;

 isCurrentlyBrewing = true;
 brewingStartTime = millis();    // เก็บเวลาเริ่มต้น
 digitalWrite(LED_STATUS, HIGH); // เปิด Status LED ตลอดกระบวนการ

 Serial.println("Starting brewing process for order: " + currentOrderId);
 Serial.println("Each state will take 20 seconds");

 // ปิด LED ทั้งหมดก่อนเริ่ม
 turnOffAllStateLEDs();

 // Step 1: Preparing cup (20 วินาที)
 Serial.println("State 1/5: Preparing cup...");
 digitalWrite(LED_PREPARING, HIGH);
 updateStatus("preparing", "preparing_cup", "กำลังเตรียมแก้ว");
 delay(STATE_DURATION);
 digitalWrite(LED_PREPARING, LOW);

 // Step 2: Adding toppings (20 วินาที)
 Serial.println("State 2/5: Adding toppings...");
 digitalWrite(LED_TOPPINGS, HIGH);
 updateStatus("brewing", "adding_toppings", "ใส่ท็อปปิ้ง");
 delay(STATE_DURATION);
 digitalWrite(LED_TOPPINGS, LOW);

 // Step 3: Adding ice (20 วินาที)
 Serial.println("State 3/5: Adding ice...");
 digitalWrite(LED_ICE, HIGH);
 updateStatus("brewing", "adding_ice", "ใส่น้ำแข็ง");
 delay(STATE_DURATION);
 digitalWrite(LED_ICE, LOW);

 // Step 4: Brewing drink (20 วินาที)
 Serial.println("State 4/5: Brewing drink...");
 digitalWrite(LED_BREWING, HIGH);
 updateStatus("brewing", "brewing_drink", "ใส่เครื่องดื่ม");
 delay(STATE_DURATION);
 digitalWrite(LED_BREWING, LOW);

 // Step 5: Completed (20 วินาที สำหรับแสดงผล)
 Serial.println("State 5/5: Completed!");
 digitalWrite(LED_COMPLETED, HIGH);
 updateStatus("completed", "completed", "เสร็จสิ้น");
 delay(STATE_DURATION);
 digitalWrite(LED_COMPLETED, LOW);

 Serial.println("Brewing completed for order: " + currentOrderId);
 Serial.println("Total time: 100 seconds (5 states x 20 seconds)");

 // Reset state
 currentOrderId = "";
 isCurrentlyBrewing = false;
 brewingStartTime = 0;
 digitalWrite(LED_STATUS, LOW);

 // Final completion blink
 completionBlink();
}

void updateStatus(String status, String step, String message)
{
 HTTPClient http;
 String url = String(baseURL) + "/hardware/status";

 http.begin(url);
 http.addHeader("Content-Type", "application/json");
 http.addHeader("X-API-Key", hardwareAPIKey);

 DynamicJsonDocument doc(1024);
 doc["orderId"] = currentOrderId;
 doc["status"] = status;
 doc["step"] = step;
 doc["message"] = message;
 doc["hardwareId"] = hardwareId;

 // Add LED state information
 JsonObject ledState = doc.createNestedObject("ledState");
 ledState["preparing"] = digitalRead(LED_PREPARING) == HIGH;
 ledState["toppings"] = digitalRead(LED_TOPPINGS) == HIGH;
 ledState["ice"] = digitalRead(LED_ICE) == HIGH;
 ledState["brewing"] = digitalRead(LED_BREWING) == HIGH;
 ledState["completed"] = digitalRead(LED_COMPLETED) == HIGH;

 // Calculate progress and elapsed time
 int progress = 0;
 int elapsedTime = 0;

 if (isCurrentlyBrewing && brewingStartTime > 0)
 {
  elapsedTime = (millis() - brewingStartTime) / 1000;
  progress = min(100, (elapsedTime * 100) / 100); // 100 seconds total
 }

 doc["progress"] = progress;
 doc["elapsedTime"] = elapsedTime;
 doc["totalTime"] = 100; // 5 states × 20 seconds each

 String jsonString;
 serializeJson(doc, jsonString);

 int httpCode = http.POST(jsonString);

 if (httpCode == 200)
 {
  Serial.println("Status updated: " + message);
  Serial.println("LED States - P:" + String(digitalRead(LED_PREPARING)) +
                 " T:" + String(digitalRead(LED_TOPPINGS)) +
                 " I:" + String(digitalRead(LED_ICE)) +
                 " B:" + String(digitalRead(LED_BREWING)) +
                 " C:" + String(digitalRead(LED_COMPLETED)));
 }
 else
 {
  Serial.println("Error updating status: " + String(httpCode));
 }

 http.end();
}

void sendHeartbeat()
{
 HTTPClient http;
 String url = String(baseURL) + "/hardware/status?hardwareId=" + hardwareId;

 http.begin(url);
 http.addHeader("X-API-Key", hardwareAPIKey);

 int httpCode = http.GET();

 if (httpCode == 200)
 {
  Serial.println("Heartbeat sent successfully");
 }
 else
 {
  Serial.println("Heartbeat failed: " + String(httpCode));
 }

 http.end();
}

// Check for commands from API
void checkForCommands()
{
 HTTPClient http;
 String url = String(baseURL) + "/hardware/trigger?hardwareId=" + hardwareId;

 http.begin(url);
 http.addHeader("X-API-Key", hardwareAPIKey);

 int httpCode = http.GET();

 if (httpCode == 200)
 {
  String payload = http.getString();
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  if (doc["success"] && !doc["command"].isNull())
  {
   // Execute command
   String action = doc["command"]["action"];
   String commandId = doc["command"]["id"];

   Serial.println("Received command: " + action + " (ID: " + commandId + ")");

   if (action == "completion_signal")
   {
    int ledPin = doc["command"]["params"]["ledPin"] | LED_COMPLETED;
    int duration = doc["command"]["params"]["duration"] | 3000;

    executeCompletionSignal(ledPin, duration);
   }
   else if (action == "test_led")
   {
    testLED();
   }
  }
 }

 http.end();
}

// Execute completion signal
void executeCompletionSignal(int ledPin, int duration)
{
 Serial.println("Executing completion signal on pin " + String(ledPin) + " for " + String(duration) + "ms");

 // If it's the completion LED, use special pattern
 if (ledPin == LED_COMPLETED)
 {
  completionBlink();
 }
 else
 {
  // Blink pattern for other LEDs
  unsigned long startTime = millis();
  while (millis() - startTime < duration)
  {
   digitalWrite(ledPin, HIGH);
   delay(200);
   digitalWrite(ledPin, LOW);
   delay(200);
  }
 }

 Serial.println("Completion signal finished");
}

// Auto-trigger completion signal (internal)
void triggerCompletionSignal()
{
 executeCompletionSignal(LED_COMPLETED, 3000);
}

// Turn off all state LEDs
void turnOffAllStateLEDs()
{
 digitalWrite(LED_PREPARING, LOW);
 digitalWrite(LED_TOPPINGS, LOW);
 digitalWrite(LED_ICE, LOW);
 digitalWrite(LED_BREWING, LOW);
 digitalWrite(LED_COMPLETED, LOW);
}

// Test all LEDs on startup
void testAllLEDs()
{
 Serial.println("Testing all LEDs...");

 // Test each LED for 500ms
 digitalWrite(LED_STATUS, HIGH);
 delay(500);
 digitalWrite(LED_STATUS, LOW);

 digitalWrite(LED_PREPARING, HIGH);
 delay(500);
 digitalWrite(LED_PREPARING, LOW);

 digitalWrite(LED_TOPPINGS, HIGH);
 delay(500);
 digitalWrite(LED_TOPPINGS, LOW);

 digitalWrite(LED_ICE, HIGH);
 delay(500);
 digitalWrite(LED_ICE, LOW);

 digitalWrite(LED_BREWING, HIGH);
 delay(500);
 digitalWrite(LED_BREWING, LOW);

 digitalWrite(LED_COMPLETED, HIGH);
 delay(500);
 digitalWrite(LED_COMPLETED, LOW);

 Serial.println("LED test completed");
}

// Completion blink pattern
void completionBlink()
{
 Serial.println("Final completion signal...");

 // Blink all LEDs together 3 times
 for (int i = 0; i < 3; i++)
 {
  digitalWrite(LED_PREPARING, HIGH);
  digitalWrite(LED_TOPPINGS, HIGH);
  digitalWrite(LED_ICE, HIGH);
  digitalWrite(LED_BREWING, HIGH);
  digitalWrite(LED_COMPLETED, HIGH);
  delay(300);

  turnOffAllStateLEDs();
  delay(300);
 }

 Serial.println("Completion signal finished");
}

// Test LED function (เรียกจาก API)
void testLED()
{
 Serial.println("API LED Test requested...");

 // Test status LED
 digitalWrite(LED_STATUS, HIGH);
 delay(500);
 digitalWrite(LED_STATUS, LOW);
 delay(500);

 // Test all state LEDs in sequence
 int stateLEDs[] = {LED_PREPARING, LED_TOPPINGS, LED_ICE, LED_BREWING, LED_COMPLETED};
 String stateNames[] = {"PREPARING", "TOPPINGS", "ICE", "BREWING", "COMPLETED"};

 for (int i = 0; i < 5; i++)
 {
  Serial.println("Testing LED: " + stateNames[i]);
  digitalWrite(stateLEDs[i], HIGH);
  delay(1000);
  digitalWrite(stateLEDs[i], LOW);
  delay(300);
 }

 Serial.println("API LED test completed");
}