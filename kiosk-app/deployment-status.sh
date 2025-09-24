#!/bin/bash

# Odroid Deployment Guide for porametix.online
# TotoBin Kiosk App Native Deployment (without Docker)

echo "🚀 TotoBin Kiosk App - Native Deployment on Odroid C4"
echo "Domain: porametix.online"
echo ""

# แสดงสถานะปัจจุบัน
echo "📊 Current Status:"
echo "=================="

# ตรวจสอบ PM2
echo "1. PM2 Process Status:"
pm2 status

echo ""
echo "2. Nginx Status:"
sudo systemctl status nginx --no-pager -l

echo ""
echo "3. App Health Check:"
curl -s http://localhost/api/health | jq . 2>/dev/null || curl -s http://localhost/api/health

echo ""
echo "4. Network Interface:"
ip addr show | grep inet | grep -v 127.0.0.1

echo ""
echo "🌐 Access Points:"
echo "=================="
echo "Local:      http://localhost:3000"
echo "Nginx:      http://localhost:80"
echo "Public:     http://porametix.online (if DNS is configured)"

echo ""
echo "📋 Management Commands:"
echo "======================="
echo "View App Logs:    pm2 logs totobin-kiosk"
echo "Restart App:      pm2 restart totobin-kiosk"
echo "Stop App:         pm2 stop totobin-kiosk"
echo "Start App:        pm2 start totobin-kiosk"
echo "Nginx Reload:     sudo systemctl reload nginx"
echo "Nginx Status:     sudo systemctl status nginx"

echo ""
echo "🔧 Next Steps:"
echo "==============="
echo "1. Configure DNS: Point porametix.online to this server's IP"
echo "2. Setup SSL: Run 'sudo certbot --nginx -d porametix.online'"
echo "3. Monitor: Use 'pm2 monit' to monitor performance"

echo ""
echo "📁 File Locations:"
echo "=================="
echo "App Directory:    /home/odroid/totobin-app/kiosk-app"
echo "Nginx Config:     /etc/nginx/sites-available/porametix.conf"
echo "PM2 Logs:         /home/odroid/.pm2/logs/"
echo "Environment:      /home/odroid/totobin-app/kiosk-app/.env.production"

echo ""
echo "✅ Deployment Complete!"