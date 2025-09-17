# Simple Direct Deployment for TotoBin Kiosk
# Deploy without complex builds

Write-Host "🚀 TotoBin Kiosk - Simple Direct Deployment" -ForegroundColor Green

$PROJECT_ID = "totobin-kiosk-porametix"
$REGION = "asia-southeast1"
$SERVICE_NAME = "totobin-kiosk"
$GCLOUD_PATH = "$env:USERPROFILE\AppData\Local\Google\Cloud SDK\google-cloud-sdk\bin\gcloud.cmd"

# Deploy directly from source
Write-Host "📦 Deploying from source..." -ForegroundColor Cyan

& $GCLOUD_PATH run deploy $SERVICE_NAME `
--source . `
--region $REGION `
--platform managed `
--port 3000 `
--allow-unauthenticated `
--memory 1Gi `
--cpu 1 `
--min-instances 0 `
--max-instances 10 `
--timeout 300 `
--concurrency 80 `
--set-env-vars NODE_ENV=production,PORT=3000,HARDWARE_API_KEY=totobin-hardware-secret-2024 `
--project $PROJECT_ID

if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Deployment successful!" -ForegroundColor Green
    
    # Get service URL
    $SERVICE_URL = & $GCLOUD_PATH run services describe $SERVICE_NAME --region $REGION --format 'value(status.url)' --project $PROJECT_ID
    
    Write-Host ""
    Write-Host "🎉 TotoBin Kiosk is LIVE!" -ForegroundColor Green
    Write-Host "🔗 URL: $SERVICE_URL" -ForegroundColor Yellow
    Write-Host "💰 PromptPay: 0984435255" -ForegroundColor White
    Write-Host "🔑 Hardware API Key: totobin-hardware-secret-2024" -ForegroundColor White
    
    # Create Pub/Sub topic
    Write-Host ""
    Write-Host "📡 Setting up Pub/Sub..." -ForegroundColor Cyan
    & $GCLOUD_PATH pubsub topics create order-status-updates --project $PROJECT_ID
    
    Write-Host ""
    Write-Host "✅ Deployment Complete!" -ForegroundColor Green
    Write-Host "🌐 Access your kiosk at: $SERVICE_URL" -ForegroundColor Cyan
    
} else {
    Write-Host "❌ Deployment failed!" -ForegroundColor Red
    Write-Host "Check the logs above for errors." -ForegroundColor Yellow
}