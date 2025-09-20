#!/bin/bash
# TotoBin ODROID C4 Fresh Deployment Script
# เริ่มต้นใหม่หมดทั้งหมด

set -e

echo "🚀 TotoBin ODROID C4 Fresh Deployment"
echo "======================================"

# สี
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# ข้อมูลการติดตั้ง
DOMAIN="porametix.online"
APP_USER="odroid"
APP_DIR="/home/$APP_USER/totobin-app"
SERVICE_NAME="totobin-kiosk"

print_status "Starting fresh deployment for ODROID C4..."
print_status "Domain: $DOMAIN"
print_status "App Directory: $APP_DIR"
print_status "User: $APP_USER"

# ตรวจสอบและติดตั้ง Node.js
print_status "Checking Node.js installation..."
if ! command -v node &> /dev/null; then
    print_status "Installing Node.js 18 LTS..."
    curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
    sudo apt-get install -y nodejs
else
    node_version=$(node --version)
    print_success "Node.js already installed: $node_version"
fi

# ตรวจสอบและติดตั้ง PM2
print_status "Checking PM2 installation..."
if ! command -v pm2 &> /dev/null; then
    print_status "Installing PM2..."
    sudo npm install -g pm2
else
    pm2_version=$(pm2 --version)
    print_success "PM2 already installed: $pm2_version"
fi

# ล้างการติดตั้งเก่า (ถ้ามี)
print_status "Cleaning up previous installation..."
pm2 delete all 2>/dev/null || true
pm2 kill 2>/dev/null || true
sudo rm -rf $APP_DIR
sudo systemctl stop $SERVICE_NAME 2>/dev/null || true
sudo systemctl disable $SERVICE_NAME 2>/dev/null || true
sudo rm -f /etc/systemd/system/$SERVICE_NAME.service

# สร้างโฟลเดอร์แอพ
print_status "Creating application directory..."
mkdir -p $APP_DIR
cd $APP_DIR

# Clone repository
print_status "Cloning TotoBin repository..."
git clone https://github.com/PorametKeawubol/ToTobinProject.git .
cd kiosk-app

# สร้างไฟล์ environment
print_status "Creating production environment file..."
cat > .env.production << EOF
# Production Environment for ODROID C4
NODE_ENV=production
PORT=3000
NEXT_TELEMETRY_DISABLED=1

# Database (SQLite for ODROID)
DATABASE_URL="file:./data/totobin.db"

# API Keys
HARDWARE_API_KEY=odroid-hardware-key-$(date +%s)

# Payment (Test mode)
PROMPTPAY_API_KEY=test_key
PROMPTPAY_WEBHOOK_SECRET=test_secret

# Authentication
NEXTAUTH_URL=https://$DOMAIN
NEXTAUTH_SECRET=$(openssl rand -base64 32)

# Performance optimization
NODE_OPTIONS=--max-old-space-size=1024
UV_THREADPOOL_SIZE=4
EOF

# สร้าง Next.js config ที่เรียบง่าย (JavaScript เท่านั้น)
print_status "Creating optimized Next.js configuration..."
cat > next.config.js << 'EOF'
/** @type {import('next').NextConfig} */
const nextConfig = {
  // Enable standalone for production
  output: "standalone",
  
  // Experimental features
  experimental: {
    serverComponentsExternalPackages: ["@google-cloud/firestore"],
  },
  
  // Performance optimizations
  compiler: {
    removeConsole: process.env.NODE_ENV === "production",
  },
  
  // Image optimization for ODROID
  images: {
    formats: ['image/webp'],
    deviceSizes: [640, 750, 828, 1080],
    imageSizes: [16, 32, 48, 64, 96, 128],
  },
  
  // Environment variables
  env: {
    HARDWARE_API_KEY: process.env.HARDWARE_API_KEY,
    JWT_SECRET: process.env.JWT_SECRET,
    PROMPTPAY_PHONE: process.env.PROMPTPAY_PHONE,
    BUSINESS_NAME: process.env.BUSINESS_NAME,
  },
};

module.exports = nextConfig;
EOF

# ลบ TypeScript config ถ้ามี
rm -f next.config.ts next.config.odroid.ts tsconfig.json

# ติดตั้ง dependencies
print_status "Installing dependencies..."
npm install

# Build application
print_status "Building application for production..."
NODE_OPTIONS="--max-old-space-size=1536" npm run build

# ลบ dev dependencies เพื่อประหยัดพื้นที่
print_status "Cleaning up development dependencies..."
npm prune --production

# สร้างโฟลเดอร์ที่จำเป็น
mkdir -p data logs

# สร้าง PM2 ecosystem config
print_status "Creating PM2 configuration..."
cat > ecosystem.config.js << 'EOF'
module.exports = {
  apps: [{
    name: 'totobin-kiosk',
    script: './node_modules/.bin/next',
    args: 'start',
    cwd: process.cwd(),
    instances: 1,
    exec_mode: 'fork',
    max_memory_restart: '512M',
    env: {
      NODE_ENV: 'production',
      PORT: '3000'
    },
    env_file: '.env.production',
    error_file: './logs/error.log',
    out_file: './logs/output.log',
    log_file: './logs/combined.log',
    time: true,
    autorestart: true,
    max_restarts: 10,
    min_uptime: '10s',
    kill_timeout: 5000,
    restart_delay: 4000
  }]
}
EOF

# เริ่มต้น PM2
print_status "Starting application with PM2..."
pm2 start ecosystem.config.js
pm2 save

# สร้าง systemd service
print_status "Creating systemd service..."
sudo tee /etc/systemd/system/$SERVICE_NAME.service > /dev/null <<EOF
[Unit]
Description=TotoBin Kiosk Application
Documentation=https://pm2.keymetrics.io/
After=network.target

