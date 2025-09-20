# TotoBin ODROID C4 Fresh Deployment Guide

## 🚀 การติดตั้งใหม่หมดทั้งหมด

สำหรับ ODROID C4 ที่ต้องการเริ่มต้นใหม่หมดทั้งหมด

### 📋 สิ่งที่ต้องเตรียม:
- ODROID C4 ที่ติดตั้ง Ubuntu 20.04+ แล้ว
- Internet connection
- อย่างน้อย 2GB RAM และ 16GB storage
- Domain `porametix.online` (optional)

### 🛠️ วิธีติดตั้ง (1 คำสั่งเดียว):

```bash
# ดาวน์โหลดและรัน deployment script
wget https://raw.githubusercontent.com/PorametKeawubol/ToTobinProject/main/kiosk-app/deployment/fresh-odroid-deploy.sh
chmod +x fresh-odroid-deploy.sh
./fresh-odroid-deploy.sh
```

### ✅ สิ่งที่ script จะทำ:

1. **System Setup:**
   - ติดตั้ง Node.js 18 LTS
   - ติดตั้ง PM2 Process Manager  
   - ติดตั้ง Nginx

2. **Application Setup:**
   - Clone TotoBin repository
   - สร้าง production environment
   - Build application สำหรับ ODROID
   - ติดตั้ง dependencies

3. **Service Configuration:**
   - สร้าง systemd service
   - Configure Nginx reverse proxy
   - Setup PM2 auto-restart

4. **Management Tools:**
   - สร้าง status, update, restart scripts
   - Setup logging system

### 🎯 ผลลัพธ์ที่ได้:

- **Application URL**: http://localhost:3000
- **Health Check**: http://localhost:3000/api/health
- **Domain**: http://porametix.online (ถ้า DNS configured)

### 🔧 คำสั่งจัดการ:

```bash
cd ~/totobin-app/kiosk-app

# ดูสถานะ
./status.sh

# อัปเดต application
./update.sh

# Restart application
./restart.sh

# ดู logs
pm2 logs

# ดูสถานะ PM2
pm2 status
```

### 📊 Performance ที่คาดหวัง:

- **RAM Usage**: ~300-400MB
- **Startup Time**: 15-20 วินาที
- **Response Time**: < 200ms
- **Uptime**: 99.9%+

### 🔒 SSL Setup (Optional):

```bash
# ติดตั้ง SSL certificate
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d porametix.online
```

### 🌐 Domain Setup:

1. Point DNS A record ของ `porametix.online` ไปยัง IP ของ ODROID
2. รอ DNS propagation (5-30 นาที)
3. ทดสอบ: http://porametix.online

### 🔍 Troubleshooting:

**Application ไม่เริ่ม:**
```bash
pm2 logs
pm2 restart totobin-kiosk
```

**Memory issues:**
```bash
free -h
pm2 monit
```

**Nginx issues:**
```bash
sudo nginx -t
sudo systemctl status nginx
```

### 📈 Optimization Tips:

1. **ใช้ SSD** สำหรับ storage
2. **Monitor memory** เป็นประจำ
3. **Update system** สม่ำเสมอ
4. **Backup** database และ environment files

### 🎉 หลังจากติดตั้งเสร็จ:

1. ทดสอบทุกฟีเจอร์ของแอพ
2. Setup SSL certificate
3. Configure hardware integration
4. Setup monitoring และ backup

---

**หมายเหตุ**: Script นี้จะลบการติดตั้งเก่าทั้งหมดและเริ่มใหม่ เพื่อให้แน่ใจว่าไม่มีปัญหาจากการติดตั้งก่อนหน้า