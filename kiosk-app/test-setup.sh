#!/bin/bash

# Quick test script for deployment

echo "🧪 Testing Odroid C4 deployment setup..."

# Test Docker
echo "Checking Docker..."
if command -v docker &> /dev/null; then
    echo "✅ Docker installed: $(docker --version)"
else
    echo "❌ Docker not installed"
fi

# Test Docker Compose
echo "Checking Docker Compose..."
if command -v docker-compose &> /dev/null; then
    echo "✅ Docker Compose installed: $(docker-compose --version)"
else
    echo "❌ Docker Compose not installed"
fi

# Test compose file
echo "Validating docker-compose.prod.yml..."
if docker-compose -f docker-compose.prod.yml config &> /dev/null; then
    echo "✅ Docker Compose file is valid"
else
    echo "❌ Docker Compose file has errors:"
    docker-compose -f docker-compose.prod.yml config
fi

# Test if required files exist
echo "Checking required files..."
files=("Dockerfile" "docker-compose.prod.yml" "nginx/nginx.conf" "nginx/conf.d/kiosk.conf")
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
    fi
done

# Test environment file
if [ -f ".env.production" ]; then
    echo "✅ .env.production exists"
else
    echo "⚠️ .env.production not found (will be created during deployment)"
fi

echo ""
echo "📋 Summary:"
echo "If all checks pass, you can run: ./deploy-odroid.sh"