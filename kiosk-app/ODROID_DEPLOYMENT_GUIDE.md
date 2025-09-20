# ODROID C4 Deployment Guide

## 🚀 Complete Next.js Deployment on ODROID C4

### Prerequisites
- ODROID C4 with Ubuntu 20.04+ 
- Internet connection
- Domain `porametix.online` pointing to your ODROID's IP
- At least 2GB RAM and 16GB storage

## 📋 Quick Deployment (Recommended)

### 1. Initial Setup (One-time)
```bash
# Download and run setup script
wget https://raw.githubusercontent.com/PorametKeawubol/ToTobinProject/main/kiosk-app/deployment/odroid-setup.sh
chmod +x odroid-setup.sh
./odroid-setup.sh
```

### 2. Deploy Application
```bash
# Quick deployment
wget https://raw.githubusercontent.com/PorametKeawubol/ToTobinProject/main/kiosk-app/deployment/deploy-odroid.sh
chmod +x deploy-odroid.sh
./deploy-odroid.sh
```

## 🔧 Manual Setup (Advanced)

### 1. Install Dependencies
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
sudo apt install -y docker.io docker-compose
sudo systemctl enable docker
sudo usermod -aG docker $USER

# Install Nginx and SSL tools
sudo apt install -y nginx certbot python3-certbot-nginx
```

### 2. Clone Repository
```bash
# Create app directory
sudo mkdir -p /opt/totobin
sudo chown $USER:$USER /opt/totobin
cd /opt/totobin

# Clone project
git clone https://github.com/PorametKeawubol/ToTobinProject.git
cd ToTobinProject/kiosk-app
```

### 3. Configure Environment
```bash
# Copy optimized config for ODROID
cp next.config.odroid.ts next.config.ts

# Create production environment
cat > .env.production << EOF
NODE_ENV=production
PORT=3000
HARDWARE_API_KEY=your-secure-key
DATABASE_URL=file:./data/totobin.db
NEXTAUTH_URL=https://porametix.online
NEXTAUTH_SECRET=$(openssl rand -base64 32)
EOF
```

### 4. Deploy with Docker
```bash
# Build and run optimized for ODROID
docker-compose -f docker-compose.odroid.yml up -d --build
```

### 5. Configure Nginx Reverse Proxy
```bash
# Create Nginx config
sudo nano /etc/nginx/sites-available/totobin-kiosk

# Add configuration:
server {
    listen 80;
    server_name porametix.online www.porametix.online;
    
    location / {
        proxy_pass http://localhost:3000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}

# Enable site
sudo ln -s /etc/nginx/sites-available/totobin-kiosk /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx
```

### 6. Setup SSL Certificate
```bash
# Get Let's Encrypt certificate
sudo certbot --nginx -d porametix.online -d www.porametix.online
```

## 📊 Performance Optimizations for ODROID C4

### Memory Optimization
```bash
# Set memory limits in docker-compose.odroid.yml
mem_limit: 768m
memswap_limit: 768m
cpus: '2.0'
```

### Node.js Optimization
```bash
# In .env.production
NODE_OPTIONS=--max-old-space-size=512
UV_THREADPOOL_SIZE=4
```

### System Optimization
```bash
# Increase swap if needed
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

## 🔍 Monitoring & Maintenance

### Check Application Status
```bash
# View container status
docker-compose -f docker-compose.odroid.yml ps

# View logs
docker-compose -f docker-compose.odroid.yml logs -f

# Health check
curl http://localhost:3000/api/health
```

### Resource Monitoring
```bash
# System resources
htop

# Docker stats
docker stats

# Disk usage
df -h
```

### Update Application
```bash
cd /opt/totobin/ToTobinProject/kiosk-app
git pull origin main
docker-compose -f docker-compose.odroid.yml up -d --build
```

## 🚨 Troubleshooting

### Common Issues

**1. Out of Memory**
```bash
# Check memory usage
free -h

# Restart container with memory limits
docker-compose -f docker-compose.odroid.yml restart
```

**2. Docker Build Fails**
```bash
# Clean Docker cache
docker system prune -f
docker-compose -f docker-compose.odroid.yml build --no-cache
```

**3. SSL Certificate Issues**
```bash
# Renew certificate
sudo certbot renew

# Test certificate
sudo certbot certificates
```

**4. Application Won't Start**
```bash
# Check environment variables
docker-compose -f docker-compose.odroid.yml config

# View detailed logs
docker-compose -f docker-compose.odroid.yml logs --tail=100
```

## 📱 Hardware Integration

### Connect ESP32/ODROID Hardware
```bash
# Install hardware client
cd /opt/totobin/ToTobinProject/hardware/odroid
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# Run hardware service
sudo systemctl enable totobin-hardware
sudo systemctl start totobin-hardware
```

## 🌐 Access Your Application

- **Local**: http://localhost:3000
- **Domain**: https://porametix.online
- **Health Check**: https://porametix.online/api/health

## 📈 Performance Tips

1. **Use SSD storage** for better I/O performance
2. **Enable Gzip compression** in Nginx
3. **Monitor memory usage** regularly
4. **Use CDN** for static assets if needed
5. **Keep system updated** for security

## 🔒 Security Best Practices

1. **Enable UFW firewall**
```bash
sudo ufw enable
sudo ufw allow ssh
sudo ufw allow 'Nginx Full'
```

2. **Install fail2ban**
```bash
sudo apt install fail2ban
```

3. **Regular security updates**
```bash
sudo apt update && sudo apt upgrade -y
```

4. **Use strong passwords** and consider SSH key authentication

## 📞 Support

If you encounter issues:
1. Check the logs: `docker-compose logs`
2. Verify domain DNS settings
3. Ensure ODROID has sufficient resources
4. Check firewall settings

Happy deploying! 🎉