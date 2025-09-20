#!/bin/bash
# ODROID C4 Native Next.js Deployment (No Docker)
# Optimized for ARM64 architecture

set -e

echo "🚀 ODROID C4 Native Next.js Setup (No Docker)"

# Colors
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

# Configuration
DOMAIN="porametix.online"
APP_DIR="/opt/totobin"
APP_USER="totobin"
SERVICE_NAME="totobin-kiosk"

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "This script should not be run as root"
   exit 1
fi

# Update system
print_status "Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install Node.js 18 LTS (recommended for ODROID C4)
print_status "Installing Node.js 18 LTS..."
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Verify Node.js installation
node_version=$(node --version)
npm_version=$(npm --version)
print_success "Node.js $node_version and npm $npm_version installed"

# Install PM2 for process management
print_status "Installing PM2 process manager..."
sudo npm install -g pm2 pm2-logrotate

# Install required system packages
print_status "Installing system packages..."
sudo apt install -y \
    nginx \
    certbot \
    python3-certbot-nginx \
    git \
    curl \
    wget \
    htop \
    ufw \
    fail2ban \
    build-essential

# Create application user
print_status "Creating application user..."
if ! id "$APP_USER" &>/dev/null; then
    sudo adduser --system --group --home $APP_DIR --shell /bin/bash $APP_USER
    print_success "User $APP_USER created"
else
    print_warning "User $APP_USER already exists"
fi

# Create application directory
print_status "Setting up application directory..."
sudo mkdir -p $APP_DIR
sudo chown $APP_USER:$APP_USER $APP_DIR

# Switch to app user for Node.js operations
print_status "Setting up application as $APP_USER..."
sudo -u $APP_USER bash << 'USEREOF'

APP_DIR="/opt/totobin"
cd $APP_DIR

# Clone or update repository
if [ -d "ToTobinProject" ]; then
    echo "Updating repository..."
    cd ToTobinProject
    git pull origin main
else
    echo "Cloning repository..."
    git clone https://github.com/PorametKeawubol/ToTobinProject.git
    cd ToTobinProject
fi

cd kiosk-app

# Create production environment
echo "Creating production environment..."
cat > .env.production << EOF
NODE_ENV=production
PORT=3000
NEXT_TELEMETRY_DISABLED=1

# Database (SQLite for ODROID)
DATABASE_URL="file:./data/totobin.db"

# API Keys  
HARDWARE_API_KEY=odroid-hardware-key-$(openssl rand -hex 8)

# Payment (Test mode)
PROMPTPAY_API_KEY=test_key
PROMPTPAY_WEBHOOK_SECRET=test_secret

# Auth
NEXTAUTH_URL=https://porametix.online
NEXTAUTH_SECRET=$(openssl rand -base64 32)

# Performance optimization for ODROID
NODE_OPTIONS=--max-old-space-size=1024
UV_THREADPOOL_SIZE=4
EOF

# Copy optimized Next.js config
if [ -f "next.config.odroid.ts" ]; then
    cp next.config.odroid.ts next.config.ts
    echo "Using optimized Next.js config for ODROID"
fi

# Install dependencies with production optimizations
echo "Installing dependencies..."
NODE_ENV=production npm ci --only=production --no-audit --no-fund

# Build application
echo "Building application for production..."
NODE_OPTIONS="--max-old-space-size=1024" npm run build

# Create data directory
mkdir -p data logs

# Create PM2 ecosystem configuration
cat > ecosystem.config.js << 'PM2EOF'
module.exports = {
  apps: [{
    name: 'totobin-kiosk',
    script: 'npm',
    args: 'start',
    cwd: '/opt/totobin/ToTobinProject/kiosk-app',
    instances: 1,
    exec_mode: 'fork',
    max_memory_restart: '512M',
    env: {
      NODE_ENV: 'production',
      PORT: '3000'
    },
    env_file: '.env.production',
    error_file: './logs/err.log',
    out_file: './logs/out.log',
    log_file: './logs/combined.log',
    time: true,
    autorestart: true,
    max_restarts: 5,
    min_uptime: '10s',
    kill_timeout: 5000
  }]
}
PM2EOF

echo "Application setup completed for user $APP_USER"

USEREOF

# Configure Nginx
print_status "Configuring Nginx..."
sudo tee /etc/nginx/sites-available/$SERVICE_NAME > /dev/null <<EOF
# TotoBin Kiosk Nginx Configuration
server {
    listen 80;
    server_name $DOMAIN www.$DOMAIN;
    
    # Security headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header Referrer-Policy "no-referrer-when-downgrade" always;
    add_header Content-Security-Policy "default-src 'self' http: https: data: blob: 'unsafe-inline'" always;
    
    # Rate limiting
    limit_req_zone \$binary_remote_addr zone=api:10m rate=30r/m;
    limit_req zone=api burst=5 nodelay;
    
    # Gzip compression
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_proxied expired no-cache no-store private must-revalidate auth;
    gzip_types
        text/plain
        text/css
        text/xml
        text/javascript
        application/javascript
        application/xml+rss
        application/json;
    
    # Main application
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
        proxy_read_timeout 60s;
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
    }
    
    # Static files caching
    location /_next/static/ {
        proxy_pass http://127.0.0.1:3000;
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
    
    # Health check
    location /api/health {
        proxy_pass http://127.0.0.1:3000/api/health;
        access_log off;
    }
}
EOF

