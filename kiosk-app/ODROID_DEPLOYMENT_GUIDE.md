# การ Deploy Next.js Kiosk App บน Odroid C4# TotoBin Kiosk - ODROID C4 Deployment Guide



## ข้อมูลพื้นฐาน## 🚀 Quick Deployment

- **โดเมน**: `porametix.online`

- **เซิร์ฟเวอร์**: Odroid C4 (ARM64)Deploy TotoBin Kiosk on ODROID C4 with domain `porametix.online` using yarn for better ARM performance.

- **สถาปัตยกรรม**: Docker + Nginx + SSL

### Prerequisites

## ขั้นตอนการติดตั้ง- ODROID C4 with Ubuntu 20.04+ or Debian 11+

- Root access

### 1. เตรียมความพร้อม- Internet connection

- Domain `porametix.online` pointing to your ODROID IP

#### ติดตั้ง OS และ dependencies

```bash### One-Command Deployment

# อัพเดทระบบ

sudo apt update && sudo apt upgrade -y```bash

# Quick deployment (recommended)

# ติดตั้ง tools ที่จำเป็นwget -O- https://raw.githubusercontent.com/PorametKeawubol/ToTobinProject/main/kiosk-app/deployment/quick-odroid-deploy.sh | sudo bash

sudo apt install -y git curl wget nano

```# Or clone and run locally

git clone https://github.com/PorametKeawubol/ToTobinProject.git

#### ตั้งค่า DNScd ToTobinProject/kiosk-app/deployment

ให้แน่ใจว่าโดเมน `porametix.online` ชี้ไปที่ IP ของ Odroid C4sudo bash quick-odroid-deploy.sh

```

### 2. Clone โปรเจค

### Full Deployment (with monitoring)

```bash

# Clone repository```bash

git clone <your-repo-url>sudo bash odroid-deploy.sh

cd kiosk-app```



# ให้สิทธิ์ execute กับ scripts## 📊 Performance Monitoring

chmod +x deploy-odroid.sh update.sh

``````bash

# Check system status

### 3. Deploy แบบ Automatedbash monitor-odroid.sh



```bash# Real-time monitoring

# รัน deploy scriptwatch -n 5 'bash monitor-odroid.sh'

./deploy-odroid.sh

```# Check application logs

sudo journalctl -fu totobin-kiosk

สคริปต์จะทำการ:```

- ติดตั้ง Docker และ Docker Compose

- สร้าง directory structure## 🎯 What Gets Installed

- สร้างไฟล์ `.env.production`

- Build Docker images### Software Stack

- เริ่ม services ทั้งหมด- **Node.js 18.x** (optimized for ARM64)

- **yarn** (faster than npm on ARM)

### 4. ตั้งค่า Environment Variables- **nginx** (reverse proxy with SSL)

- **certbot** (automatic SSL certificates)

แก้ไขไฟล์ `.env.production`:

### Services

```bash- **totobin-kiosk.service** - Main application

nano .env.production- **nginx** - Web server with SSL termination

```

### File Structure

```env```

# Next.js Environment Variables/opt/totobin-kiosk/

NODE_ENV=production├── kiosk-app/              # Next.js application

HOSTNAME=0.0.0.0│   ├── .env.production     # Production environment

PORT=3000│   ├── .next/              # Built application

│   └── node_modules/       # Dependencies

# Application Variables└── .git/                   # Git repository

GOOGLE_CLOUD_PROJECT_ID=your-actual-project-id```

GOOGLE_CLOUD_REGION=asia-southeast1

HARDWARE_API_KEY=your-hardware-api-key## ⚙️ Configuration

JWT_SECRET=your-super-secret-jwt-key

PROMPTPAY_PHONE=0123456789### Environment Variables

BUSINESS_NAME=TotoBin KioskThe deployment creates `/opt/totobin-kiosk/kiosk-app/.env.production`:

```

```bash

### 5. ตั้งค่า SSL CertificateNODE_ENV=production

PORT=3000

แก้ไข email ใน `docker-compose.prod.yml`:NEXT_TELEMETRY_DISABLED=1

DATABASE_URL="file:./dev.db"

```bashHARDWARE_API_KEY="prod-[random]"

nano docker-compose.prod.ymlKIOSK_ID="odroid-kiosk-001"

```NEXTAUTH_URL="https://porametix.online"

NEXTAUTH_SECRET="[random]"

เปลี่ยน `your-email@example.com` เป็น email จริงของคุณ```



