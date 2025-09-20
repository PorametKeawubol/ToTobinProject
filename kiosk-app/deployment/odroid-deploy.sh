#!/bin/bash
set -e

# TotoBin Kiosk - ODROID C4 Deployment Script
# Optimized for ARM architecture with yarn instead of npm

echo "🚀 Starting ODROID C4 deployment for TotoBin Kiosk..."
echo "Domain: porametix.online"
echo "Using yarn for better performance on ARM"

# Configuration
APP_NAME="totobin-kiosk"
APP_DIR="/opt/$APP_NAME"
SERVICE_NAME="$APP_NAME"
DOMAIN="porametix.online"
NODE_VERSION="18"
PORT="3000"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   print_error "This script must be run as root"
   exit 1
fi

print_status "Step 1: Installing system dependencies..."

# Update system
apt update && apt upgrade -y

# Install essential packages
apt install -y curl wget git nginx certbot python3-certbot-nginx

# Install Node.js via NodeSource (optimized for ARM)
curl -fsSL https://deb.nodesource.com/setup_${NODE_VERSION}.x | bash -
apt install -y nodejs

# Install yarn globally
npm install -g yarn

# Verify installations
print_status "Checking installed versions..."
echo "Node.js: $(node --version)"
echo "npm: $(npm --version)"
echo "yarn: $(yarn --version)"

print_status "Step 2: Creating application directory..."

# Create app directory
mkdir -p $APP_DIR
cd $APP_DIR

# If app already exists, backup and update
if [ -d "$APP_DIR/.git" ]; then
    print_warning "Existing installation found. Creating backup..."
    systemctl stop $SERVICE_NAME || true
    cp -r $APP_DIR "${APP_DIR}_backup_$(date +%Y%m%d_%H%M%S)"
fi

print_status "Step 3: Cloning/updating application..."

# Clone or update repository
if [ ! -d ".git" ]; then
    git clone https://github.com/PorametKeawubol/ToTobinProject.git .
    cd kiosk-app
else
    git pull origin main
    cd kiosk-app
fi

print_status "Step 4: Installing dependencies with yarn..."

# Install dependencies (faster than npm)
export NODE_ENV=production
yarn install --production=false --frozen-lockfile

print_status "Step 5: Building application..."

# Build the application
yarn build

print_status "Step 6: Setting up environment..."

# Create production environment file
cat > .env.production << EOF
NODE_ENV=production
PORT=$PORT
NEXT_TELEMETRY_DISABLED=1

# Database
DATABASE_URL="file:./dev.db"

# Hardware API
HARDWARE_API_KEY="prod-hardware-key-$(openssl rand -hex 16)"

# Kiosk Settings
KIOSK_ID="odroid-kiosk-001"
KIOSK_LOCATION="Main Location"

# Domain settings
NEXTAUTH_URL="https://$DOMAIN"
NEXTAUTH_SECRET="$(openssl rand -base64 32)"
EOF

print_status "Step 7: Setting up systemd service..."

# Create systemd service
cat > /etc/systemd/system/$SERVICE_NAME.service << EOF
[Unit]
Description=TotoBin Kiosk Application
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=$APP_DIR/kiosk-app
Environment=NODE_ENV=production
Environment=PORT=$PORT
ExecStart=$(which yarn) start
Restart=always
RestartSec=10
StandardOutput=syslog
StandardError=syslog
SyslogIdentifier=$SERVICE_NAME

# Performance optimizations for ODROID C4
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
EOF

# Set permissions
chown -R www-data:www-data $APP_DIR
chmod +x $APP_DIR/kiosk-app/start.sh

print_status "Step 8: Configuring nginx..."

# Create nginx configuration
cat > /etc/nginx/sites-available/$DOMAIN << EOF
# TotoBin Kiosk - ODROID C4 Configuration
server {
    listen 80;
    server_name $DOMAIN www.$DOMAIN;
    
    # Redirect to HTTPS
    return 301 https://\$server_name\$request_uri;
}

server {
    listen 443 ssl http2;
    server_name $DOMAIN www.$DOMAIN;
    
    # SSL Configuration (will be set up by certbot)
    ssl_certificate /etc/letsencrypt/live/$DOMAIN/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/$DOMAIN/privkey.pem;
    
    # Modern SSL configuration
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES128-GCM-SHA256;
    ssl_prefer_server_ciphers off;
    
    # Security headers
    add_header X-Frame-Options DENY;
    add_header X-Content-Type-Options nosniff;
    add_header X-XSS-Protection "1; mode=block";
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
    
    # Proxy to Next.js
    location / {
        proxy_pass http://127.0.0.1:$PORT;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        proxy_cache_bypass \$http_upgrade;
        
        # Timeout settings
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
    
    # Static files caching
    location /_next/static/ {
        proxy_pass http://127.0.0.1:$PORT;
        proxy_cache_valid 200 1y;
        add_header Cache-Control "public, immutable";
    }
    
    # API routes
    location /api/ {
        proxy_pass http://127.0.0.1:$PORT;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
    }
}
EOF

# Enable site
ln -sf /etc/nginx/sites-available/$DOMAIN /etc/nginx/sites-enabled/
rm -f /etc/nginx/sites-enabled/default

# Test nginx configuration
nginx -t

print_status "Step 9: Setting up SSL certificate..."

# Get SSL certificate
certbot --nginx -d $DOMAIN -d www.$DOMAIN --non-interactive --agree-tos --email admin@$DOMAIN

print_status "Step 10: Starting services..."

# Enable and start services
systemctl daemon-reload
systemctl enable $SERVICE_NAME
systemctl enable nginx

# Start nginx first
systemctl restart nginx

# Start the application
systemctl restart $SERVICE_NAME

# Wait a moment for services to start
sleep 5

print_status "Step 11: Checking service status..."

# Check service status
if systemctl is-active --quiet $SERVICE_NAME; then
    print_status "✅ TotoBin Kiosk service is running"
else
    print_error "❌ TotoBin Kiosk service failed to start"
    systemctl status $SERVICE_NAME
fi

if systemctl is-active --quiet nginx; then
    print_status "✅ Nginx is running"
else
    print_error "❌ Nginx failed to start"
    systemctl status nginx
fi

print_status "Step 12: Performance optimization for ODROID C4..."

# CPU governor for performance
echo 'performance' > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 'performance' > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo 'performance' > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo 'performance' > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor

# Memory optimization
echo 'vm.swappiness=10' >> /etc/sysctl.conf
echo 'vm.vfs_cache_pressure=50' >> /etc/sysctl.conf
sysctl -p

print_status "🎉 Deployment completed successfully!"
echo ""
echo "📋 Deployment Summary:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🌐 Application URL: https://$DOMAIN"
echo "📁 Application Directory: $APP_DIR/kiosk-app"
echo "🔧 Service Name: $SERVICE_NAME"
echo "📝 Logs: journalctl -fu $SERVICE_NAME"
echo "⚙️  Package Manager: yarn (faster than npm on ARM)"
echo "🖥️  Platform: ODROID C4 (ARM64)"
echo ""
echo "🛠️  Useful Commands:"
echo "   Restart app: systemctl restart $SERVICE_NAME"
echo "   Check logs: journalctl -fu $SERVICE_NAME"
echo "   Update app: cd $APP_DIR && git pull && cd kiosk-app && yarn install && yarn build && systemctl restart $SERVICE_NAME"
echo "   Check status: systemctl status $SERVICE_NAME nginx"
echo ""
print_status "Application should be available at https://$DOMAIN"
print_status "Hardware API endpoint: https://$DOMAIN/api/hardware"