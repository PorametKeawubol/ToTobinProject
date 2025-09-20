#!/bin/bash

# Odroid C4 Deployment Script for Kiosk App
# Domain: porametix.online

set -e

echo "🚀 Starting deployment on Odroid C4..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
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
if [[ $EUID -eq 0 ]]; then
   print_error "This script should not be run as root"
   exit 1
fi

# Update system packages
print_status "Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install Docker if not installed
if ! command -v docker &> /dev/null; then
    print_status "Installing Docker..."
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
    sudo usermod -aG docker $USER
    rm get-docker.sh
    print_warning "Please log out and log back in for Docker group changes to take effect"
fi

# Install Docker Compose if not installed
if ! command -v docker-compose &> /dev/null; then
    print_status "Installing Docker Compose..."
    sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
fi

# Create necessary directories
print_status "Creating directories..."
mkdir -p nginx/www
mkdir -p nginx/ssl

# Set up environment file
if [ ! -f .env.production ]; then
    print_warning "Creating .env.production file..."
    cat > .env.production << EOF
# Next.js Environment Variables
NODE_ENV=production
HOSTNAME=0.0.0.0
PORT=3000

# Your app-specific variables
GOOGLE_CLOUD_PROJECT_ID=your-project-id
GOOGLE_CLOUD_REGION=your-region
HARDWARE_API_KEY=your-hardware-api-key
JWT_SECRET=your-jwt-secret
PROMPTPAY_PHONE=your-promptpay-phone
BUSINESS_NAME=your-business-name
EOF
    print_warning "Please edit .env.production with your actual values"
fi

# Create health check endpoint
print_status "Adding health check endpoint..."
mkdir -p src/app/api/health
cat > src/app/api/health/route.ts << 'EOF'
import { NextResponse } from 'next/server';

export async function GET() {
  return NextResponse.json({ 
    status: 'healthy',
    timestamp: new Date().toISOString(),
    uptime: process.uptime()
  });
}
EOF

# Build and start services
print_status "Building Docker images..."
docker-compose -f docker-compose.prod.yml build --no-cache

print_status "Starting services..."
docker-compose -f docker-compose.prod.yml up -d

# Wait for services to be ready
print_status "Waiting for services to start..."
sleep 30

# Check if services are running
if docker-compose -f docker-compose.prod.yml ps | grep -q "Up"; then
    print_status "Services started successfully!"
else
    print_error "Failed to start services"
    docker-compose -f docker-compose.prod.yml logs
    exit 1
fi

# Setup SSL certificate (initial setup)
print_status "Setting up SSL certificate..."
print_warning "Make sure your domain porametix.online points to this server's IP address"
print_warning "You may need to edit the certbot command in docker-compose.prod.yml with your email"

# Show status
print_status "Deployment completed!"
print_status "Your app should be available at:"
print_status "  HTTP:  http://porametix.online"
print_status "  HTTPS: https://porametix.online (after SSL setup)"

echo ""
print_status "Useful commands:"
echo "  View logs:        docker-compose -f docker-compose.prod.yml logs -f"
echo "  Restart services: docker-compose -f docker-compose.prod.yml restart"
echo "  Stop services:    docker-compose -f docker-compose.prod.yml down"
echo "  Update app:       ./update.sh"

print_status "Next steps:"
echo "1. Edit .env.production with your actual configuration"
echo "2. Update your email in docker-compose.prod.yml for SSL certificate"
echo "3. Make sure porametix.online points to this server"
echo "4. Run: docker-compose -f docker-compose.prod.yml restart certbot"