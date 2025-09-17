# TotoBin Kiosk - Complete Google Cloud Deployment

Write-Host "🚀 Starting TotoBin Kiosk Deployment to Google Cloud..." -ForegroundColor Green

$PROJECT_ID = "totobin-kiosk-porametix"
$REGION = "asia-southeast1"
$SERVICE_NAME = "totobin-kiosk"
$DOMAIN = "porametix.online"
$GCLOUD_PATH = "$env:USERPROFILE\AppData\Local\Google\Cloud SDK\google-cloud-sdk\bin\gcloud.cmd"

# Function to run gcloud commands
function Invoke-GCloud {
    param([string]$Command)
    $FullCommand = "$GCLOUD_PATH $Command"
    Write-Host "Executing: gcloud $Command" -ForegroundColor Yellow
    Invoke-Expression $FullCommand
    return $LASTEXITCODE
}

# 1. Check build status
Write-Host "`n📦 Checking latest build status..." -ForegroundColor Cyan
$BuildResult = Invoke-GCloud "builds list --limit=1 --format='value(status)' --project=$PROJECT_ID"

if ($BuildResult -eq 0) {
    Write-Host "✅ Build check completed" -ForegroundColor Green
} else {
    Write-Host "❌ Build status check failed" -ForegroundColor Red
    exit 1
}

# 2. Deploy to Cloud Run
Write-Host "`n🚀 Deploying to Cloud Run..." -ForegroundColor Cyan
$DeployResult = Invoke-GCloud @"
run deploy $SERVICE_NAME \
--image gcr.io/$PROJECT_ID/$SERVICE_NAME \
--region $REGION \
--platform managed \
--port 3000 \
--allow-unauthenticated \
--memory 1Gi \
--cpu 1 \
--min-instances 0 \
--max-instances 10 \
--timeout 300 \
--concurrency 80 \
--set-env-vars NODE_ENV=production,PORT=3000,HARDWARE_API_KEY=totobin-hardware-secret-2024 \
--project $PROJECT_ID
"@

if ($DeployResult -eq 0) {
    Write-Host "✅ Cloud Run deployment successful" -ForegroundColor Green
} else {
    Write-Host "❌ Cloud Run deployment failed" -ForegroundColor Red
    exit 1
}

# 3. Get service URL
Write-Host "`n🔗 Getting service URL..." -ForegroundColor Cyan
$SERVICE_URL = & $GCLOUD_PATH run services describe $SERVICE_NAME --region $REGION --format 'value(status.url)' --project $PROJECT_ID

if ($SERVICE_URL) {
    Write-Host "✅ Service URL: $SERVICE_URL" -ForegroundColor Green
} else {
    Write-Host "❌ Failed to get service URL" -ForegroundColor Red
}

# 4. Setup domain mapping
Write-Host "`n🌐 Setting up domain mapping for $DOMAIN..." -ForegroundColor Cyan
$DomainResult = Invoke-GCloud "run domain-mappings create --service $SERVICE_NAME --domain $DOMAIN --region $REGION --project $PROJECT_ID"

if ($DomainResult -eq 0) {
    Write-Host "✅ Domain mapping created successfully" -ForegroundColor Green
} else {
    Write-Host "⚠️  Domain mapping may have failed - check DNS settings" -ForegroundColor Yellow
}

# 5. Create Pub/Sub topic for hardware communication
Write-Host "`n📡 Setting up Pub/Sub topics..." -ForegroundColor Cyan
$PubSubResult = Invoke-GCloud "pubsub topics create order-status-updates --project $PROJECT_ID"

if ($PubSubResult -eq 0 -or $LASTEXITCODE -eq 1) {
    Write-Host "✅ Pub/Sub topic ready" -ForegroundColor Green
} else {
    Write-Host "❌ Pub/Sub setup failed" -ForegroundColor Red
}

# 6. Display deployment summary
Write-Host "`n🎉 Deployment Summary" -ForegroundColor Green
Write-Host "===========================================" -ForegroundColor White
Write-Host "🏪 TotoBin Kiosk System" -ForegroundColor Cyan
Write-Host "📱 Project: $PROJECT_ID" -ForegroundColor White
Write-Host "🌍 Region: $REGION" -ForegroundColor White
Write-Host "🔗 Service URL: $SERVICE_URL" -ForegroundColor Yellow
Write-Host "🌐 Custom Domain: https://$DOMAIN" -ForegroundColor Yellow
Write-Host "🛠️  Hardware API Key: totobin-hardware-secret-2024" -ForegroundColor White
Write-Host "💰 PromptPay: 0984435255" -ForegroundColor White
Write-Host ""
Write-Host "📋 Next Steps:" -ForegroundColor Cyan
Write-Host "1. Configure DNS for $DOMAIN to point to Google Cloud" -ForegroundColor White
Write-Host "2. Update ESP32/Odroid hardware with production endpoints" -ForegroundColor White
Write-Host "3. Test the complete system end-to-end" -ForegroundColor White
Write-Host ""
Write-Host "🔧 Management Commands:" -ForegroundColor Cyan
Write-Host "View logs: gcloud run logs tail $SERVICE_NAME --region $REGION" -ForegroundColor Gray
Write-Host "Scale service: gcloud run services update $SERVICE_NAME --max-instances 20 --region $REGION" -ForegroundColor Gray
Write-Host "Check domain: gcloud run domain-mappings list --region $REGION" -ForegroundColor Gray