# Enable Nginx site
sudo ln -sf /etc/nginx/sites-available/$SERVICE_NAME /etc/nginx/sites-enabled/
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t

# Start and enable Nginx
sudo systemctl enable nginx
sudo systemctl restart nginx

# Create systemd service for PM2
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
Environment=PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
Environment=PM2_HOME=/opt/totobin/.pm2
PIDFile=/opt/totobin/.pm2/pm2.pid
Restart=on-failure

ExecStart=/usr/bin/pm2 resurrect
ExecReload=/usr/bin/pm2 reload all
ExecStop=/usr/bin/pm2 kill

[Install]
WantedBy=multi-user.target
EOF

# Start application with PM2
print_status "Starting application..."
sudo -u $APP_USER bash -c "
    cd $APP_DIR/ToTobinProject/kiosk-app
    export PM2_HOME=$APP_DIR/.pm2
    pm2 start ecosystem.config.js
    pm2 save
    pm2 startup systemd -u $APP_USER --hp $APP_DIR
"

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable $SERVICE_NAME
sudo systemctl start $SERVICE_NAME

# Setup SSL Certificate
print_status "Setting up SSL certificate..."
print_warning "Make sure domain $DOMAIN points to this server's IP"
read -p "Continue with SSL setup? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo certbot --nginx -d $DOMAIN -d www.$DOMAIN --non-interactive --agree-tos --email admin@$DOMAIN
    print_success "SSL certificate installed"
else
    print_warning "SSL setup skipped. Run later: sudo certbot --nginx -d $DOMAIN"
fi

# Setup firewall
print_status "Configuring firewall..."
sudo ufw --force enable
sudo ufw allow ssh
sudo ufw allow 'Nginx Full'

# Setup fail2ban
print_status "Configuring fail2ban..."
sudo systemctl enable fail2ban
sudo systemctl start fail2ban

# Create management scripts
print_status "Creating management scripts..."

# Status script
sudo -u $APP_USER tee $APP_DIR/status.sh > /dev/null <<'EOF'
#!/bin/bash
echo "=== TotoBin Kiosk Status ==="
echo
echo "🚀 Application Status:"
pm2 status

echo
echo "💾 System Resources:"
echo "Memory: $(free -h | awk 'NR==2{printf "Used: %s/%s (%.1f%%)", $3,$2,$3*100/$2}')"
echo "Disk: $(df -h /opt/totobin | awk 'NR==2{printf "Used: %s/%s (%s)", $3,$2,$5}')"
echo "Load: $(uptime | awk -F'load average:' '{print $2}')"

echo
echo "🌐 Service Status:"
systemctl is-active nginx && echo "✅ Nginx: Running" || echo "❌ Nginx: Stopped"
systemctl is-active totobin-kiosk && echo "✅ TotoBin: Running" || echo "❌ TotoBin: Stopped"

echo
echo "📊 Recent Logs:"
pm2 logs --lines 10

echo
echo "🔗 URLs:"
echo "- Local: http://localhost:3000"
echo "- Domain: https://porametix.online"
echo "- Health: https://porametix.online/api/health"
EOF

# Update script
sudo -u $APP_USER tee $APP_DIR/update.sh > /dev/null <<'EOF'
#!/bin/bash
echo "🔄 Updating TotoBin Kiosk..."

cd /opt/totobin/ToTobinProject
git pull origin main

cd kiosk-app
npm ci --only=production
npm run build

pm2 restart totobin-kiosk
pm2 save

echo "✅ Update completed!"
EOF

# Make scripts executable
chmod +x $APP_DIR/status.sh $APP_DIR/update.sh

# Final verification
print_status "Performing final checks..."
sleep 10

# Check if application is running
if curl -f http://localhost:3000/api/health >/dev/null 2>&1; then
    print_success "Application is running successfully!"
else
    print_error "Application health check failed"
    print_status "Checking PM2 status..."
    sudo -u $APP_USER pm2 status
    print_status "Checking logs..."
    sudo -u $APP_USER pm2 logs --lines 20
fi

# Show final status
print_success "ODROID C4 Native Setup Complete! 🎉"
echo
echo "📋 Management Commands:"
echo "- Status: sudo -u $APP_USER $APP_DIR/status.sh"
echo "- Update: sudo -u $APP_USER $APP_DIR/update.sh"
echo "- Logs: sudo -u $APP_USER pm2 logs"
echo "- Restart: sudo -u $APP_USER pm2 restart totobin-kiosk"
echo "- Service: sudo systemctl status $SERVICE_NAME"
echo
echo "🌐 Your application:"
echo "- Local: http://localhost:3000"
echo "- Health: http://localhost:3000/api/health"
echo "- Domain: https://$DOMAIN (if SSL configured)"
echo
echo "📊 System Status:"
sudo -u $APP_USER $APP_DIR/status.sh

print_warning "Remember to:"
echo "1. Point domain $DOMAIN to this server's IP"
echo "2. Test the application thoroughly"
echo "3. Setup regular backups"

echo
print_success "Deployment complete! No Docker required! 🚀"