#!/bin/bash
# Fix TypeScript build issue and continue deployment

set -e

echo "🔧 Fixing TypeScript build issue..."

APP_DIR="/opt/totobin/ToTobinProject/kiosk-app"
cd $APP_DIR

# Install all dependencies including TypeScript
echo "Installing all dependencies including TypeScript..."
npm install

# Use JavaScript config instead of TypeScript to avoid compilation issues
echo "Using JavaScript config instead of TypeScript..."
if [ -f "next.config.odroid.js" ]; then
    cp next.config.odroid.js next.config.js
    echo "✅ Using ODROID optimized JavaScript config"
fi

# Remove TypeScript config temporarily during build
if [ -f "next.config.ts" ]; then
    mv next.config.ts next.config.ts.bak
    echo "📦 Backed up TypeScript config"
fi

# Build with increased memory
echo "Building application with optimizations..."
NODE_OPTIONS="--max-old-space-size=1536" npm run build

# Restore TypeScript config if it was backed up
if [ -f "next.config.ts.bak" ]; then
    mv next.config.ts.bak next.config.ts
    echo "📦 Restored TypeScript config"
fi

# Create PM2 config
cat > ecosystem.config.js << 'EOF'
module.exports = {
  apps: [{
    name: 'totobin-kiosk',
    script: 'npm',
    args: 'start',
    instances: 1,
    max_memory_restart: '768M',
    env: {
      NODE_ENV: 'production',
      PORT: '3000'
    },
    env_file: '.env.production',
    error_file: './logs/err.log',
    out_file: './logs/out.log',
    log_file: './logs/combined.log',
    time: true,
    autorestart: true,
    max_restarts: 5,
    min_uptime: '10s'
  }]
}
EOF

# Clean up node_modules to save space after build
echo "Cleaning up development dependencies..."
npm prune --production

# Create directories
mkdir -p data logs

# Start with PM2
echo "Starting application with PM2..."
pm2 delete totobin-kiosk 2>/dev/null || true
pm2 start ecosystem.config.js
pm2 save

# Test application
echo "Testing application..."
sleep 10

if curl -f http://localhost:3000/api/health >/dev/null 2>&1; then
    echo "✅ Application is running successfully!"
    
    echo "📊 PM2 Status:"
    pm2 status
    
    echo "💾 Memory Usage:"
    pm2 monit --lines 5
    
    echo "🌐 Application URLs:"
    echo "- Local: http://localhost:3000"
    echo "- Health: http://localhost:3000/api/health"
    
else
    echo "❌ Application health check failed"
    echo "📋 PM2 Status:"
    pm2 status
    echo "📋 Recent Logs:"
    pm2 logs --lines 20
    exit 1
fi

echo "🎉 Deployment fixed and completed!"