### 6. Restart Services### Nginx Configuration

- **HTTP (port 80)**: Redirects to HTTPS

```bash- **HTTPS (port 443)**: Proxies to Next.js on port 3000

# Restart เพื่อให้ environment variables มีผล- **SSL**: Automatic Let's Encrypt certificates

docker-compose -f docker-compose.prod.yml restart- **Security headers**: XSS protection, HSTS, etc.



# ตั้งค่า SSL certificate## 🔧 Performance Optimizations

docker-compose -f docker-compose.prod.yml restart certbot

```### ODROID C4 Specific

- **CPU Governor**: Set to `performance` for better response times

## การตรวจสอบ- **Memory**: Optimized swappiness for SSD/eMMC

- **Process Limits**: Increased for Node.js

### ตรวจสอบว่า services ทำงาน- **yarn**: Used instead of npm for faster installs



```bash### Application Optimizations

# ดู status ของ containers- **Static Caching**: 1-year cache for `_next/static/`

docker-compose -f docker-compose.prod.yml ps- **Gzip Compression**: Enabled in nginx

- **HTTP/2**: Enabled for better performance

# ดู logs- **Keep-Alive**: Optimized connection handling

docker-compose -f docker-compose.prod.yml logs -f

```## 🛠️ Management Commands



### ทดสอบ website### Application Management

```bash

1. **HTTP**: `http://porametix.online`# Restart application

2. **HTTPS**: `https://porametix.online`sudo systemctl restart totobin-kiosk

3. **Health Check**: `https://porametix.online/api/health`

# Check status

## การจัดการsudo systemctl status totobin-kiosk



### อัพเดทแอปพลิเคชัน# View logs

sudo journalctl -fu totobin-kiosk

```bash

# วิธีง่าย# Stop/start

./update.shsudo systemctl stop totobin-kiosk

sudo systemctl start totobin-kiosk

# หรือทำเอง```

git pull

docker-compose -f docker-compose.prod.yml build --no-cache kiosk-app### Updates

docker-compose -f docker-compose.prod.yml up -d```bash

```# Update application

cd /opt/totobin-kiosk

### คำสั่งที่ใช้บ่อยsudo git pull

cd kiosk-app

```bashsudo yarn install

# ดู logssudo yarn build

docker-compose -f docker-compose.prod.yml logs -f [service-name]sudo systemctl restart totobin-kiosk

```

# Restart service

docker-compose -f docker-compose.prod.yml restart [service-name]### Nginx Management

```bash

# Stop ทุกอย่าง# Test configuration

docker-compose -f docker-compose.prod.yml downsudo nginx -t



# Start ทุกอย่าง# Reload configuration

docker-compose -f docker-compose.prod.yml up -dsudo systemctl reload nginx



# ลบ containers และ rebuild# Restart nginx

docker-compose -f docker-compose.prod.yml downsudo systemctl restart nginx

docker-compose -f docker-compose.prod.yml build --no-cache```

docker-compose -f docker-compose.prod.yml up -d

```## 🔍 Troubleshooting



### การติดตาม### Common Issues



```bash#### 1. Application won't start

# ดู resource usage```bash

docker stats# Check service status

sudo systemctl status totobin-kiosk

# ตรวจสอบ disk space

df -h# Check logs

sudo journalctl -u totobin-kiosk --lines=50

# ตรวจสอบ memory

free -h# Check port availability

sudo ss -tlnp | grep :3000

# ดู process ที่ใช้ CPU มาก```

top

```#### 2. SSL certificate issues

```bash

## การแก้ไขปัญหา# Renew certificate manually

sudo certbot renew

### แอปไม่ตอบสนอง

# Test certificate

```bashsudo certbot certificates

# ตรวจสอบ container status```

docker-compose -f docker-compose.prod.yml ps

#### 3. High memory usage

# ดู logs ว่ามี error อะไร```bash

docker-compose -f docker-compose.prod.yml logs kiosk-app# Check memory usage

docker-compose -f docker-compose.prod.yml logs nginxfree -h



# Restart services# Restart application to free memory

docker-compose -f docker-compose.prod.yml restartsudo systemctl restart totobin-kiosk

``````



### SSL ไม่ทำงาน#### 4. Slow performance

```bash

```bash# Check CPU governor

# ตรวจสอบว่าโดเมนชี้ถูกต้องcat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

