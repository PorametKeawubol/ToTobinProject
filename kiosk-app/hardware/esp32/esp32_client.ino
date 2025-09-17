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
const int LED_PIN = 2;
const int PUMP_PIN = 4;
const int VALVE_PIN = 5;
const int SENSOR_PIN = 34;

// State variables
String currentOrderId = "";
bool isBrewing = false;
unsigned long lastPollTime = 0;
unsigned long lastHeartbeat = 0;
const unsigned long POLL_INTERVAL = 5000;       // 5 seconds
const unsigned long HEARTBEAT_INTERVAL = 30000; // 30 seconds

void setup()
{
 Serial.begin(115200);

 // Initialize pins
 pinMode(LED_PIN, OUTPUT);
 pinMode(PUMP_PIN, OUTPUT);
 pinMode(VALVE_PIN, OUTPUT);
 pinMode(SENSOR_PIN, INPUT);

 // Turn off all outputs
 digitalWrite(LED_PIN, LOW);
 digitalWrite(PUMP_PIN, LOW);
 digitalWrite(VALVE_PIN, LOW);

 // Connect to WiFi
 connectWiFi();

 Serial.println("TotoBin ESP32 Hardware ready!");
 Serial.println("Hardware ID: " + String(hardwareId));
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

 // Poll for orders if not brewing
 if (!isBrewing && currentTime - lastPollTime >= POLL_INTERVAL)
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

 isBrewing = true;
 digitalWrite(LED_PIN, HIGH); // Indicator LED

 Serial.println("Starting brewing process for order: " + currentOrderId);

 // Step 1: Preparing cup
 updateStatus("preparing", "preparing_cup", "กำลังเตรียมแก้ว");
 delay(3000);

 // Step 2: Adding toppings
 updateStatus("brewing", "adding_toppings", "ใส่ท็อปปิ้ง");
 digitalWrite(VALVE_PIN, HIGH);
 delay(4000);
 digitalWrite(VALVE_PIN, LOW);

 // Step 3: Adding ice
 updateStatus("brewing", "adding_ice", "ใส่น้ำแข็ง");
 delay(3500);

 // Step 4: Brewing drink
 updateStatus("brewing", "brewing_drink", "ใส่เครื่องดื่ม");
 digitalWrite(PUMP_PIN, HIGH);
 delay(5000);
 digitalWrite(PUMP_PIN, LOW);

 // Step 5: Completed
 updateStatus("completed", "completed", "เสร็จสิ้น");

 Serial.println("Brewing completed for order: " + currentOrderId);

 // Reset state
 currentOrderId = "";
 isBrewing = false;
 digitalWrite(LED_PIN, LOW);

 // Blink LED to indicate completion
 for (int i = 0; i < 5; i++)
 {
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(200);
 }
}

void updateStatus(String status, String step, String message)
{
 HTTPClient http;
 String url = String(baseURL) + "/hardware/status";

 http.begin(url);
 http.addHeader("Content-Type", "application/json");
 http.addHeader("X-API-Key", hardwareAPIKey);

 DynamicJsonDocument doc(512);
 doc["orderId"] = currentOrderId;
 doc["status"] = status;
 doc["step"] = step;
 doc["message"] = message;
 doc["hardwareId"] = hardwareId;

 String jsonString;
 serializeJson(doc, jsonString);

 int httpCode = http.POST(jsonString);

 if (httpCode == 200)
 {
  Serial.println("Status updated: " + message);
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