# 🚀 TotoBin Kiosk - คู่มือ Deploy ผ่าน Google Cloud Console

## 📋 ขั้นตอนการ Deploy แบบละเอียด

### เตรียมความพร้อม
ก่อนเริ่ม ให้แน่ใจว่า:
- ✅ เข้าสู่ระบบ Google Cloud ด้วย account: `poramet.contact@gmail.com`
- ✅ มี billing account เปิดใช้งานแล้ว
- ✅ Project `totobin-kiosk-porametix` พร้อมใช้งาน

---

## 🎯 วิธีที่ 1: Deploy จาก Source Code (แนะนำ)

### ขั้นตอนที่ 1: เข้าสู่ Google Cloud Console
1. เปิดเบราว์เซอร์ไปที่: https://console.cloud.google.com/run?project=totobin-kiosk-porametix
2. ตรวจสอบว่าอยู่ใน Project `totobin-kiosk-porametix` ด้านบนซ้าย

### ขั้นตอนที่ 2: สร้าง Cloud Run Service
1. คลิกปุ่ม **"CREATE SERVICE"** (ปุ่มสีน้ำเงิน)
2. เลือก **"Continuously deploy new revisions from a source repository"**
3. คลิก **"SET UP WITH CLOUD BUILD"**

### ขั้นตอนที่ 3: เชื่อมต่อ Source Repository
1. เลือก **Repository provider**: GitHub
2. คลิก **"MANAGE CONNECTED REPOSITORIES"**
3. เชื่อมต่อกับ GitHub repository: `PorametKeawubol/ToTobinProject`
4. เลือก Branch: `main`
5. คลิก **"NEXT"**

### ขั้นตอนที่ 4: กำหนดค่า Build
**Build Configuration:**
- Build Type: **Dockerfile** หรือ **Buildpacks**
- Source Location: `/kiosk-app/` (โฟลเดอร์ที่มีโค้ด)

**หาก Dockerfile ไม่พบ ให้เลือก Buildpacks และระบุ:**
- Build Environment: **Node.js**
- Source Location: `kiosk-app/`

### ขั้นตอนที่ 5: การตั้งค่า Service
**Service Configuration:**
- Service name: `totobin-kiosk`
- Region: **asia-southeast1 (Singapore)**
- CPU allocation: **CPU is only allocated during request processing**
- Ingress: **Allow all traffic**
- Authentication: **Allow unauthenticated invocations** ✅

### ขั้นตอนที่ 6: Container Configuration
**Container Settings:**
- Container port: `3000`
- Memory: `1 GiB`
- CPU: `1 vCPU`
- Maximum requests per container: `80`
- Timeout: `300 seconds`

### ขั้นตอนที่ 7: ตั้งค่า Environment Variables
คลิก **"VARIABLES & SECRETS"** และเพิ่ม Environment Variables:

```
NODE_ENV = production
PORT = 3000
HARDWARE_API_KEY = totobin-hardware-secret-2024
PROMPTPAY_PHONE = 0984435255
BUSINESS_NAME = TotoBin Kiosk
ENABLE_HARDWARE_INTEGRATION = true
ENABLE_QUEUE_SYSTEM = true
```

**วิธีเพิ่ม Environment Variables:**
1. คลิก **"+ ADD VARIABLE"**
2. ใส่ชื่อตัวแปรในช่อง **"Name"**
3. ใส่ค่าในช่อง **"Value"**
4. คลิก **"DONE"**
5. ทำซ้ำสำหรับทุกตัวแปร

### ขั้นตอนที่ 8: การตั้งค่าขั้นสูง
**Capacity:**
- Minimum number of instances: `0`
- Maximum number of instances: `10`

**Autoscaling:**
- Target CPU utilization: `60%`
- Target concurrency: `80`

### ขั้นตอนที่ 9: Deploy Service
1. คลิก **"CREATE"** ด้านล่าง
2. รอให้ build และ deploy เสร็จ (อาจใช้เวลา 5-10 นาที)
3. เมื่อเสร็จจะแสดงสถานะ **"Service deployed successfully"**

---

## 🎯 วิธีที่ 2: Deploy จาก ZIP File (สำรอง)

### หากมีปัญหากับ Git Repository:

1. **เตรียม Source Code**
   - Zip โฟลเดอร์ `kiosk-app` ทั้งหมด
   - ตั้งชื่อไฟล์: `totobin-kiosk.zip`

