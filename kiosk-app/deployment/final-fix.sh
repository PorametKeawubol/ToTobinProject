#!/bin/bash
# Final fix for TypeScript config issue

set -e

echo "🔧 Final fix for TypeScript config issue..."

APP_DIR="/opt/totobin/ToTobinProject/kiosk-app"
cd $APP_DIR

# Stop PM2 process
echo "Stopping PM2 process..."
pm2 delete totobin-kiosk 2>/dev/null || true

# Remove all TypeScript configs and use JavaScript only
echo "Removing TypeScript configs and using JavaScript only..."

# Remove any existing TypeScript configs
rm -f next.config.ts next.config.ts.bak

# Copy the working JavaScript config
if [ -f "next.config.odroid.js" ]; then
    cp next.config.odroid.js next.config.js
    echo "✅ Using JavaScript config"
else
    # Create a minimal JavaScript config
    cat > next.config.js << 'EOF'
/** @type {import('next').NextConfig} */
const nextConfig = {
  output: "standalone",
  experimental: {
    serverComponentsExternalPackages: ["@google-cloud/firestore"],
  },
  compiler: {
    removeConsole: process.env.NODE_ENV === "production",
  },
  env: {
    HARDWARE_API_KEY: process.env.HARDWARE_API_KEY,
    JWT_SECRET: process.env.JWT_SECRET,
    PROMPTPAY_PHONE: process.env.PROMPTPAY_PHONE,
    BUSINESS_NAME: process.env.BUSINESS_NAME,
  },
};

module.exports = nextConfig;
EOF
    echo "✅ Created minimal JavaScript config"
fi

# Make sure we have production dependencies for runtime
echo "Installing production dependencies..."
npm install --only=production

# Create updated PM2 config with better error handling
cat > ecosystem.config.js << 'EOF'
module.exports = {
  apps: [{
    name: 'totobin-kiosk',
    script: './node_modules/.bin/next',
    args: 'start',
    cwd: '/opt/totobin/ToTobinProject/kiosk-app',
    instances: 1,
    exec_mode: 'fork',
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
    max_restarts: 10,
    min_uptime: '10s',
    kill_timeout: 5000,
    restart_delay: 4000
  }]
}
EOF

# Ensure log directory exists
mkdir -p logs

# Start with PM2 using direct Next.js command
echo "Starting application with direct Next.js command..."
pm2 start ecosystem.config.js

# Wait longer for startup
echo "Waiting for application to start..."
sleep 15

# Check if it's running
echo "Checking application status..."
pm2 status

# Test health endpoint
echo "Testing health endpoint..."
for i in {1..6}; do
    if curl -f http://localhost:3000/api/health >/dev/null 2>&1; then
        echo "✅ Application is running successfully!"
        echo "📊 PM2 Status:"
        pm2 status
        echo "🌐 Application URLs:"
        echo "- Local: http://localhost:3000"
        echo "- Health: http://localhost:3000/api/health"
        break
    else
        echo "Attempt $i/6: Health check failed, waiting..."
        if [ $i -eq 6 ]; then
            echo "❌ Health check still failing after 6 attempts"
            echo "📋 PM2 Status:"
            pm2 status
            echo "📋 Recent Error Logs:"
            pm2 logs --err --lines 20
            echo "📋 Recent Output Logs:"
            pm2 logs --out --lines 20
        else
            sleep 5
        fi
    fi
done

# Save PM2 configuration
pm2 save

echo "🎉 Deployment completed!"