#!/bin/bash
# Quick ODROID C4 deployment script for TotoBin Kiosk
# Lightweight version with yarn optimization

set -e

echo "🚀 TotoBin Kiosk - Quick ODROID C4 Setup"
echo "Domain: porametix.online | Using yarn for ARM optimization"

# Quick check
if [[ $EUID -ne 0 ]]; then
   echo "❌ Run as root: sudo bash quick-odroid-deploy.sh"
   exit 1
fi

# Colors
G='\033[0;32m'
Y='\033[1;33m'
R='\033[0;31m'
NC='\033[0m'

step() { echo -e "${G}[STEP]${NC} $1"; }
warn() { echo -e "${Y}[WARN]${NC} $1"; }
error() { echo -e "${R}[ERROR]${NC} $1"; }

# Quick system setup
step "1. Installing essentials..."
apt update
apt install -y curl git nginx certbot python3-certbot-nginx

# Node.js + yarn (optimized for ARM)
step "2. Installing Node.js + yarn..."
curl -fsSL https://deb.nodesource.com/setup_18.x | bash -
apt install -y nodejs
npm install -g yarn

step "3. Setting up application..."
APP_DIR="/opt/totobin-kiosk"
mkdir -p $APP_DIR && cd $APP_DIR

# Clone if new, pull if exists
if [ ! -d ".git" ]; then
    git clone https://github.com/PorametKeawubol/ToTobinProject.git .
fi
cd kiosk-app

# Fast install with yarn
step "4. Installing dependencies (yarn)..."
export NODE_ENV=production
yarn install --production=false --silent

step "5. Building application..."
yarn build

step "6. Quick environment setup..."
cat > .env.production << EOF
NODE_ENV=production
PORT=3000
NEXT_TELEMETRY_DISABLED=1
DATABASE_URL="file:./dev.db"
HARDWARE_API_KEY="prod-$(openssl rand -hex 8)"
KIOSK_ID="odroid-kiosk-001"
NEXTAUTH_URL="https://porametix.online"
NEXTAUTH_SECRET="$(openssl rand -base64 32)"
EOF

step "7. Creating systemd service..."
cat > /etc/systemd/system/totobin-kiosk.service << 'EOF'
[Unit]
Description=TotoBin Kiosk
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/totobin-kiosk/kiosk-app
Environment=NODE_ENV=production
Environment=PORT=3000
ExecStart=/usr/bin/yarn start
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

step "8. Quick nginx setup..."
cat > /etc/nginx/sites-available/porametix.online << 'EOF'
server {
    listen 80;
    server_name porametix.online www.porametix.online;
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name porametix.online www.porametix.online;
    
    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
EOF

ln -sf /etc/nginx/sites-available/porametix.online /etc/nginx/sites-enabled/
rm -f /etc/nginx/sites-enabled/default

step "9. Setting up SSL..."
certbot --nginx -d porametix.online -d www.porametix.online --non-interactive --agree-tos --email admin@porametix.online || warn "SSL setup failed - will use HTTP"

step "10. Starting services..."
chown -R www-data:www-data $APP_DIR
systemctl daemon-reload
systemctl enable totobin-kiosk nginx
systemctl restart nginx totobin-kiosk

# Quick status check
sleep 3
if systemctl is-active --quiet totobin-kiosk; then
    step "✅ Application is running!"
    echo ""
    echo "🌐 URL: https://porametix.online"
    echo "📝 Logs: journalctl -fu totobin-kiosk"
    echo "🔄 Restart: systemctl restart totobin-kiosk"
    echo ""
    echo "🚀 Hardware API: https://porametix.online/api/hardware"
else
    error "❌ Service failed to start"
    journalctl -u totobin-kiosk --lines=20
fi