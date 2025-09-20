#!/bin/bash

# Fix Docker permissions and service issues on Odroid

echo "🔧 Fixing Docker issues on Odroid C4..."

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed. Please run ./deploy-odroid.sh first"
    exit 1
fi

# Start Docker service
print_status "Starting Docker service..."
sudo systemctl enable docker
sudo systemctl start docker

# Wait for Docker to start
sleep 5

# Check if Docker daemon is running
if ! sudo docker info &> /dev/null; then
    print_error "Docker daemon is not running. Trying to restart..."
    sudo systemctl restart docker
    sleep 10
fi

# Add user to docker group
print_status "Adding user to docker group..."
sudo usermod -aG docker $USER

# Fix socket permissions
print_status "Fixing Docker socket permissions..."
sudo chmod 666 /var/run/docker.sock

# Test Docker without sudo
print_status "Testing Docker access..."
if docker info &> /dev/null; then
    print_status "✅ Docker is working without sudo"
else
    print_warning "Docker still requires sudo. Applying group membership..."
    # Apply group membership immediately
    newgrp docker << 'EOF'
    if docker info &> /dev/null; then
        echo "✅ Docker is now working"
    else
        echo "❌ Still having issues. You may need to log out and log back in."
    fi
EOF
fi

# Check Docker Compose
print_status "Checking Docker Compose..."
if command -v docker-compose &> /dev/null; then
    print_status "✅ Docker Compose found: $(docker-compose --version)"
else
    print_error "Docker Compose not found"
fi

# Test compose file
print_status "Testing docker-compose.prod.yml..."
if docker-compose -f docker-compose.prod.yml config &> /dev/null; then
    print_status "✅ Docker Compose file is valid"
else
    print_error "❌ Docker Compose file has errors"
    docker-compose -f docker-compose.prod.yml config
fi

print_status "Docker fix completed!"
print_warning "If you still have permission issues, please log out and log back in, then try again."