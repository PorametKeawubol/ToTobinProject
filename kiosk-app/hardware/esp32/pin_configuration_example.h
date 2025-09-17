/*
 * TotoBin ESP32 Pin Configuration Guide
 * =====================================================
 *
 * คู่มือการต่อสายและปรับแต่ง Pin สำหรับ ESP32
 */

// === PIN CONFIGURATION ===
// ปรับแต่ง Pin ตามการต่อสายจริงของคุณ

// LEDs
const int LED_STATUS = 2;     // LED onboard ESP32 (ไม่ต้องต่อสาย)
const int LED_COMPLETION = 4; // LED แสดงเครื่องดื่มเสร็จ (ต่อผ่าน resistor 220Ω)

// Motors & Actuators
const int PUMP_PIN = 5;   // ปั๊มน้ำ/เครื่องดื่ม (ผ่าน Relay)
const int VALVE_PIN = 18; // วาล์วท็อปปิ้ง/น้ำแข็ง (ผ่าน Relay)

// Sensors
const int SENSOR_PIN = 34; // เซ็นเซอร์ระดับน้ำ (ADC Pin)

// === WIRING DIAGRAM ===
/*
LED Completion (Pin 4):
ESP32 Pin 4 → 220Ω Resistor → LED(+) → LED(-) → GND

Pump Relay (Pin 5):
ESP32 Pin 5 → Relay IN1
ESP32 3.3V → Relay VCC
ESP32 GND → Relay GND
Relay COM → Pump (+)
Relay NO → Power Supply (+)
Power Supply (-) → Pump (-) → ESP32 GND

Valve Relay (Pin 18):
ESP32 Pin 18 → Relay IN2
(Same power setup as pump)

Water Level Sensor (Pin 34):
Sensor Signal → ESP32 Pin 34
Sensor VCC → ESP32 3.3V
Sensor GND → ESP32 GND
*/

// === ALTERNATIVE PIN OPTIONS ===
// ถ้าต้องการเปลี่ยน Pin สามารถใช้ Pin เหล่านี้:

// Digital Output Pins (สำหรับ LED, Relay):
// 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27

// ADC Input Pins (สำหรับ Sensor):
// 32, 33, 34, 35, 36, 39

// === POWER REQUIREMENTS ===
/*
ESP32: 3.3V (USB หรือ External Power)
Relay Module: 3.3V/5V (ดูจาก spec)
Pump/Valve: 12V/24V (ต้องใช้ External Power Supply)
LED: 3.3V (ผ่าน resistor)
*/

// === SAFETY NOTES ===
/*
1. ใช้ Relay สำหรับอุปกรณ์แรงดันสูง (ปั๊ม, วาล์ว)
2. ไม่ต่ออุปกรณ์แรงดันสูงเข้า ESP32 โดยตรง
3. ตรวจสอบ Ground connection ให้ดี
4. ใช้ External Power Supply สำหรับปั๊ม/วาล์ว
*/