nslookup porametix.online

# Set to performance mode

# ลอง renew certificateecho 'performance' | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

docker-compose -f docker-compose.prod.yml run --rm certbot renew```



# ตรวจสอบ nginx config### Performance Monitoring

docker-compose -f docker-compose.prod.yml exec nginx nginx -t```bash

```# CPU temperature

cat /sys/class/thermal/thermal_zone0/temp

### Performance ต่ำ

# Memory usage

```bashfree -h

# ตรวจสอบ resource usage

docker stats# Disk usage

df -h

# ดู system load

htop# Network connections

sudo ss -tlnp | grep :3000

# เพิ่ม swap ถ้าจำเป็น```

sudo swapon --show

```## 📈 Expected Performance



## การสำรองข้อมูล### ODROID C4 Specifications

- **CPU**: Amlogic S905X3 (Quad-core ARM Cortex-A55)

```bash- **RAM**: 4GB LPDDR4

# สำรอง environment variables- **Network**: Gigabit Ethernet

cp .env.production .env.production.backup- **Storage**: eMMC/microSD



# สำรอง SSL certificates### Performance Expectations

sudo tar -czf ssl-backup.tar.gz /var/lib/docker/volumes/kiosk-app_certbot-etc/- **Response Time**: < 100ms for static pages

- **API Response**: < 200ms for simple queries

# สำรอง database (ถ้ามี)- **Memory Usage**: ~300-500MB for Next.js app

# docker-compose -f docker-compose.prod.yml exec db mysqldump...- **CPU Usage**: ~5-15% during normal operation

```

## 🌐 URLs and Endpoints

## ข้อแนะนำการปรับแต่ง

After successful deployment:

### สำหรับ Odroid C4 (ARM64)

- **Main Application**: https://porametix.online

1. **Memory Management**: ใช้ swap file ถ้า RAM ไม่พอ- **Hardware API**: https://porametix.online/api/hardware

2. **Docker**: ใช้ `--memory` flag เพื่อจำกัด memory usage- **Health Check**: https://porametix.online/api/hardware/current-status

3. **Build Time**: อาจใช้เวลานานกว่าปกติ เพราะเป็น ARM architecture- **Queue API**: https://porametix.online/api/queue



### การ Optimize## 🔐 Security Features



```bash- **SSL/TLS**: Automatic Let's Encrypt certificates

# ตั้งค่า Docker ให้ใช้ memory น้อยลง- **Security Headers**: XSS protection, HSTS, content type sniffing prevention

echo '{"log-driver":"json-file","log-opts":{"max-size":"10m","max-file":"3"}}' | sudo tee /etc/docker/daemon.json- **API Authentication**: Hardware API key protection

sudo systemctl restart docker- **Process Isolation**: Non-root application execution

```- **Firewall Ready**: Only ports 80, 443, and 22 (SSH) needed



## การติดต่อ Support## 📝 Logs and Monitoring



หากมีปัญหา:### Log Locations

1. ตรวจสอบ logs ก่อน- **Application**: `sudo journalctl -u totobin-kiosk`

2. ลอง restart services- **Nginx**: `/var/log/nginx/access.log`, `/var/log/nginx/error.log`

3. ตรวจสอบ disk space และ memory- **System**: `/var/log/syslog`

4. ดู health check endpoint: `/api/health`

### Monitoring Script

---Run `bash monitor-odroid.sh` for comprehensive system status including:

- CPU usage and temperature

**เสร็จแล้ว!** เว็บไซต์ของคุณควรจะทำงานที่ `https://porametix.online`- Memory usage
- Service status
- Application health
- Performance recommendations

## 🚀 Hardware Integration

The deployment automatically sets up:
- Hardware API endpoints at `/api/hardware/*`
- Authentication for hardware devices
- GPIO control integration (if hardware client is deployed)
- LED status monitoring endpoints

To deploy the hardware client on the same ODROID:
```bash
# Install Python dependencies for hardware control
sudo apt install python3-pip python3-rpi.gpio
cd /opt/totobin-kiosk/hardware/odroid
sudo python3 odroid_client.py
```

## 💡 Tips for ODROID C4

1. **Use eMMC storage** for better performance than microSD
2. **Enable performance governor** for consistent response times
3. **Monitor temperature** during heavy loads
4. **Use yarn** instead of npm for faster package management
5. **Regular updates** to keep dependencies current
