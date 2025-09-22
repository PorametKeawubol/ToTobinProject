#!/bin/bash

# Quick Deploy Script - One command deployment
# ใช้เมื่อต้องการ deploy เร็วๆ

set -e  # Exit on error

echo "⚡ Quick Deploy - TotoBin Kiosk"
echo "=============================="

cd /home/odroid/totobin-app/kiosk-app

# Quick build and restart
echo "🔨 Building..."
npm run build

echo "🔄 Restarting..."
pm2 restart totobin-kiosk

echo "⏳ Waiting for app..."
sleep 3

# Quick health check
if curl -f -s http://localhost/api/health > /dev/null; then
    echo "✅ Deploy successful!"
    echo "🌐 App running at: http://porametix.online"
else
    echo "❌ Deploy failed - check logs with: pm2 logs totobin-kiosk"
    exit 1
fi