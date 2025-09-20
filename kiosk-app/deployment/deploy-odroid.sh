#!/bin/bash
# Quick ODROID C4 Deployment Script
# Optimized for minimal resource usage

set -e

echo "🚀 Quick ODROID C4 Deployment"

# Configuration
DOMAIN="porametix.online"
APP_DIR="/opt/totobin"
GITHUB_REPO="https://github.com/PorametKeawubol/ToTobinProject.git"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed. Please run setup script first."
    exit 1
fi

# Create app directory
print_status "Setting up application directory..."
sudo mkdir -p $APP_DIR
sudo chown $USER:$USER $APP_DIR
cd $APP_DIR

# Clone or update repository
if [ -d "ToTobinProject" ]; then
    print_status "Updating repository..."
    cd ToTobinProject
    git pull origin main
else
    print_status "Cloning repository..."
    git clone $GITHUB_REPO
    cd ToTobinProject
fi

cd kiosk-app

# Create optimized environment file for ODROID
print_status "Creating production environment..."
cat > .env.production << EOF
# ODROID C4 Production Environment
NODE_ENV=production
PORT=3000
NEXT_TELEMETRY_DISABLED=1

# Database (use lightweight SQLite for ODROID)
DATABASE_URL="file:./data/totobin.db"

# API Keys
HARDWARE_API_KEY=prod-hardware-key-$(openssl rand -hex 16)

# Payment (PromptPay test mode)
PROMPTPAY_API_KEY=test_key
PROMPTPAY_WEBHOOK_SECRET=test_secret

# Domain
NEXTAUTH_URL=https://$DOMAIN
NEXTAUTH_SECRET=$(openssl rand -base64 32)

# Performance optimization for ARM64
NODE_OPTIONS=--max-old-space-size=512
UV_THREADPOOL_SIZE=4
EOF

# Create lightweight Docker Compose for ODROID
print_status "Creating Docker Compose configuration..."
cat > docker-compose.odroid.yml << EOF
version: '3.8'

services:
  app:
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
      - ./data:/app/data
      - /tmp:/tmp
    tmpfs:
      - /app/.next/cache:size=64m,noexec,nosuid,nodev
    mem_limit: 768m
    memswap_limit: 768m
    cpus: '2.0'
    logging:
      driver: "json-file"
      options:
        max-size: "5m"
        max-file: "2"
    healthcheck:
      test: ["CMD-SHELL", "curl -f http://localhost:3000/api/health || exit 1"]
      interval: 60s
      timeout: 10s
      retries: 3
      start_period: 60s
EOF

# Create data directory
mkdir -p data

# Build and deploy
print_status "Building optimized application for ARM64..."
export DOCKER_BUILDKIT=1
docker-compose -f docker-compose.odroid.yml down --remove-orphans
docker-compose -f docker-compose.odroid.yml build --no-cache
docker-compose -f docker-compose.odroid.yml up -d

# Wait for application to start
print_status "Waiting for application to start..."
sleep 30

# Health check
print_status "Performing health check..."
if curl -f http://localhost:3000/api/health >/dev/null 2>&1; then
    print_success "Application is running successfully!"
else
    print_error "Application health check failed"
    echo "Checking logs..."
    docker-compose -f docker-compose.odroid.yml logs --tail=50
    exit 1
fi

# Show status
print_success "Deployment completed!"
echo
echo "📊 Container Status:"
docker-compose -f docker-compose.odroid.yml ps

echo
echo "💾 Resource Usage:"
echo "Memory: $(free -h | awk 'NR==2{printf "%.1f%%", $3*100/$2 }')"
echo "Disk: $(df -h . | awk 'NR==2{print $5}')"
echo "Load: $(uptime | awk -F'load average:' '{ print $2 }')"

echo
echo "🌐 Application URLs:"
echo "- Local: http://localhost:3000"
echo "- Domain: https://$DOMAIN (if DNS configured)"
echo "- Health: http://localhost:3000/api/health"

echo
echo "🔧 Management Commands:"
echo "- View logs: docker-compose -f docker-compose.odroid.yml logs -f"
echo "- Restart: docker-compose -f docker-compose.odroid.yml restart"
echo "- Stop: docker-compose -f docker-compose.odroid.yml down"
echo "- Update: git pull && docker-compose -f docker-compose.odroid.yml up -d --build"

print_success "ODROID C4 deployment ready! 🎉"