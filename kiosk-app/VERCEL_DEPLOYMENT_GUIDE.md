# 🚀 TotoBin Kiosk - Vercel Deployment Guide

## 🎯 วิธีการ Deploy TotoBin ไป Vercel (เร็วและง่าย!)

### เตรียมความพร้อม:
- ✅ GitHub repository: `PorametKeawubol/ToTobinProject`
- ✅ Vercel account (สร้างฟรีที่ vercel.com)
- ✅ Next.js app พร้อม deploy

---

## 🌐 วิธีที่ 1: Deploy ผ่าน Vercel Dashboard

### ขั้นตอนที่ 1: เข้าสู่ Vercel
1. ไปที่: https://vercel.com/
2. คลิก **"Sign up"** หรือ **"Login"**
3. เชื่อมต่อกับ GitHub account

### ขั้นตอนที่ 2: Import Project
1. คลิก **"New Project"**
2. เลือก **"Import Git Repository"**
3. หา repository: `PorametKeawubol/ToTobinProject`
4. คลิก **"Import"**

### ขั้นตอนที่ 3: Configure Project
```
Project Name: totobin-kiosk
Framework Preset: Next.js
Root Directory: kiosk-app/
Build Command: npm run build
Output Directory: .next
Install Command: npm install
```

### ขั้นตอนที่ 4: Environment Variables
ใน Vercel Dashboard ให้เพิ่ม Environment Variables:

```
NODE_ENV = production
HARDWARE_API_KEY = totobin-hardware-secret-2024
PROMPTPAY_PHONE = 0984435255
BUSINESS_NAME = TotoBin Kiosk
ENABLE_HARDWARE_INTEGRATION = true
ENABLE_QUEUE_SYSTEM = true
ENABLE_REAL_TIME_UPDATES = true
```

### ขั้นตอนที่ 5: Deploy
1. คลิก **"Deploy"**
2. รอ 2-3 นาที ให้ build เสร็จ
3. จะได้ URL: `https://totobin-kiosk-[random].vercel.app`

---

## 💻 วิธีที่ 2: Deploy ผ่าน Vercel CLI

### ติดตั้ง Vercel CLI:
```bash
npm install -g vercel
```

### Login และ Deploy:
```bash
# ใน kiosk-app directory
cd E:\EMPROJECT\kiosk-app

# Login to Vercel
vercel login

# Deploy
vercel

# Follow prompts:
# ? Set up and deploy "kiosk-app"? Y
# ? Which scope? [Your username]
# ? Link to existing project? N
# ? What's your project's name? totobin-kiosk
# ? In which directory is your code located? ./
```

### ตั้งค่า Environment Variables ผ่าน CLI:
```bash
vercel env add NODE_ENV
# Value: production

vercel env add HARDWARE_API_KEY
# Value: totobin-hardware-secret-2024

vercel env add PROMPTPAY_PHONE
# Value: 0984435255

vercel env add BUSINESS_NAME
# Value: TotoBin Kiosk

vercel env add ENABLE_HARDWARE_INTEGRATION
# Value: true

vercel env add ENABLE_QUEUE_SYSTEM
# Value: true
```

### Deploy Production:
```bash
vercel --prod
```

---

## 🌐 ตั้งค่า Custom Domain (porametix.online)

### ใน Vercel Dashboard:
1. ไปที่ Project → **Settings** → **Domains**
2. คลิก **"Add Domain"**
3. ใส่: `porametix.online`
4. ตั้งค่า DNS records ตามที่ Vercel บอก:

```
Type: CNAME
Name: @
Value: cname.vercel-dns.com

Type: CNAME  
Name: www
Value: cname.vercel-dns.com
```

---

## 🔧 ทดสอบหลัง Deploy

### 1. ตรวจสอบ URL ทำงาน
```
https://totobin-kiosk-[random].vercel.app
```

### 2. ทดสอบ Features:
- ✅ หน้าแรกโหลดได้
- ✅ เมนูเครื่องดื่มแสดง
- ✅ เพิ่มสินค้าลงตะกร้า
- ✅ สร้าง PromptPay QR
- ✅ Queue system
- ✅ Hardware API endpoints

### 3. ตรวจสอบ API Endpoints:
```
GET  /api/menu
GET  /api/hardware/orders
POST /api/hardware/status
GET  /api/events/queue
```

---

## 🎉 ข้อดีของ Vercel:

1. **รวดเร็ว**: Deploy ใน 2-3 นาที
2. **ฟรี**: มี tier ฟรีที่เพียงพอ
3. **Auto-deploy**: Push code = auto deploy
4. **Global CDN**: เร็วทั่วโลก
5. **Zero Config**: Next.js ทำงานได้ทันที
6. **Custom Domain**: ฟรี SSL certificate

---

## 🚀 Quick Deploy Commands

```bash
# ติดตั้ง Vercel CLI
npm install -g vercel

# เข้าไปในโฟลเดอร์
cd E:\EMPROJECT\kiosk-app

# Deploy ทันที
vercel

# หรือ Deploy production
vercel --prod
```

**🎯 แนะนำ: ใช้ Vercel Dashboard สำหรับครั้งแรก เพราะง่ายและเห็นภาพ!**

---

## 📱 ผลลัพธ์ที่จะได้:

- ✅ TotoBin Kiosk online ใน 3 นาที
- ✅ URL สำหรับใช้งาน
- ✅ Auto-deploy เมื่อ push code ใหม่
- ✅ ความเร็วโหลดสูง
- ✅ SSL certificate ฟรี
- ✅ ใช้งานได้ทันทีบนมือถือ

**Vercel เหมาะสำหรับ Next.js มากกว่า Google Cloud Run สำหรับ project นี้!** 🚀