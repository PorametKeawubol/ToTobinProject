# แก้ไขปัญหา Docker บน Odroid C4

## ปัญหาที่พบบ่อย

### 1. Error: "Not supported URL scheme http+docker"

**สาเหตุ**: Docker service ไม่ทำงาน หรือ permission ไม่ถูกต้อง

**วิธีแก้**:
```bash
# แก้ไข Docker permissions
chmod +x fix-docker.sh
./fix-docker.sh

# หรือทำเอง
sudo systemctl start docker
sudo chmod 666 /var/run/docker.sock
sudo usermod -aG docker $USER
```

### 2. Permission denied

**วิธีแก้**:
```bash
# ใช้สคริปต์ deploy แบบง่าย
chmod +x deploy-simple.sh
./deploy-simple.sh
```

### 3. Docker Compose เวอร์ชันเก่า

**ตรวจสอบเวอร์ชัน**:
```bash
docker-compose --version
```

**อัพเดท Docker Compose**:
```bash
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
```

### 4. Memory ไม่เพียงพอ

**เพิ่ม Swap**:
```bash
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

### 5. Build ช้าเกินไป

**ใช้ multi-stage build**:
- ไฟล์ Dockerfile ได้ปรับให้ใช้ multi-stage แล้ว
- ใช้ `--no-cache` เฉพาะเมื่อจำเป็น

## สคริปต์ที่มีให้ใช้

| สคริปต์ | วัตถุประสงค์ |
|---------|-------------|
| `deploy-simple.sh` | Deploy แบบง่าย จัดการ permissions เอง |
| `fix-docker.sh` | แก้ไขปัญหา Docker permissions |
| `test-setup.sh` | ทดสอบความพร้อมก่อน deploy |
| `update.sh` | อัพเดท app |
| `backup.sh` | สำรองข้อมูล |

## การใช้งานแนะนำ

### วิธีที่ 1: Deploy แบบง่าย (แนะนำ)
```bash
chmod +x deploy-simple.sh
./deploy-simple.sh
```

### วิธีที่ 2: แก้ปัญหาแล้ว deploy
```bash
chmod +x fix-docker.sh
./fix-docker.sh

chmod +x deploy-odroid.sh  
./deploy-odroid.sh
```

### วิธีที่ 3: Manual
```bash
# แก้ Docker
sudo systemctl start docker
sudo chmod 666 /var/run/docker.sock

# Build และ run
docker-compose -f docker-compose.prod.yml build
docker-compose -f docker-compose.prod.yml up -d
```

## ตรวจสอบ Status

```bash
# ดู services
docker-compose -f docker-compose.prod.yml ps

# ดู logs
docker-compose -f docker-compose.prod.yml logs -f

# ดู resource usage
docker stats

# Health check
curl http://localhost:3000/api/health
```

## การ Monitor

```bash
# ดู CPU/Memory
htop

# ดู disk space  
df -h

# ดู Docker logs
docker-compose -f docker-compose.prod.yml logs --tail=100 kiosk-app
```