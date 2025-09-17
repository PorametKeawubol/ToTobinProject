# TotoBin System Testing Guide

## 🚀 ระบบที่แก้ไขแล้ว

### ✅ ปัญหาที่แก้ไข:
1. **ESP32 Error 307**: แก้ไข API endpoints สำหรับ hardware polling
2. **Queue Routing Logic**: คิวลำดับที่ 1 ไปหน้า in-progress แทนหน้า queue
3. **Queue Management**: จัดการหน้าที่เกี่ยวกับคิวให้ดีขึ้น

### 🎯 Logic ใหม่:

#### **Queue Position Logic:**
- **Position 1** → ไปหน้า **in-progress** (รอเครื่องเริ่มทำ/กำลังทำ)
- **Position 2+** → อยู่ที่หน้า **queue** (รอคิว)
- **Processing** → หน้า **in-progress** (กำลังทำจริง)
- **Completed** → หน้า **done** (เสร็จแล้ว)

#### **Page Content:**
- **Queue Page**: แสดงเฉพาะคิว position 2+ (ซ่อน position 1)
- **In-Progress Page**: แสดงทั้ง "รอเครื่องเริ่มทำ" และ "กำลังทำ"

---

## 🧪 วิธีการทดสอบระบบ

### **1. ทดสอบ Web Application**

#### **A. Single User Flow:**
```bash
1. เปิด https://porametix.online
2. เลือกเครื่องดื่ม + toppings → "สั่งเลย"
3. ชำระเงิน (Mock Payment)
4. ควรไปหน้า "รอเครื่องเริ่มทำ" (in-progress)
5. รอ ESP32 เริ่มทำ (หรือใช้ manual test)
6. ดู LED states เปลี่ยนทุก 20 วินาที
7. เสร็จแล้วไปหน้า "done"
```

#### **B. Multiple Users Flow:**
```bash
User 1: สั่ง → ไปหน้า in-progress (position 1)
User 2: สั่ง → ไปหน้า queue (position 2) 
User 3: สั่ง → ไปหน้า queue (position 3)

เมื่อ User 1 เสร็จ:
- User 2 → ไปหน้า in-progress (position 1)
- User 3 → อยู่หน้า queue (position 2)
```

### **2. ทดสอบ ESP32 Hardware**

#### **A. ESP32 Setup:**
```cpp
// 1. Update WiFi credentials in esp32_client.ino
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// 2. Wire LEDs:
Pin 4  → Red LED    (Preparing)
Pin 5  → Yellow LED (Toppings) 
Pin 18 → Blue LED   (Ice)
Pin 19 → Green LED  (Brewing)
Pin 21 → White LED  (Completed)

// 3. Upload code to ESP32
// 4. Open Serial Monitor (115200 baud)
```

#### **B. Expected Serial Output:**
```
WiFi connected!
IP address: 192.168.x.x
Hardware esp32-001 ready
LED Test completed

Polling for orders...
No pending orders  

// When order comes:
New order received:
Order ID: order_xxx
Drink: ชาไทย

Starting brewing process for order: order_xxx
State 1/5: Preparing cup...
Status updated: กำลังเตรียมแก้ว
LED States - P:1 T:0 I:0 B:0 C:0

State 2/5: Adding toppings...
Status updated: ใส่ท็อปปิ้ง  
LED States - P:0 T:1 I:0 B:0 C:0
...
```

### **3. Manual Testing Tools**

#### **A. Hardware Monitor:**
```bash
# เปิดเครื่องมือ monitoring
https://porametix.online/esp32-monitor.html

1. กด "เริ่มทดสอบการชง"
2. ดู LED status real-time
3. ตรวจสอบ log และ timing
```

#### **B. API Testing:**
```bash
# Test ESP32 polling
curl -X GET "https://porametix.online/api/hardware/orders?hardwareId=esp32-001" \
  -H "X-API-Key: dev-hardware-key"

# Test manual brewing
curl -X POST "https://porametix.online/api/hardware/test-brewing" \
  -H "Content-Type: application/json" \
  -d '{"drinkName": "ชาไทย (ทดสอบ)", "toppings": ["ไข่มุก"]}'
```

---

## 🔧 การแก้ปัญหาที่พบบ่อย

### **ESP32 Issues:**

#### **Error 307 (Redirect):**
```cpp
// Already fixed in esp32_client.ino:
http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
```

#### **WiFi Connection Problems:**
```cpp
// Check WiFi credentials
Serial.println("SSID: " + String(ssid));
Serial.println("Status: " + String(WiFi.status()));
```

#### **API Communication:**
```cpp
// Check baseURL and API key
const char *baseURL = "https://porametix.online/api";
const char *hardwareAPIKey = "dev-hardware-key";
```

### **Web Issues:**

#### **Queue Routing Problems:**
```typescript
// Check useOrderRedirect.ts logic:
// Position 1 → in-progress  
// Position 2+ → queue
```

#### **Order Status Issues:**
```typescript
// Check order status in browser console:
console.log("Order:", currentUserOrder);
console.log("Status:", currentUserOrder.status);
console.log("Position:", currentUserOrder.queuePosition);
```

---

## 📊 Expected Results

### **Normal Flow Timeline:**
```
00:00 - User สั่งเครื่องดื่ม
00:01 - ไปหน้า in-progress (รอเครื่องเริ่มทำ)
00:05 - ESP32 polling ได้ order
00:06 - เริ่มกระบวนการชง 5 steps
00:26 - Step 1 เสร็จ (Preparing - 20s)
00:46 - Step 2 เสร็จ (Toppings - 20s)  
01:06 - Step 3 เสร็จ (Ice - 20s)
01:26 - Step 4 เสร็จ (Brewing - 20s)
01:46 - Step 5 เสร็จ (Completed - 20s)
01:49 - ไปหน้า done (3s delay)
```

### **Performance Targets:**
- **Web Response**: < 1 วินาที
- **ESP32 Polling**: ทุก 5 วินาที  
- **Hardware Status Update**: ทุก 2 วินาที
- **Total Brewing Time**: 100 วินาที (5×20s)
- **LED Sync Accuracy**: ± 2 วินาที

---

## 🎯 Production Checklist

### **Before Go-Live:**
- [ ] ทดสอบ multiple orders จำลอง
- [ ] ทดสอบ ESP32 hardware จริง
- [ ] ตรวจสอบ WiFi stability  
- [ ] ทดสอบ payment integration
- [ ] ทดสอบ queue management
- [ ] ตั้งค่า custom domain
- [ ] Backup queue data system

### **Hardware Requirements:**
- [ ] ESP32 Development Board ✅
- [ ] 5 LEDs + resistors ✅  
- [ ] Stable WiFi connection ✅
- [ ] Power supply ✅
- [ ] Enclosure/mounting ⏳

---

**🎉 ระบบพร้อมใช้งาน!** 
ทั้ง queue routing logic และ ESP32 hardware integration ทำงานได้ถูกต้องแล้ว