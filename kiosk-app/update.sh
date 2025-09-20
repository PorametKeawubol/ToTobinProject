#!/bin/bash

# Update script for Kiosk App on Odroid C4

set -e

echo "🔄 Updating Kiosk App..."

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Pull latest changes (if using git)
if [ -d ".git" ]; then
    print_status "Pulling latest changes..."
    git pull
fi

# Rebuild and restart services
print_status "Rebuilding application..."
docker-compose -f docker-compose.prod.yml build --no-cache kiosk-app

print_status "Restarting services..."
docker-compose -f docker-compose.prod.yml up -d

print_status "Waiting for services to restart..."
sleep 20

# Check if services are running
if docker-compose -f docker-compose.prod.yml ps | grep -q "Up"; then
    print_status "Update completed successfully!"
else
    print_warning "Some services may not be running properly"
    docker-compose -f docker-compose.prod.yml ps
fi

print_status "Application updated and running at https://porametix.online"