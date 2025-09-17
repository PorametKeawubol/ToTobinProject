# ESP32 Hardware Testing Guide

## การทดสอบ ESP32 กับระบบ TotoBin

### 1. เตรียมความพร้อม

#### Hardware Setup
- ESP32 Development Board
- 5 LED (แดง, เหลือง, น้ำเงิน, เขียว, ขาว)
- 5 ตัวต้านทาน 220Ω
- Breadboard และสายจัมเปอร์

#### Pin Configuration
```cpp
LED_PREPARING = 4    // สีแดง
LED_TOPPINGS = 5     // สีเหลือง  
LED_ICE = 18         // สีน้ำเงิน
LED_BREWING = 19     // สีเขียว
LED_COMPLETED = 21   // สีขาว
```

### 2. การตั้งค่า WiFi

แก้ไขไฟล์ `esp32_client.ino`:
```cpp
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
```

### 3. การทดสอบการเชื่อมต่อ

#### 3.1 Serial Monitor Test
1. Upload โค้ดไปยัง ESP32
2. เปิด Serial Monitor (115200 baud)
3. ตรวจสอบข้อความ:
   ```
   WiFi connected!
   IP address: 192.168.x.x
   Hardware esp32-001 ready
   LED Test completed
   ```

#### 3.2 LED Function Test
ESP32 จะทดสอบ LED ทั้งหมดตอนเริ่มต้น:
- LED แต่ละดวงจะเปิด-ปิด 3 ครั้ง
- ตรวจสอบให้แน่ใจว่า LED ทุกดวงทำงานปกติ

### 4. การทดสอบ API Communication

#### 4.1 Heartbeat Test
ESP32 จะส่ง heartbeat ทุก 30 วินาที:
```
Heartbeat sent successfully
```

#### 4.2 Order Polling Test
ESP32 จะ poll สำหรับ order ใหม่ทุก 5 วินาที:
```
Polling for orders...
No pending orders
```

### 5. การทดสอบ Brewing Process

#### 5.1 สร้าง Order ผ่านเว็บ
1. เปิด https://porametix.online
2. เลือกเครื่องดื่มและ toppings
3. กด "สั่งเลย"
4. ไปหน้า "สถานะการชง"

#### 5.2 ตรวจสอบ ESP32 Response
Serial Monitor จะแสดง:
```
Received order: order_xxx
Starting brewing process for order: order_xxx
Each state will take 20 seconds

State 1/5: Preparing cup...
Status updated: กำลังเตรียมแก้ว
LED States - P:1 T:0 I:0 B:0 C:0

State 2/5: Adding toppings...
Status updated: ใส่ท็อปปิ้ง
LED States - P:0 T:1 I:0 B:0 C:0

State 3/5: Adding ice...
Status updated: ใส่น้ำแข็ง
LED States - P:0 T:0 I:1 B:0 C:0

State 4/5: Brewing drink...
Status updated: ใส่เครื่องดื่ม
LED States - P:0 T:0 I:0 B:1 C:0

State 5/5: Completed!
Status updated: เสร็จสิ้น
LED States - P:0 T:0 I:0 B:0 C:1

Brewing completed for order: order_xxx
Total time: 100 seconds (5 states x 20 seconds)
```

### 6. การทดสอบ Web Integration

#### 6.1 Real-time Status Updates
ตรวจสอบในหน้าเว็บ "สถานะการชง":
- Progress bar ควรเคลื่อนไหวตาม LED
- ข้อความ status ควรเปลี่ยนแปลงทุก 20 วินาที
- เวลาที่เหลือควรลดลงแบบ real-time

#### 6.2 LED State Synchronization
- LED ที่เปิดอยู่ควรสอดคล้องกับ progress ในเว็บ
- เมื่อเปลี่ยน state LED ก่อนหน้าควรปิด และ LED ใหม่ควรเปิด

### 7. การแก้ปัญหา

#### 7.1 WiFi Connection Issues
```cpp
// ตรวจสอบ credentials
const char *ssid = "YOUR_ACTUAL_WIFI_NAME";
const char *password = "YOUR_ACTUAL_PASSWORD";
```

#### 7.2 API Communication Issues
```cpp
// ตรวจสอบ API key
const char *hardwareAPIKey = "dev-hardware-key";

// ตรวจสอบ URL
const char *baseURL = "https://porametix.online/api";
```

#### 7.3 LED Issues
```cpp
// ตรวจสอบ pin assignment
const int LED_PREPARING = 4;   // ใช้ GPIO ที่ support output
const int LED_TOPPINGS = 5;
const int LED_ICE = 18;
const int LED_BREWING = 19; 
const int LED_COMPLETED = 21;
```

### 8. การผลิตจริง (Production)

#### 8.1 Update WiFi Credentials
```cpp
const char *ssid = "PRODUCTION_WIFI";
const char *password = "PRODUCTION_PASSWORD";
```

#### 8.2 Update Hardware ID
```cpp
const char *hardwareId = "esp32-kiosk-01"; // unique ID for each machine
```

#### 8.3 Update API Key
```cpp
const char *hardwareAPIKey = "PRODUCTION_API_KEY";
```

### 9. Monitoring Commands

#### 9.1 Check WiFi Status
```
AT+CWJAP?
```

#### 9.2 Monitor Memory Usage
```cpp
Serial.println("Free heap: " + String(ESP.getFreeHeap()));
```

#### 9.3 Monitor Network Activity
Serial Monitor จะแสดงทุก HTTP request/response

---

## Expected Behavior

### ขั้นตอนการทำงานปกติ:
1. **Startup**: Test LED → Connect WiFi → Start polling
2. **Order Received**: แสดง "Received order" → เริ่ม brewing
3. **Brewing Process**: LED เปลี่ยนทุก 20 วินาที × 5 states
4. **Completion**: LED completed กะพริบ → รีเซ็ต → กลับไป polling

### ประสิทธิภาพที่คาดหวัง:
- **Total Brewing Time**: 100 วินาที (5 states × 20 วินาที)
- **API Update Frequency**: ทุก state เปลี่ยน + real-time progress
- **Web Sync Accuracy**: ≤ 2 วินาที delay
- **LED Response Time**: ≤ 1 วินาที
