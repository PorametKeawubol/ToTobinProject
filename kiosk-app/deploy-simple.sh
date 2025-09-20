#!/bin/bash

# Simple deployment script for Odroid C4 - handles Docker permissions

set -e

echo "🚀 Simple Deploy for Odroid C4..."

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Function to run docker commands with proper permissions
run_docker() {
    if docker info &> /dev/null; then
        "$@"
    elif sudo docker info &> /dev/null; then
        print_warning "Using sudo for Docker commands"
        sudo "$@"
    else
        print_error "Cannot connect to Docker daemon"
        exit 1
    fi
}

# Function to run docker-compose with proper permissions
run_compose() {
    if docker info &> /dev/null; then
        docker-compose "$@"
    elif sudo docker info &> /dev/null; then
        print_warning "Using sudo for Docker Compose"
        sudo docker-compose "$@"
    else
        print_error "Cannot connect to Docker daemon"
        exit 1
    fi
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "Don't run this script as root"
   exit 1
fi

# Update system
print_status "Updating system..."
sudo apt update

# Install Docker if needed
if ! command -v docker &> /dev/null; then
    print_status "Installing Docker..."
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
    sudo usermod -aG docker $USER
    rm get-docker.sh
fi

# Install Docker Compose if needed
if ! command -v docker-compose &> /dev/null; then
    print_status "Installing Docker Compose..."
    sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
fi

# Start Docker service
print_status "Starting Docker service..."
sudo systemctl enable docker
sudo systemctl start docker
sleep 5

# Fix permissions
print_status "Fixing Docker permissions..."
sudo chmod 666 /var/run/docker.sock 2>/dev/null || true

# Create directories
print_status "Creating directories..."
mkdir -p nginx/www nginx/ssl

# Create environment file if it doesn't exist
if [ ! -f .env.production ]; then
    print_status "Creating .env.production..."
    cp .env.production.example .env.production 2>/dev/null || {
        cat > .env.production << 'EOF'
NODE_ENV=production
PORT=3000
HOSTNAME=0.0.0.0
HARDWARE_API_KEY=totobin-hardware-secret-2024
BUSINESS_NAME=TotoBin Kiosk
PROMPTPAY_PHONE=0984435255
ENABLE_HARDWARE_INTEGRATION=true
ENABLE_REAL_TIME_UPDATES=true
ENABLE_QUEUE_SYSTEM=true
LOG_LEVEL=info
EOF
    }
    print_warning "Please edit .env.production with your actual values"
fi

# Test Docker
print_status "Testing Docker..."
if ! run_docker docker --version; then
    print_error "Docker is not working properly"
    exit 1
fi

# Validate compose file
print_status "Validating Docker Compose file..."
if ! run_compose -f docker-compose.prod.yml config > /dev/null; then
    print_error "Docker Compose file is invalid"
    exit 1
fi

# Build images
print_status "Building Docker images..."
if ! run_compose -f docker-compose.prod.yml build --no-cache; then
    print_error "Failed to build Docker images"
    exit 1
fi

# Start services
print_status "Starting services..."
if ! run_compose -f docker-compose.prod.yml up -d; then
    print_error "Failed to start services"
    exit 1
fi

# Wait for services
print_status "Waiting for services to start..."
sleep 30

# Check if services are running
print_status "Checking services..."
run_compose -f docker-compose.prod.yml ps

print_status "✅ Deployment completed!"
print_status "🌐 Your app should be available at:"
print_status "   HTTP:  http://porametix.online"
print_status "   HTTPS: https://porametix.online (after SSL setup)"

echo ""
print_status "📋 Next steps:"
echo "1. Edit .env.production with your actual configuration"
echo "2. Make sure porametix.online points to this server"
echo "3. Run: $(basename "$0" .sh) restart"

print_status "🔧 Useful commands:"
echo "  View logs:    docker-compose -f docker-compose.prod.yml logs -f"
echo "  Restart:      docker-compose -f docker-compose.prod.yml restart"
echo "  Stop:         docker-compose -f docker-compose.prod.yml down"