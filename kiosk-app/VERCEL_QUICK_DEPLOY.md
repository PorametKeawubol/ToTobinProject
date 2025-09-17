# 🚀 Deploy TotoBin ผ่าน Vercel Dashboard

## 📱 ขั้นตอนการ Deploy (5 นาทีเสร็จ!)

### 🎯 **ขั้นตอนที่ 1: เข้า Vercel และ Import Repository**

1. **เปิดเบราว์เซอร์ไปที่:** https://vercel.com/
2. **คลิก "Continue with GitHub"** (หรือ Login ด้วย GitHub)
3. **คลิก "Add New Project"**
4. **ค้นหา repository:** `PorametKeawubol/ToTobinProject`
5. **คลิก "Import"** ข้าง repository นี้

### 🔧 **ขั้นตอนที่ 2: Configure Project Settings**

**Project Configuration:**
```
Project Name: totobin-kiosk
Framework Preset: Next.js
Root Directory: kiosk-app/                 ← สำคัญ! ต้องเลือกโฟลเดอร์นี้
Build Command: npm run build               ← อัตโนมัติ
Output Directory: .next                    ← อัตโนมัติ  
Install Command: npm install               ← อัตโนมัติ
Node.js Version: 18.x                      ← แนะนำ
```

### 🌍 **ขั้นตอนที่ 3: ตั้งค่า Environment Variables**

คลิก **"Environment Variables"** แล้วเพิ่ม:

```
NODE_ENV
Value: production

HARDWARE_API_KEY  
Value: totobin-hardware-secret-2024

PROMPTPAY_PHONE
Value: 0984435255

BUSINESS_NAME
Value: TotoBin Kiosk

ENABLE_HARDWARE_INTEGRATION
Value: true

ENABLE_QUEUE_SYSTEM  
Value: true

ENABLE_REAL_TIME_UPDATES
Value: true
```

**การเพิ่ม Environment Variable:**
1. พิมพ์ชื่อใน "Name"
2. พิมพ์ค่าใน "Value" 
3. เลือก "Production, Preview, Development"
4. คลิก "Add"
5. ทำซ้ำ 7 ครั้ง

### 🚀 **ขั้นตอนที่ 4: Deploy!**

1. **คลิก "Deploy"**
2. **รอ 2-3 นาที** ให้ Vercel build และ deploy
3. **เสร็จแล้ว!** จะได้ URL: `https://totobin-kiosk-[random].vercel.app`

---

## 🎯 **URL ที่จะได้หลัง Deploy:**

```
https://totobin-kiosk-[random-id].vercel.app
```

**ตัวอย่าง URL:**
- `https://totobin-kiosk-9f2x8k1m.vercel.app`
- `https://totobin-kiosk-git-main-porametkeawubol.vercel.app`

---

## ✅ **ตรวจสอบหลัง Deploy สำเร็จ:**

### 1. **หน้าแรก TotoBin Kiosk**
- เปิด URL ที่ได้
- ต้องเห็นหน้า TotoBin Kiosk
- มีเมนูเครื่องดื่ม

### 2. **ทดสอบ Features:**
- ✅ เพิ่มเครื่องดื่มลงตะกร้า
- ✅ สร้าง PromptPay QR Code
- ✅ ระบบ Queue ทำงาน
- ✅ หน้า `/queue` แสดงลำดับ

### 3. **ทดสอบ API Endpoints:**
```
GET  [URL]/api/menu
GET  [URL]/api/hardware/orders  
POST [URL]/api/hardware/status
GET  [URL]/api/events/queue
```

---

## 🌐 **ตั้งค่า Custom Domain (porametix.online)**

**หลังจาก deploy สำเร็จ:**

1. **ใน Vercel Dashboard:**
   - ไปที่ Project → **Settings** → **Domains**
   - คลิก **"Add Domain"**
   - ใส่: `porametix.online`

2. **ตั้งค่า DNS ที่ Domain Provider:**
   ```
   Type: CNAME
   Name: @
   Value: cname.vercel-dns.com
   
   Type: CNAME
   Name: www  
   Value: cname.vercel-dns.com
   ```

3. **รอ DNS propagate:** 15-30 นาที

---

## 🔥 **ข้อดีของ Vercel:**

- ✅ **รวดเร็ว:** Deploy ใน 3 นาที
- ✅ **ฟรี:** Hobby plan เพียงพอ
- ✅ **Auto Deploy:** Push code = Auto deploy ใหม่
- ✅ **Global CDN:** เร็วทั่วโลก
- ✅ **SSL ฟรี:** HTTPS อัตโนมัติ
- ✅ **Zero Config:** Next.js ทำงานได้ทันที

---

## 🎉 **ผลลัพธ์สุดท้าย:**

**หลังจาก deploy เสร็จ คุณจะได้:**
- 🏪 **TotoBin Kiosk** online และใช้งานได้
- 🔗 **Production URL** สำหรับใช้งานจริง
- 💳 **PromptPay QR** เบอร์ 0984435255
- 🤖 **Hardware API** สำหรับ ESP32/Odroid
- 📱 **Responsive Design** ใช้งานได้บนมือถือ
- ⚡ **Real-time Queue** อัพเดทแบบ live

**🚀 TotoBin Kiosk พร้อมใช้งานจริงใน 5 นาที!**