[Service]
Type=forking
User=$APP_USER
LimitNOFILE=infinity
LimitNPROC=infinity
LimitCORE=infinity
Environment=PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
Environment=PM2_HOME=$APP_DIR/.pm2
PIDFile=$APP_DIR/.pm2/pm2.pid
Restart=on-failure

ExecStart=/usr/bin/pm2 resurrect
ExecReload=/usr/bin/pm2 reload all
ExecStop=/usr/bin/pm2 kill

[Install]
WantedBy=multi-user.target
EOF

# เปิดใช้งาน service
sudo systemctl daemon-reload
sudo systemctl enable $SERVICE_NAME

# ติดตั้ง Nginx (ถ้ายังไม่มี)
print_status "Setting up Nginx..."
if ! command -v nginx &> /dev/null; then
    sudo apt update
    sudo apt install -y nginx
fi

# สร้าง Nginx configuration
sudo tee /etc/nginx/sites-available/$SERVICE_NAME > /dev/null <<EOF
server {
    listen 80;
    server_name $DOMAIN www.$DOMAIN;
    
    # Security headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header X-Content-Type-Options "nosniff" always;
    
    # Gzip compression
    gzip on;
    gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;
    
    # Main proxy
    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        proxy_cache_bypass \$http_upgrade;
    }
    
    # Health check
    location /api/health {
        proxy_pass http://127.0.0.1:3000/api/health;
        access_log off;
    }
}
EOF

# เปิดใช้งาน Nginx site
sudo ln -sf /etc/nginx/sites-available/$SERVICE_NAME /etc/nginx/sites-enabled/
sudo rm -f /etc/nginx/sites-enabled/default

# ทดสอบ Nginx config
if sudo nginx -t; then
    sudo systemctl restart nginx
    sudo systemctl enable nginx
    print_success "Nginx configured successfully"
else
    print_error "Nginx configuration test failed"
fi

# สร้าง management scripts
print_status "Creating management scripts..."

# Status script
cat > status.sh << 'EOF'
#!/bin/bash
echo "=== TotoBin Kiosk Status ==="
echo
echo "📊 PM2 Applications:"
pm2 status

echo
echo "💾 System Resources:"
echo "Memory: $(free -h | awk 'NR==2{printf "Used: %s/%s (%.1f%%)", $3,$2,$3*100/$2}')"
echo "Disk: $(df -h . | awk 'NR==2{printf "Used: %s/%s (%s)", $3,$2,$5}')"
echo "Load: $(uptime | awk -F'load average:' '{print $2}')"

echo
echo "🌐 Services:"
systemctl is-active nginx && echo "✅ Nginx: Running" || echo "❌ Nginx: Stopped"
systemctl is-active totobin-kiosk && echo "✅ TotoBin Service: Running" || echo "❌ TotoBin Service: Stopped"

echo
echo "🔗 URLs:"
echo "- Local: http://localhost:3000"
echo "- Health: http://localhost:3000/api/health"
echo "- Domain: http://$DOMAIN"
EOF

# Update script
cat > update.sh << 'EOF'
#!/bin/bash
echo "🔄 Updating TotoBin application..."

git pull origin main
npm install --only=production
NODE_OPTIONS="--max-old-space-size=1536" npm run build
npm prune --production

pm2 restart totobin-kiosk
pm2 save

echo "✅ Update completed!"
EOF

# Restart script
cat > restart.sh << 'EOF'
#!/bin/bash
echo "🔄 Restarting TotoBin application..."
pm2 restart totobin-kiosk
pm2 save
echo "✅ Restart completed!"
EOF

# ให้สิทธิ์ execute
chmod +x status.sh update.sh restart.sh

# รอแอพ start up
print_status "Waiting for application to start..."
sleep 15

# ทดสอบการทำงาน
print_status "Testing application..."
attempt=0
max_attempts=6

while [ $attempt -lt $max_attempts ]; do
    if curl -f http://localhost:3000/api/health >/dev/null 2>&1; then
        print_success "Application is running successfully! 🎉"
        break
    else
        attempt=$((attempt + 1))
        if [ $attempt -eq $max_attempts ]; then
            print_error "Application failed to start after $max_attempts attempts"
            print_status "PM2 Status:"
            pm2 status
            print_status "Recent Logs:"
            pm2 logs --lines 20
            exit 1
        else
            print_warning "Attempt $attempt/$max_attempts failed, retrying in 5 seconds..."
            sleep 5
        fi
    fi
done

# แสดงผลลัพธ์สุดท้าย
print_success "=== DEPLOYMENT COMPLETED SUCCESSFULLY! ==="
echo
echo "📊 Application Status:"
pm2 status

echo
echo "💾 System Info:"
./status.sh

echo
echo "🌐 Access URLs:"
echo "- Local Application: http://localhost:3000"
echo "- Health Check: http://localhost:3000/api/health"
echo "- Public Domain: http://$DOMAIN (if DNS configured)"

echo
echo "🔧 Management Commands:"
echo "- View status: ./status.sh"
echo "- Update app: ./update.sh"
echo "- Restart app: ./restart.sh"
echo "- View logs: pm2 logs"
echo "- PM2 status: pm2 status"

echo
echo "📁 Application Directory: $APP_DIR/kiosk-app"
echo "📝 Logs Directory: $APP_DIR/kiosk-app/logs"

echo
print_success "TotoBin is ready to serve customers! 🎉"
print_warning "Next steps:"
echo "1. Point domain $DOMAIN to this server's IP address"
echo "2. Setup SSL certificate: sudo certbot --nginx -d $DOMAIN"
echo "3. Test all features thoroughly"
echo "4. Configure hardware integration if needed"