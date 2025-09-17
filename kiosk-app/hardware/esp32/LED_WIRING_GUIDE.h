/*
 * TotoBin ESP32 LED Wiring Guide - 5 State LEDs + Status LED
 * ==========================================================
 *
 * สำหรับการ Mock การทำเครื่องดื่มด้วย LED แสดง State
 * แต่ละ State จะใช้เวลา 20 วินาที
 */

// === LED PIN CONFIGURATION ===
/*
LED_STATUS = 2;        // LED Status (onboard ESP32) - สีน้ำเงิน
LED_PREPARING = 4;     // State 1: Preparing cup - สีแดง
LED_TOPPINGS = 5;      // State 2: Adding toppings - สีเหลือง
LED_ICE = 18;          // State 3: Adding ice - สีน้ำเงิน
LED_BREWING = 19;      // State 4: Brewing drink - สีเขียว
LED_COMPLETED = 21;    // State 5: Completed - สีขาว
*/

// === WIRING DIAGRAM ===
/*
สำหรับแต่ละ LED (Pin 4, 5, 18, 19, 21):

ESP32 Pin → 220Ω Resistor → LED Anode (+)
LED Cathode (-) → ESP32 GND

ตัวอย่าง Pin 4 (LED_PREPARING):
ESP32 Pin 4 → 220Ω → LED(+) → LED(-) → GND
*/

// === LED COLORS RECOMMENDATION ===
/*
Pin 2  (STATUS):    LED น้ำเงิน - แสดงว่าเครื่องกำลังทำงาน
Pin 4  (PREPARING): LED แดง - เตรียมแก้ว
Pin 5  (TOPPINGS):  LED เหลือง - ใส่ท็อปปิ้ง
Pin 18 (ICE):       LED น้ำเงินอ่อน - ใส่น้ำแข็ง
Pin 19 (BREWING):   LED เขียว - ชงเครื่องดื่ม
Pin 21 (COMPLETED): LED ขาว - เสร็จสิ้น
*/

// === BREWING SEQUENCE ===
/*
Total Time: 100 วินาที (1 นาที 40 วินาที)

1. Order Received → LED_STATUS เปิด (ติดตลอดกระบวนการ)
2. State 1 (0-20s):   LED_PREPARING เปิด → API: "preparing", "preparing_cup"
3. State 2 (20-40s):  LED_TOPPINGS เปิด → API: "brewing", "adding_toppings"
4. State 3 (40-60s):  LED_ICE เปิด → API: "brewing", "adding_ice"
5. State 4 (60-80s):  LED_BREWING เปิด → API: "brewing", "brewing_drink"
6. State 5 (80-100s): LED_COMPLETED เปิด → API: "completed", "completed"
7. Final Signal:      ทุก LED กะพริบ 3 ครั้ง → LED_STATUS ปิด
*/

// === API COMMUNICATION ===
/*
ESP32 จะส่งสถานะไปยัง API ทุกครั้งที่เปลี่ยน State:

POST /api/hardware/status
{
  "orderId": "order_123",
  "status": "brewing",
  "step": "adding_ice",
  "message": "ใส่น้ำแข็ง",
  "hardwareId": "esp32-001"
}

เว็บจะได้รับสถานะนี้และแสดงหน้า in-progress ตาม State ที่ได้รับ
*/

// === TESTING COMMANDS ===
/*
1. Test All LEDs:
   POST /api/hardware/trigger
   { "hardwareId": "esp32-001", "action": "test_led" }

2. Test Completion Signal:
   POST /api/hardware/trigger
   {
     "hardwareId": "esp32-001",
     "action": "completion_signal",
     "ledPin": 21,
     "duration": 3000
   }
*/

// === POWER REQUIREMENTS ===
/*
- ESP32: 3.3V (USB Power)
- LED (6 ดวง): 3.3V @ ~20mA each = 120mA total
- Resistors: 220Ω (6 ตัว)
- Total Power: ประมาณ 400mA (ปลอดภัยสำหรับ USB)
*/

// === SHOPPING LIST ===
/*
1. ESP32 Development Board (1 ชิ้น)
2. LED 5mm หลากสี (6 ดวง)
   - แดง (1), เหลือง (1), น้ำเงิน (2), เขียว (1), ขาว (1)
3. Resistor 220Ω (6 ตัว)
4. Breadboard (1 ชิ้น)
5. Jump Wires Male-to-Male (10 เส้น)
6. USB Cable Type-C/Micro-USB (1 เส้น)
*/