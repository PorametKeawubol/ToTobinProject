# TotoBin Kiosk - ODROID C4 Deployment Guide

## 🚀 Quick Deployment

Deploy TotoBin Kiosk on ODROID C4 with domain `porametix.online` using yarn for better ARM performance.

### Prerequisites
- ODROID C4 with Ubuntu 20.04+ or Debian 11+
- Root access
- Internet connection
- Domain `porametix.online` pointing to your ODROID IP

### One-Command Deployment

```bash
# Quick deployment (recommended)
wget -O- https://raw.githubusercontent.com/PorametKeawubol/ToTobinProject/main/kiosk-app/deployment/quick-odroid-deploy.sh | sudo bash

# Or clone and run locally
git clone https://github.com/PorametKeawubol/ToTobinProject.git
cd ToTobinProject/kiosk-app/deployment
sudo bash quick-odroid-deploy.sh
```

### Full Deployment (with monitoring)

```bash
sudo bash odroid-deploy.sh
```

## 📊 Performance Monitoring

```bash
# Check system status
bash monitor-odroid.sh

# Real-time monitoring
watch -n 5 'bash monitor-odroid.sh'

# Check application logs
sudo journalctl -fu totobin-kiosk
```

## 🎯 What Gets Installed

### Software Stack
- **Node.js 18.x** (optimized for ARM64)
- **yarn** (faster than npm on ARM)
- **nginx** (reverse proxy with SSL)
- **certbot** (automatic SSL certificates)

### Services
- **totobin-kiosk.service** - Main application
- **nginx** - Web server with SSL termination

### File Structure
```
/opt/totobin-kiosk/
├── kiosk-app/              # Next.js application
│   ├── .env.production     # Production environment
│   ├── .next/              # Built application
│   └── node_modules/       # Dependencies
└── .git/                   # Git repository
```

## ⚙️ Configuration

### Environment Variables
The deployment creates `/opt/totobin-kiosk/kiosk-app/.env.production`:

```bash
NODE_ENV=production
PORT=3000
NEXT_TELEMETRY_DISABLED=1
DATABASE_URL="file:./dev.db"
HARDWARE_API_KEY="prod-[random]"
KIOSK_ID="odroid-kiosk-001"
NEXTAUTH_URL="https://porametix.online"
NEXTAUTH_SECRET="[random]"
```

### Nginx Configuration
- **HTTP (port 80)**: Redirects to HTTPS
- **HTTPS (port 443)**: Proxies to Next.js on port 3000
- **SSL**: Automatic Let's Encrypt certificates
- **Security headers**: XSS protection, HSTS, etc.

## 🔧 Performance Optimizations

### ODROID C4 Specific
- **CPU Governor**: Set to `performance` for better response times
- **Memory**: Optimized swappiness for SSD/eMMC
- **Process Limits**: Increased for Node.js
- **yarn**: Used instead of npm for faster installs

### Application Optimizations
- **Static Caching**: 1-year cache for `_next/static/`
- **Gzip Compression**: Enabled in nginx
- **HTTP/2**: Enabled for better performance
- **Keep-Alive**: Optimized connection handling

## 🛠️ Management Commands

### Application Management
```bash
# Restart application
sudo systemctl restart totobin-kiosk

# Check status
sudo systemctl status totobin-kiosk

# View logs
sudo journalctl -fu totobin-kiosk

# Stop/start
sudo systemctl stop totobin-kiosk
sudo systemctl start totobin-kiosk
```

### Updates
```bash
# Update application
cd /opt/totobin-kiosk
sudo git pull
cd kiosk-app
sudo yarn install
sudo yarn build
sudo systemctl restart totobin-kiosk
```

### Nginx Management
```bash
# Test configuration
sudo nginx -t

# Reload configuration
sudo systemctl reload nginx

# Restart nginx
sudo systemctl restart nginx
```

## 🔍 Troubleshooting

### Common Issues

#### 1. Application won't start
```bash
# Check service status
sudo systemctl status totobin-kiosk

# Check logs
sudo journalctl -u totobin-kiosk --lines=50

# Check port availability
sudo ss -tlnp | grep :3000
```

#### 2. SSL certificate issues
```bash
# Renew certificate manually
sudo certbot renew

# Test certificate
sudo certbot certificates
```

#### 3. High memory usage
```bash
# Check memory usage
free -h

# Restart application to free memory
sudo systemctl restart totobin-kiosk
```

#### 4. Slow performance
```bash
# Check CPU governor
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Set to performance mode
echo 'performance' | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Performance Monitoring
```bash
# CPU temperature
cat /sys/class/thermal/thermal_zone0/temp

# Memory usage
free -h

# Disk usage
df -h

# Network connections
sudo ss -tlnp | grep :3000
```

## 📈 Expected Performance

### ODROID C4 Specifications
- **CPU**: Amlogic S905X3 (Quad-core ARM Cortex-A55)
- **RAM**: 4GB LPDDR4
- **Network**: Gigabit Ethernet
- **Storage**: eMMC/microSD

### Performance Expectations
- **Response Time**: < 100ms for static pages
- **API Response**: < 200ms for simple queries
- **Memory Usage**: ~300-500MB for Next.js app
- **CPU Usage**: ~5-15% during normal operation

## 🌐 URLs and Endpoints

After successful deployment:

- **Main Application**: https://porametix.online
- **Hardware API**: https://porametix.online/api/hardware
- **Health Check**: https://porametix.online/api/hardware/current-status
- **Queue API**: https://porametix.online/api/queue

## 🔐 Security Features

- **SSL/TLS**: Automatic Let's Encrypt certificates
- **Security Headers**: XSS protection, HSTS, content type sniffing prevention
- **API Authentication**: Hardware API key protection
- **Process Isolation**: Non-root application execution
- **Firewall Ready**: Only ports 80, 443, and 22 (SSH) needed

## 📝 Logs and Monitoring

### Log Locations
- **Application**: `sudo journalctl -u totobin-kiosk`
- **Nginx**: `/var/log/nginx/access.log`, `/var/log/nginx/error.log`
- **System**: `/var/log/syslog`

### Monitoring Script
Run `bash monitor-odroid.sh` for comprehensive system status including:
- CPU usage and temperature
- Memory usage
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
