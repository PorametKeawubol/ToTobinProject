#!/bin/bash

# TotoBin Kiosk - Google Cloud Deployment Script
# Run this script to deploy the entire system

set -e

PROJECT_ID="totobin-kiosk-2025"
REGION="asia-southeast1"
SERVICE_NAME="totobin-kiosk"
DOMAIN="porametix.online"

echo "🚀 Starting TotoBin Kiosk deployment..."

# Set project and region
echo "📝 Setting up Google Cloud project..."
gcloud config set project $PROJECT_ID
gcloud config set compute/region $REGION

# Enable required services
echo "🔧 Enabling Google Cloud services..."
gcloud services enable \
  cloudbuild.googleapis.com \
  run.googleapis.com \
  firestore.googleapis.com \
  pubsub.googleapis.com \
  cloudfunctions.googleapis.com \
  dns.googleapis.com \
  certificatemanager.googleapis.com

# Create Firestore database
echo "🗄️ Setting up Firestore database..."
gcloud firestore databases create --region=$REGION || echo "Database may already exist"

# Build and deploy
echo "🏗️ Building container image..."
gcloud builds submit --tag gcr.io/$PROJECT_ID/kiosk-app .

echo "☁️ Deploying to Cloud Run..."
gcloud run deploy $SERVICE_NAME \
  --image gcr.io/$PROJECT_ID/kiosk-app \
  --platform managed \
  --region $REGION \
  --allow-unauthenticated \
  --memory 1Gi \
  --cpu 1 \
  --concurrency 80 \
  --timeout 300 \
  --set-env-vars="GOOGLE_CLOUD_PROJECT_ID=$PROJECT_ID,GOOGLE_CLOUD_REGION=$REGION,HARDWARE_API_KEY=change-this-in-production,JWT_SECRET=change-this-secret-too"

# Get service URL
SERVICE_URL=$(gcloud run services describe $SERVICE_NAME \
  --region=$REGION \
  --format="value(status.url)")

echo "✅ Cloud Run deployment complete!"
echo "🔗 Service URL: $SERVICE_URL"

# Setup domain mapping
echo "🌐 Setting up domain mapping..."
gcloud run domain-mappings create \
  --service $SERVICE_NAME \
  --domain $DOMAIN \
  --region $REGION || echo "Domain mapping may already exist"

echo "🎉 Deployment complete!"
echo ""
echo "📋 Next steps:"
echo "1. Configure DNS at GoDaddy:"
echo "   - Add CNAME record: @ -> ghs.googlehosted.com"
echo "   - Add CNAME record: www -> ghs.googlehosted.com"
echo ""
echo "2. Update hardware configuration:"
echo "   - ESP32: baseURL = \"https://$DOMAIN/api\""
echo "   - Odroid: BASE_URL = \"https://$DOMAIN/api\""
echo ""
echo "3. Test the application:"
echo "   - Web: https://$DOMAIN"
echo "   - API: https://$DOMAIN/api/queue"
echo ""
echo "4. Monitor logs:"
echo "   gcloud logs tail \"resource.type=cloud_run_revision AND resource.labels.service_name=$SERVICE_NAME\""