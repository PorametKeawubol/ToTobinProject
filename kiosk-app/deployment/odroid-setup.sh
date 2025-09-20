#!/bin/bash
# ODROID C4 Complete Setup Script
# Sets up Next.js app, Nginx, SSL, and domain configuration

set -e

echo "🚀 TotoBin ODROID C4 Setup Starting..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOMAIN="porametix.online"
APP_DIR="/opt/totobin"
SERVICE_NAME="totobin-kiosk"
NGINX_CONF="/etc/nginx/sites-available/$SERVICE_NAME"

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

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "This script should not be run as root"
   exit 1
fi

# Update system
print_status "Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install required packages
print_status "Installing required packages..."
sudo apt install -y \
    docker.io \
    docker-compose \
    nginx \
    certbot \
    python3-certbot-nginx \
    git \
    curl \
    wget \
    htop \
    fail2ban

# Enable and start Docker
print_status "Setting up Docker..."
sudo systemctl enable docker
sudo systemctl start docker
sudo usermod -aG docker $USER

# Create application directory
print_status "Creating application directory..."
sudo mkdir -p $APP_DIR
sudo chown $USER:$USER $APP_DIR

# Setup Nginx configuration
print_status "Configuring Nginx reverse proxy..."
sudo tee $NGINX_CONF > /dev/null <<EOF
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
    limit_req_zone \$binary_remote_addr zone=api:10m rate=10r/s;
    limit_req zone=api burst=20 nodelay;
    
    # Gzip compression
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_proxied expired no-cache no-store private must-revalidate auth;
    gzip_types text/plain text/css text/xml text/javascript application/x-javascript application/xml+rss application/javascript application/json;
    
    # Main application
    location / {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        proxy_cache_bypass \$http_upgrade;
        proxy_read_timeout 86400;
    }
    
    # Static files with long cache
    location /_next/static/ {
        proxy_pass http://localhost:3000;
        proxy_cache_valid 60m;
        add_header Cache-Control "public, immutable, max-age=31536000";
    }
    
    # Health check endpoint
    location /health {
        proxy_pass http://localhost:3000/api/health;
        access_log off;
    }
}
EOF

# Enable site
sudo ln -sf $NGINX_CONF /etc/nginx/sites-enabled/
sudo nginx -t

# Start services
print_status "Starting services..."
sudo systemctl enable nginx
sudo systemctl start nginx

# Setup SSL certificate
print_status "Setting up SSL certificate for $DOMAIN..."
print_warning "Please ensure your domain $DOMAIN points to this server's IP address"
read -p "Continue with SSL setup? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo certbot --nginx -d $DOMAIN -d www.$DOMAIN --non-interactive --agree-tos --email admin@$DOMAIN
    print_success "SSL certificate installed"
else
    print_warning "SSL setup skipped. You can run 'sudo certbot --nginx -d $DOMAIN' later"
fi

# Setup fail2ban for security
print_status "Configuring fail2ban..."
sudo tee /etc/fail2ban/jail.local > /dev/null <<EOF
[DEFAULT]
bantime = 3600
findtime = 600
maxretry = 5

[nginx-http-auth]
enabled = true

[nginx-noscript]
enabled = true

[nginx-badbots]
enabled = true

[nginx-noproxy]
enabled = true
EOF

sudo systemctl enable fail2ban
sudo systemctl start fail2ban

# Create Docker Compose file
print_status "Creating Docker Compose configuration..."
tee $APP_DIR/docker-compose.yml > /dev/null <<EOF
version: '3.8'

services:
  totobin-app:
    build:
      context: .
      dockerfile: Dockerfile.odroid
    container_name: totobin-kiosk
    restart: unless-stopped
    ports:
      - "3000:3000"
    environment:
      - NODE_ENV=production
      - PORT=3000
    env_file:
      - .env.production
    volumes:
      - app-data:/app/data
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
    healthcheck:
      test: ["CMD", "wget", "--no-verbose", "--tries=1", "--spider", "http://localhost:3000/api/health"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s

volumes:
  app-data:
    driver: local
EOF

# Create deployment script
print_status "Creating deployment script..."
tee $APP_DIR/deploy.sh > /dev/null <<'EOF'
#!/bin/bash
# Quick deployment script

set -e

APP_DIR="/opt/totobin"
REPO_URL="https://github.com/PorametKeawubol/ToTobinProject.git"

echo "🚀 Deploying TotoBin Kiosk..."

cd $APP_DIR

# Pull latest changes
if [ -d ".git" ]; then
    echo "📥 Pulling latest changes..."
    git pull origin main
else
    echo "📥 Cloning repository..."
    git clone $REPO_URL .
fi

# Copy configuration files
cd kiosk-app

# Build and deploy
echo "🔨 Building application..."
docker-compose down --remove-orphans
docker-compose build --no-cache
docker-compose up -d

echo "✅ Deployment complete!"
echo "🌐 Application should be available at https://porametix.online"

# Show status
docker-compose ps
EOF

chmod +x $APP_DIR/deploy.sh

# Create systemd service for auto-restart
print_status "Creating systemd service..."
sudo tee /etc/systemd/system/totobin-kiosk.service > /dev/null <<EOF
[Unit]
Description=TotoBin Kiosk Docker Compose Application
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=$APP_DIR/kiosk-app
ExecStart=/usr/bin/docker-compose up -d
ExecStop=/usr/bin/docker-compose down
TimeoutStartSec=0

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl enable totobin-kiosk

# Create monitoring script
print_status "Creating monitoring script..."
tee $APP_DIR/monitor.sh > /dev/null <<'EOF'
#!/bin/bash
# Simple monitoring script

echo "=== TotoBin Kiosk Status ==="
echo

echo "🐳 Docker Status:"
docker-compose ps

echo
echo "💾 System Resources:"
free -h
df -h /
echo "CPU Usage: $(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1)%"

echo
echo "🌐 Nginx Status:"
sudo systemctl status nginx --no-pager -l

echo
echo "🔒 SSL Certificate Status:"
sudo certbot certificates

echo
echo "📊 Recent Application Logs:"
docker-compose logs --tail=20 totobin-app

echo
echo "🔗 Quick Links:"
echo "Application: https://porametix.online"
echo "Health Check: https://porametix.online/health"
EOF

chmod +x $APP_DIR/monitor.sh

# Setup automatic updates
print_status "Setting up automatic security updates..."
sudo apt install -y unattended-upgrades
echo 'Unattended-Upgrade::Automatic-Reboot "false";' | sudo tee -a /etc/apt/apt.conf.d/50unattended-upgrades

# Print completion message
print_success "ODROID C4 setup completed successfully!"
echo
echo "📋 Next Steps:"
echo "1. Copy your application code to $APP_DIR"
echo "2. Configure your .env.production file"
echo "3. Run: cd $APP_DIR && ./deploy.sh"
echo "4. Monitor with: $APP_DIR/monitor.sh"
echo
echo "🔧 Useful Commands:"
echo "- View logs: cd $APP_DIR/kiosk-app && docker-compose logs -f"
echo "- Restart app: cd $APP_DIR/kiosk-app && docker-compose restart"
echo "- Update SSL: sudo certbot renew"
echo
echo "🌐 Your application will be available at:"
echo "- https://$DOMAIN"
echo "- http://$DOMAIN (redirects to HTTPS)"

print_warning "Don't forget to:"
echo "1. Point your domain $DOMAIN to this server's IP"
echo "2. Configure your environment variables"
echo "3. Test the deployment"

# Reboot reminder
echo
print_warning "A reboot is recommended to ensure all changes take effect"
read -p "Reboot now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo reboot
fi