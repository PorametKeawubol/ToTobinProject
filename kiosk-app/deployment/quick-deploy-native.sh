#!/bin/bash
# Quick ODROID Native Deployment (No Docker)
# For when Docker is not working

set -e

echo "🚀 Quick ODROID Native Deployment"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

APP_DIR="/opt/totobin"

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    print_status "Installing Node.js 18..."
    curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
    sudo apt-get install -y nodejs
fi

# Check if PM2 is installed
if ! command -v pm2 &> /dev/null; then
    print_status "Installing PM2..."
    sudo npm install -g pm2
fi

# Setup application
print_status "Setting up application..."
sudo mkdir -p $APP_DIR
sudo chown $USER:$USER $APP_DIR
cd $APP_DIR

# Clone/update repository
if [ -d "ToTobinProject" ]; then
    print_status "Updating repository..."
    cd ToTobinProject
    git pull origin main
else
    print_status "Cloning repository..."
    git clone https://github.com/PorametKeawubol/ToTobinProject.git
    cd ToTobinProject
fi

cd kiosk-app

# Create environment
print_status "Creating production environment..."
cat > .env.production << EOF
NODE_ENV=production
PORT=3000
NEXT_TELEMETRY_DISABLED=1
DATABASE_URL="file:./data/totobin.db"
HARDWARE_API_KEY=odroid-hardware-key-$(date +%s)
NEXTAUTH_URL=https://porametix.online
NEXTAUTH_SECRET=$(openssl rand -base64 32)
NODE_OPTIONS=--max-old-space-size=1024
EOF

# Use optimized config if available
if [ -f "next.config.odroid.ts" ]; then
    cp next.config.odroid.ts next.config.ts
    print_status "Using ODROID optimized config"
fi

# Install and build
print_status "Installing dependencies..."
NODE_ENV=production npm ci --only=production

print_status "Building application..."
NODE_OPTIONS="--max-old-space-size=1024" npm run build

# Create directories
mkdir -p data logs

# Create PM2 config
cat > ecosystem.config.js << 'EOF'
module.exports = {
  apps: [{
    name: 'totobin-kiosk',
    script: 'npm',
    args: 'start',
    instances: 1,
    max_memory_restart: '512M',
    env: {
      NODE_ENV: 'production',
      PORT: '3000'
    },
    env_file: '.env.production',
    error_file: './logs/err.log',
    out_file: './logs/out.log',
    log_file: './logs/combined.log',
    time: true
  }]
}
EOF

# Start with PM2
print_status "Starting application..."
pm2 delete totobin-kiosk 2>/dev/null || true
pm2 start ecosystem.config.js
pm2 save
pm2 startup

# Quick Nginx setup if not exists
if ! systemctl is-active --quiet nginx; then
    print_status "Installing Nginx..."
    sudo apt install -y nginx
    
    # Basic config
    sudo tee /etc/nginx/sites-available/totobin-kiosk > /dev/null <<'NGINXEOF'
server {
    listen 80;
    server_name porametix.online www.porametix.online;
    
    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
NGINXEOF
    
    sudo ln -sf /etc/nginx/sites-available/totobin-kiosk /etc/nginx/sites-enabled/
    sudo rm -f /etc/nginx/sites-enabled/default
    sudo nginx -t
    sudo systemctl restart nginx
fi

# Test application
print_status "Testing application..."
sleep 5

if curl -f http://localhost:3000/api/health >/dev/null 2>&1; then
    print_success "Application is running! 🎉"
else
    echo "Health check failed, checking status..."
    pm2 status
    pm2 logs --lines 10
fi

print_success "Quick deployment completed!"
echo
echo "📊 Status:"
pm2 status

echo
echo "🌐 URLs:"
echo "- Local: http://localhost:3000"
echo "- Health: http://localhost:3000/api/health"
echo "- Domain: http://porametix.online (if DNS configured)"

echo
echo "🔧 Commands:"
echo "- Restart: pm2 restart totobin-kiosk"
echo "- Logs: pm2 logs"
echo "- Status: pm2 status"
echo "- Stop: pm2 stop totobin-kiosk"