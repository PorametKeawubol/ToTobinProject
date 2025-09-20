#!/bin/bash

# Odroid C4 Deployment with Docker Daemon Fix
# This script handles Docker daemon issues automatically

set -e

echo "🚀 Deploying on Odroid C4 with Docker fixes..."

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }
print_step() { echo -e "${BLUE}[STEP]${NC} $1"; }

# Function to check Docker status and fix if needed
check_and_fix_docker() {
    print_step "Checking Docker status..."
    
    # Check if Docker daemon is running
    if sudo docker info &> /dev/null; then
        print_status "✅ Docker daemon is running"
        return 0
    fi
    
    print_warning "Docker daemon not running. Attempting to fix..."
    
    # Try to start Docker
    sudo systemctl start docker
    sleep 5
    
    if sudo docker info &> /dev/null; then
        print_status "✅ Docker started successfully"
        return 0
    fi
    
    # If still not working, try full fix
    print_warning "Docker still not working. Running full fix..."
    
    # Stop services
    sudo systemctl stop docker.service || true
    sudo systemctl stop docker.socket || true
    
    # Clean up if needed
    sudo systemctl daemon-reload
    sudo systemctl start docker
    sleep 10
    
    if sudo docker info &> /dev/null; then
        print_status "✅ Docker fixed and running"
        return 0
    else
        print_error "❌ Cannot fix Docker. Please run ./fix-docker-complete.sh"
        return 1
    fi
}

# Function to run docker commands with fallback to sudo
run_docker() {
    if docker info &> /dev/null 2>&1; then
        "$@"
    elif sudo docker info &> /dev/null 2>&1; then
        sudo "$@"
    else
        print_error "Docker not available"
        return 1
    fi
}

# Function to run docker-compose with fallback to sudo
run_compose() {
    if docker info &> /dev/null 2>&1; then
        docker-compose "$@"
    elif sudo docker info &> /dev/null 2>&1; then
        sudo docker-compose "$@"
    else
        print_error "Docker not available"
        return 1
    fi
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "Don't run this script as root"
   exit 1
fi

print_step "1. System Update"
sudo apt update

print_step "2. Installing Required Packages"
sudo apt install -y curl wget git

print_step "3. Docker Installation Check"
if ! command -v docker &> /dev/null; then
    print_status "Installing Docker..."
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
    sudo usermod -aG docker $USER
    rm get-docker.sh
fi

print_step "4. Docker Compose Installation Check"
if ! command -v docker-compose &> /dev/null; then
    print_status "Installing Docker Compose..."
    sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
fi

print_step "5. Docker Daemon Check and Fix"
if ! check_and_fix_docker; then
    print_error "Cannot start Docker daemon"
    exit 1
fi

print_step "6. Setting up directories"
mkdir -p nginx/www nginx/ssl

print_step "7. Environment Configuration"
if [ ! -f .env.production ]; then
    print_status "Creating .env.production..."
    cat > .env.production << 'EOF'
# TotoBin Kiosk Environment Variables for Odroid C4
NODE_ENV=production
PORT=3000
HOSTNAME=0.0.0.0

# Hardware API Configuration
HARDWARE_API_KEY=totobin-hardware-secret-2024

# Queue Configuration
QUEUE_MAX_SIZE=50
QUEUE_PROCESSING_TIMEOUT=600000

# UI Configuration
BUSINESS_NAME=TotoBin Kiosk
PROMPTPAY_PHONE=0984435255
CURRENCY=THB

# Features
ENABLE_HARDWARE_INTEGRATION=true
ENABLE_REAL_TIME_UPDATES=true
ENABLE_QUEUE_SYSTEM=true

# Logging
LOG_LEVEL=info
EOF
    print_warning "Created .env.production - edit if needed"
fi

print_step "8. Validating Docker Compose"
if ! run_compose -f docker-compose.prod.yml config > /dev/null; then
    print_error "Docker Compose file validation failed"
    exit 1
fi

print_step "9. Building Docker Images"
print_status "This may take several minutes on ARM64..."
if ! run_compose -f docker-compose.prod.yml build --no-cache; then
    print_error "Docker build failed"
    exit 1
fi

print_step "10. Starting Services"
if ! run_compose -f docker-compose.prod.yml up -d; then
    print_error "Failed to start services"
    exit 1
fi

print_step "11. Waiting for Services"
print_status "Waiting 30 seconds for services to start..."
sleep 30

print_step "12. Service Status Check"
run_compose -f docker-compose.prod.yml ps

print_status "🎉 Deployment completed!"
print_status ""
print_status "📋 Your app is available at:"
print_status "   Local:  http://localhost:3000"
print_status "   Domain: http://porametix.online (when DNS is configured)"
print_status "   HTTPS:  https://porametix.online (after SSL setup)"
print_status ""
print_status "🔧 Useful commands:"
echo "   View logs:     docker-compose -f docker-compose.prod.yml logs -f"
echo "   Restart:       docker-compose -f docker-compose.prod.yml restart"
echo "   Stop:          docker-compose -f docker-compose.prod.yml down"
echo "   Update app:    ./update.sh"
print_status ""
print_status "📝 Next steps:"
echo "   1. Configure DNS: Point porametix.online to this server"
echo "   2. Edit .env.production if needed"
echo "   3. Monitor logs for any issues"