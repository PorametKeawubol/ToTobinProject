# 🔧 Quick Fix for Docker Daemon Issues on Odroid C4

## ปัญหา: Docker daemon ไม่สามารถเริ่มทำงานได้

```bash
× docker.service - Docker Application Container Engine
     Active: failed (Result: exit-code)
```

## วิธีแก้ไขด่วน

### วิธีที่ 1: ใช้สคริปต์แก้ไขอัตโนมัติ (แนะนำ)

```bash
# แก้ไข Docker daemon แบบสมบูรณ์
chmod +x fix-docker-complete.sh
./fix-docker-complete.sh

# หรือ deploy พร้อมแก้ไข
chmod +x deploy-odroid-fixed.sh
./deploy-odroid-fixed.sh
```

### วิธีที่ 2: แก้ไขด้วยตนเอง

```bash
# หยุด Docker services
sudo systemctl stop docker.service
sudo systemctl stop docker.socket
sudo systemctl stop containerd

# ลบข้อมูล Docker ที่เสียหาย (ถ้าจำเป็น)
sudo rm -rf /var/lib/docker

# สร้าง Docker config ใหม่
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json << 'EOF'
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  },
  "storage-driver": "overlay2",
  "live-restore": false
}
EOF

# เริ่ม Docker ใหม่
sudo systemctl daemon-reload
sudo systemctl enable docker
sudo systemctl start docker

# ตรวจสอบ
sudo docker info
```

### วิธีที่ 3: ติดตั้ง Docker ใหม่

```bash
# ถอนการติดตั้ง Docker เก่า
sudo apt remove docker docker-engine docker.io containerd runc

# ติดตั้งใหม่
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh
sudo usermod -aG docker $USER

# เริ่มทำงาน
sudo systemctl enable docker
sudo systemctl start docker
```

## ตรวจสอบหลังแก้ไข

```bash
# ตรวจสอบ Docker service
sudo systemctl status docker

# ทดสอบ Docker
sudo docker run hello-world

# ตรวจสอบ Docker Compose
docker-compose --version
```

## ปัญหาที่พบบ่อยและวิธีแก้

### 1. Permission denied
```bash
sudo chmod 666 /var/run/docker.sock
sudo usermod -aG docker $USER
```

### 2. Storage driver issues
```bash
# ใช้ overlay2 storage driver
echo '{"storage-driver": "overlay2"}' | sudo tee /etc/docker/daemon.json
sudo systemctl restart docker
```

### 3. Memory issues
```bash
# เพิ่ม swap space
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

## หลังจากแก้ไขแล้ว

```bash
# Deploy แอป
./deploy-odroid-fixed.sh

# หรือ
./deploy-simple.sh
```

## Commands ที่ใช้ได้

| สคริปต์ | วัตถุประสงค์ |
|---------|-------------|
| `fix-docker-complete.sh` | แก้ไข Docker daemon แบบสมบูรณ์ |
| `deploy-odroid-fixed.sh` | Deploy พร้อมแก้ไข Docker |
| `deploy-simple.sh` | Deploy แบบง่าย |

---

**💡 Tip**: ถ้ายังไม่ได้ ให้ลอง restart Odroid C4 แล้วรันสคริปต์ใหม่