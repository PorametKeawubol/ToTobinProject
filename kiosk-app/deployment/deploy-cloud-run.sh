#!/bin/bash

# Deploy TotoBin Kiosk to Cloud Run

echo "Deploying TotoBin Kiosk to Cloud Run..."

# Set project and region
PROJECT_ID="totobin-kiosk-porametix"
REGION="asia-southeast1"
SERVICE_NAME="totobin-kiosk"
IMAGE_URL="gcr.io/${PROJECT_ID}/${SERVICE_NAME}"

# Deploy to Cloud Run
gcloud run deploy ${SERVICE_NAME} \
  --image ${IMAGE_URL} \
  --region ${REGION} \
  --platform managed \
  --port 3000 \
  --allow-unauthenticated \
  --memory 1Gi \
  --cpu 1 \
  --min-instances 0 \
  --max-instances 10 \
  --timeout 300 \
  --concurrency 80 \
  --set-env-vars NODE_ENV=production,PORT=3000 \
  --project ${PROJECT_ID}

echo "Getting service URL..."
SERVICE_URL=$(gcloud run services describe ${SERVICE_NAME} \
  --region ${REGION} \
  --format 'value(status.url)' \
  --project ${PROJECT_ID})

echo "TotoBin Kiosk deployed at: ${SERVICE_URL}"

# Create domain mapping
echo "Setting up domain mapping for porametix.online..."
gcloud run domain-mappings create \
  --service ${SERVICE_NAME} \
  --domain porametix.online \
  --region ${REGION} \
  --project ${PROJECT_ID}

echo "Deployment complete!"
echo "Service URL: ${SERVICE_URL}"
echo "Custom Domain: https://porametix.online"
echo ""
echo "To verify domain mapping, run:"
echo "gcloud run domain-mappings list --region ${REGION} --project ${PROJECT_ID}"