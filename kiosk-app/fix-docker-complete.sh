#!/bin/bash

# Complete Docker Fix for Odroid C4
# This script fixes common Docker daemon issues on ARM64 systems

echo "🔧 Fixing Docker daemon issues on Odroid C4..."

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }
print_step() { echo -e "${BLUE}[STEP]${NC} $1"; }

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "Don't run this script as root"
   exit 1
fi

print_step "1. Stopping Docker services"
sudo systemctl stop docker.service || true
sudo systemctl stop docker.socket || true
sudo systemctl stop containerd || true

print_step "2. Cleaning Docker data (if corrupted)"
print_warning "This will remove all Docker containers and images!"
read -p "Do you want to clean Docker data? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo rm -rf /var/lib/docker
    print_status "Docker data cleaned"
fi

print_step "3. Reinstalling Docker (if needed)"
if ! command -v docker &> /dev/null; then
    print_status "Installing Docker..."
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
    rm get-docker.sh
fi

print_step "4. Setting up Docker configuration"
# Create Docker daemon configuration
sudo mkdir -p /etc/docker
cat << 'EOF' | sudo tee /etc/docker/daemon.json
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  },
  "storage-driver": "overlay2",
  "live-restore": false,
  "userland-proxy": false
}
EOF

print_step "5. Setting up user permissions"
sudo usermod -aG docker $USER
sudo chmod 666 /var/run/docker.sock 2>/dev/null || true

print_step "6. Reloading systemd and starting Docker"
sudo systemctl daemon-reload
sudo systemctl enable docker
sudo systemctl start docker

# Wait for Docker to start
print_status "Waiting for Docker to start..."
sleep 10

print_step "7. Testing Docker"
if sudo docker info &> /dev/null; then
    print_status "✅ Docker daemon is running with sudo"
    
    # Test without sudo
    if docker info &> /dev/null; then
        print_status "✅ Docker works without sudo"
    else
        print_warning "Docker requires sudo (normal for fresh installation)"
        print_warning "You may need to log out and log back in for group changes"
    fi
else
    print_error "❌ Docker daemon still not working"
    print_status "Checking Docker logs..."
    sudo journalctl -u docker.service --no-pager -l | tail -20
    exit 1
fi

print_step "8. Installing Docker Compose"
if ! command -v docker-compose &> /dev/null; then
    print_status "Installing Docker Compose..."
    COMPOSE_VERSION=$(curl -s https://api.github.com/repos/docker/compose/releases/latest | grep 'tag_name' | cut -d\" -f4)
    sudo curl -L "https://github.com/docker/compose/releases/download/${COMPOSE_VERSION}/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
    sudo ln -sf /usr/local/bin/docker-compose /usr/bin/docker-compose
fi

print_step "9. Testing Docker Compose"
if docker-compose --version &> /dev/null; then
    print_status "✅ Docker Compose: $(docker-compose --version)"
elif sudo docker-compose --version &> /dev/null; then
    print_status "✅ Docker Compose (with sudo): $(sudo docker-compose --version)"
else
    print_error "❌ Docker Compose not working"
fi

print_step "10. Final system check"
echo "=== System Information ==="
echo "OS: $(uname -a)"
echo "Architecture: $(uname -m)"
echo "Docker version: $(sudo docker --version 2>/dev/null || echo 'Not available')"
echo "Docker Compose: $(sudo docker-compose --version 2>/dev/null || echo 'Not available')"
echo "User groups: $(groups)"

print_status "🎉 Docker fix completed!"
print_warning "If Docker still requires sudo, log out and log back in"
print_status "You can now try deploying with: ./deploy-simple.sh"