2. **Upload to Cloud Storage**
   - ไปที่: https://console.cloud.google.com/storage/browser?project=totobin-kiosk-porametix
   - สร้าง bucket: `totobin-source-code`
   - อัพโหลดไฟล์ zip

3. **Deploy จาก Source**
   - กลับไปที่ Cloud Run Console
   - เลือก **"Deploy from source"**
   - เลือกไฟล์จาก Cloud Storage

---

## 🔧 การตรวจสอบหลัง Deploy

### 1. ตรวจสอบ Service URL
หลังจาก deploy เสร็จ จะได้ URL แบบ:
```
https://totobin-kiosk-[random-id]-uc.a.run.app
```

### 2. ทดสอบ Application
เข้าไปที่ URL แล้วทดสอบ:
- ✅ หน้าแรกโหลดได้
- ✅ เมนูเครื่องดื่มแสดงขึ้น
- ✅ สามารถเพิ่มสินค้าใส่ตะกร้าได้
- ✅ สร้าง QR Code PromptPay ได้
- ✅ หน้า Queue ทำงานได้

### 3. ตรวจสอบ Logs
ไปที่ **"LOGS"** tab เพื่อดู:
- ✅ Application เริ่มต้นได้ถูกต้อง
- ✅ ไม่มี error ใน startup
- ✅ Port 3000 ถูก bind

---

## 🌐 ตั้งค่า Custom Domain (เพิ่มเติม)

### หลังจาก Deploy เสร็จแล้ว:

1. **ไปที่ Domain Mappings**
   - ใน Cloud Run Console
   - คลิก **"MANAGE CUSTOM DOMAINS"**

2. **เพิ่ม Domain**
   - คลิก **"ADD MAPPING"**
   - Domain: `porametix.online`
   - Service: `totobin-kiosk`
   - คลิก **"CONTINUE"**

3. **ตั้งค่า DNS**
   - Google จะแสดง DNS records ที่ต้องตั้งค่า
   - ไปที่ Domain Provider (Cloudflare/GoDaddy/etc.)
   - เพิ่ม CNAME record ตามที่ Google บอก

---

## 📱 ตั้งค่า Hardware Integration

### หลังจากได้ Production URL แล้ว:

1. **อัพเดท ESP32 Code**
   ```cpp
   const char* SERVER_URL = "https://totobin-kiosk-[your-id]-uc.a.run.app";
   const char* API_KEY = "totobin-hardware-secret-2024";
   ```

2. **อัพเดท Odroid Python Client**
   ```python
   BASE_URL = "https://totobin-kiosk-[your-id]-uc.a.run.app"
   API_KEY = "totobin-hardware-secret-2024"
   ```

3. **ทดสอบ Hardware Communication**
   - ส่ง test request ไป `/api/hardware/status`
   - ตรวจสอบว่าได้รับ response ถูกต้อง

---

## 🎉 ตรวจสอบว่า Deploy สำเร็จ

**เมื่อทุกอย่างเสร็จแล้ว คุณจะได้:**
- ✅ TotoBin Kiosk ทำงานบน Cloud Run
- ✅ URL สำหรับเข้าใช้งาน
- ✅ Queue system พร้อมใช้งาน
- ✅ Hardware API พร้อมรับคำสั่ง
- ✅ PromptPay QR Code generation
- ✅ Real-time updates ทำงาน

**🚀 TotoBin Kiosk พร้อมใช้งานจริงแล้ว!**

---

## 🆘 Troubleshooting

### หาก Build ล้มเหลว:
- ตรวจสอบ `package.json` ในโฟลเดอร์ `kiosk-app`
- ตรวจสอบว่ามี `Dockerfile` หรือใช้ Buildpacks
- ดู Build logs ใน Cloud Build Console

### หาก Service ไม่ทำงาน:
- ตรวจสอบ Environment Variables
- ดู Service logs ใน Cloud Run Console
- ตรวจสอบ PORT configuration

### หาก Hardware ไม่เชื่อมต่อได้:
- ตรวจสอบ API_KEY ใน hardware clients
- ตรวจสอบ URL endpoint ถูกต้อง
- ทดสอบ API ด้วย Postman หรือ curl

**หากมีปัญหาอื่น ๆ สามารถดู Logs ใน Google Cloud Console หรือติดต่อสอบถามเพิ่มเติมได้**