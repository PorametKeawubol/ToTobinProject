# Google Cloud Deployment Scripts

## Prerequisites
```bash
# Install Google Cloud CLI
# https://cloud.google.com/sdk/docs/install

# Login and set project
gcloud auth login
gcloud config set project totobin-kiosk-2025
gcloud config set compute/region asia-southeast1
```

## 1. Enable Required Services
```bash
gcloud services enable \
  cloudbuild.googleapis.com \
  run.googleapis.com \
  firestore.googleapis.com \
  pubsub.googleapis.com \
  cloudfunctions.googleapis.com \
  dns.googleapis.com \
  certificatemanager.googleapis.com
```

## 2. Setup Firestore Database
```bash
# Create Firestore database
gcloud firestore databases create --region=asia-southeast1
```

## 3. Build and Deploy to Cloud Run
```bash
# Build container image
gcloud builds submit --tag gcr.io/totobin-kiosk-2025/kiosk-app

# Deploy to Cloud Run
gcloud run deploy totobin-kiosk \
  --image gcr.io/totobin-kiosk-2025/kiosk-app \
  --platform managed \
  --region asia-southeast1 \
  --allow-unauthenticated \
  --memory 1Gi \
  --cpu 1 \
  --concurrency 80 \
  --timeout 300 \
  --set-env-vars="GOOGLE_CLOUD_PROJECT_ID=totobin-kiosk-2025,GOOGLE_CLOUD_REGION=asia-southeast1,HARDWARE_API_KEY=your-secure-api-key-here,JWT_SECRET=your-jwt-secret-here"
```

## 4. Setup Custom Domain (porametix.online)
```bash
# Create Cloud DNS zone (if using Google DNS)
gcloud dns managed-zones create porametix-online \
  --dns-name="porametix.online." \
  --description="TotoBin Kiosk Domain"

# Get Cloud Run service URL
SERVICE_URL=$(gcloud run services describe totobin-kiosk \
  --region=asia-southeast1 \
  --format="value(status.url)")

# Create domain mapping
gcloud run domain-mappings create \
  --service totobin-kiosk \
  --domain porametix.online \
  --region asia-southeast1

# Verify domain ownership (follow prompts)
```

## 5. Setup SSL Certificate
```bash
# SSL certificate will be automatically provisioned by Cloud Run
# when domain mapping is created and DNS is properly configured
```

## 6. Configure DNS at GoDaddy
```
# Add these DNS records at GoDaddy:
# Type: CNAME
# Name: @ (or leave blank for root domain)
# Value: ghs.googlehosted.com

# Type: CNAME  
# Name: www
# Value: ghs.googlehosted.com
```

## 7. Environment Variables
Create `.env.production` file:
```bash
GOOGLE_CLOUD_PROJECT_ID=totobin-kiosk-2025
GOOGLE_CLOUD_REGION=asia-southeast1
NEXT_PUBLIC_DOMAIN=https://porametix.online
HARDWARE_API_KEY=your-secure-hardware-api-key
JWT_SECRET=your-secure-jwt-secret
MAX_CONCURRENT_ORDERS=1
ESTIMATED_BREWING_TIME=45
```

## 8. Monitor Deployment
```bash
# Check Cloud Run service status
gcloud run services describe totobin-kiosk --region=asia-southeast1

# View logs
gcloud logs tail "resource.type=cloud_run_revision AND resource.labels.service_name=totobin-kiosk"

# Check domain mapping status
gcloud run domain-mappings describe porametix.online --region=asia-southeast1
```

## 9. Test Hardware Connection
```bash
# Test hardware API endpoint
curl -X GET "https://porametix.online/api/hardware/orders?hardwareId=esp32-001" \
  -H "X-API-Key: your-hardware-api-key"

# Test status update
curl -X POST "https://porametix.online/api/hardware/status" \
  -H "Content-Type: application/json" \
  -H "X-API-Key: your-hardware-api-key" \
  -d '{
    "orderId": "test",
    "status": "preparing", 
    "step": "preparing_cup",
    "message": "Testing connection",
    "hardwareId": "esp32-001"
  }'
```

## 10. Update Hardware Configuration
Update hardware client code with production URLs:
- ESP32: `const char* baseURL = "https://porametix.online/api";`
- Odroid: `BASE_URL = "https://porametix.online/api"`