# Environment Variables สำหรับ TotoBin Kiosk Cloud Run

## ❗ สำคัญ: แก้ไขปัญหา Container ไม่ start

หากเจอ error "container failed to start and listen on port" ให้:

### 1. เปลี่ยน Build Type เป็น Dockerfile
- Build Type: **Dockerfile** 
- Source location: `/Dockerfile`
- Build context: `/kiosk-app/`

### 2. หรือแก้ไข Buildpacks Entrypoint
- Entrypoint: `node server.js` (แทน `npm start`)

### 3. ตั้งค่า Container
- Container port: **3000**
- CPU allocation: **CPU is only allocated during request processing**
- Memory: **1 GiB**
- Request timeout: **300 seconds**

## ตัวแปรที่ต้องตั้งใน Google Cloud Console:

### 1. NODE_ENV
```
Name: NODE_ENV
Value: production
```

### 2. ~~PORT~~ (ข้าม - Google Cloud Run ตั้งค่าให้อัตโนมัติ)
```
ไม่ต้องใส่ PORT เพราะ Google Cloud Run จะตั้งค่าให้เอง
Cloud Run จะใช้ Container port ที่เราระบุไว้ (3000)
```

### 3. HARDWARE_API_KEY
```
Name: HARDWARE_API_KEY
Value: totobin-hardware-secret-2024
```

### 4. PROMPTPAY_PHONE
```
Name: PROMPTPAY_PHONE
Value: 0984435255
```

### 5. BUSINESS_NAME
```
Name: BUSINESS_NAME
Value: TotoBin Kiosk
```

### 6. ENABLE_HARDWARE_INTEGRATION
```
Name: ENABLE_HARDWARE_INTEGRATION
Value: true
```

### 7. ENABLE_QUEUE_SYSTEM
```
Name: ENABLE_QUEUE_SYSTEM
Value: true
```

## ขั้นตอนการเพิ่ม:

1. คลิك "+ ADD VARIABLE"
2. Copy Name และ Value จากด้านบน
3. คลิก "DONE"
4. ทำซ้ำสำหรับทุกตัวแปร (6 ตัว - ไม่รวม PORT)

## ตัวอย่างหน้าจอที่จะเห็น:

```
Environment Variables:
┌─────────────────────────────┬──────────────────────────────┐
│ Name                        │ Value                        │
├─────────────────────────────┼──────────────────────────────┤
│ NODE_ENV                    │ production                   │
│ HARDWARE_API_KEY            │ totobin-hardware-secret-2024 │
│ PROMPTPAY_PHONE             │ 0984435255                   │
│ BUSINESS_NAME               │ TotoBin Kiosk                │
│ ENABLE_HARDWARE_INTEGRATION │ true                         │
│ ENABLE_QUEUE_SYSTEM         │ true                         │
└─────────────────────────────┴──────────────────────────────┘
```

## หมายเหตุ:
- ใส่ Value โดยไม่ต้องใส่เครื่องหมาย quotes ("")
- ตรวจสอบให้แน่ใจว่าไม่มีช่องว่างหน้าหรือหลัง Value
- ตัวแปร HARDWARE_API_KEY ใช้สำหรับ ESP32/Odroid authentication
- **ไม่ต้องใส่ PORT** เพราะ Google Cloud Run จะตั้งค่าให้อัตโนมัติจาก Container port (3000)