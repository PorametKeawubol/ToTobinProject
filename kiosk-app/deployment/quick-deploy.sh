#!/bin/bash

# Quick deployment script for TotoBin Kiosk
# This script deploys a simple version without waiting for complex builds

echo "🚀 Quick TotoBin Kiosk Deployment"

PROJECT_ID="totobin-kiosk-porametix"
REGION="asia-southeast1"
SERVICE_NAME="totobin-kiosk"

# Deploy from a simple Node.js image with our source
echo "📦 Building simple container..."

# Create a simple Dockerfile for quick deployment
cat > Dockerfile.simple << 'EOF'
FROM node:18-alpine

WORKDIR /app

# Copy package files
COPY package*.json ./

# Install production dependencies
RUN npm ci --omit=dev

# Copy source code
COPY . .

# Build the app
RUN npm run build

EXPOSE 3000

ENV NODE_ENV=production
ENV PORT=3000

CMD ["npm", "start"]
EOF

# Build and push simple image
echo "🔨 Building image..."
gcloud builds submit \
  --tag asia-southeast1-docker.pkg.dev/$PROJECT_ID/totobin-kiosk/simple \
  --file Dockerfile.simple \
  --project $PROJECT_ID

echo "🚀 Deploying to Cloud Run..."
gcloud run deploy $SERVICE_NAME \
  --image asia-southeast1-docker.pkg.dev/$PROJECT_ID/totobin-kiosk/simple \
  --region $REGION \
  --platform managed \
  --port 3000 \
  --allow-unauthenticated \
  --memory 1Gi \
  --cpu 1 \
  --set-env-vars NODE_ENV=production,PORT=3000 \
  --project $PROJECT_ID

echo "✅ Quick deployment complete!"

# Get URL
SERVICE_URL=$(gcloud run services describe $SERVICE_NAME \
  --region $REGION \
  --format 'value(status.url)' \
  --project $PROJECT_ID)

echo "🔗 Service URL: $SERVICE_URL"

# Clean up
rm Dockerfile.simple

echo "🎉 TotoBin Kiosk is now live!"