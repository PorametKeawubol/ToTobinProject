#!/bin/bash

# Code Update & Deployment Script
# สำหรับอัพเดท code และ deploy ใหม่

echo "🚀 TotoBin Kiosk - Code Update & Deployment"
echo "==========================================="

APP_DIR="/home/odroid/totobin-app/kiosk-app"
PM2_APP_NAME="totobin-kiosk"

cd "$APP_DIR" || exit 1

echo "📊 Current Status:"
echo "=================="
pm2 status

echo ""
echo "🔄 Starting deployment process..."
echo "================================="

# Step 1: Backup current version
echo "1. Creating backup..."
BACKUP_DIR="backup-$(date +%Y%m%d-%H%M%S)"
mkdir -p "../$BACKUP_DIR"
cp -r .next "../$BACKUP_DIR/" 2>/dev/null || echo "No .next directory to backup"
echo "   ✅ Backup created: ../$BACKUP_DIR"

# Step 2: Pull latest code (if using git)
echo ""
echo "2. Updating code..."
if [ -d ".git" ]; then
    echo "   📥 Pulling from git..."
    git pull origin main || {
        echo "   ⚠️  Git pull failed, continuing with local changes..."
    }
else
    echo "   📁 No git repository, using local changes"
fi

# Step 3: Install dependencies
echo ""
echo "3. Installing dependencies..."
npm install

# Step 4: Build application
echo ""
echo "4. Building application..."
echo "   🔨 Building Next.js app..."
npm run build || {
    echo "   ❌ Build failed! Restoring from backup..."
    if [ -d "../$BACKUP_DIR/.next" ]; then
        cp -r "../$BACKUP_DIR/.next" ./
    fi
    echo "   ❌ Deployment failed!"
    exit 1
}

# Step 5: Restart PM2 application
echo ""
echo "5. Restarting application..."
pm2 restart "$PM2_APP_NAME" || {
    echo "   ❌ PM2 restart failed!"
    exit 1
}

# Step 6: Health check
echo ""
echo "6. Health check..."
sleep 5

# Wait for app to start
for i in {1..10}; do
    if curl -f -s http://localhost:3000/api/health > /dev/null; then
        echo "   ✅ Health check passed!"
        break
    else
        echo "   ⏳ Waiting for app to start... ($i/10)"
        sleep 2
    fi
    
    if [ $i -eq 10 ]; then
        echo "   ❌ Health check failed!"
        echo "   🔄 Rolling back..."
        pm2 stop "$PM2_APP_NAME"
        if [ -d "../$BACKUP_DIR/.next" ]; then
            cp -r "../$BACKUP_DIR/.next" ./
        fi
        pm2 start "$PM2_APP_NAME"
        exit 1
    fi
done

# Step 7: Final status check
echo ""
echo "7. Final status:"
echo "================"
pm2 status
echo ""
curl -s http://localhost/api/health | jq . 2>/dev/null || curl -s http://localhost/api/health

echo ""
echo "✅ Deployment completed successfully!"
echo ""
echo "🌐 Access points:"
echo "   Local:  http://localhost"
echo "   Domain: http://porametix.online"
echo ""
echo "📋 Useful commands:"
echo "   View logs:    pm2 logs $PM2_APP_NAME"
echo "   Monitor:      pm2 monit"
echo "   Status:       pm2 status"