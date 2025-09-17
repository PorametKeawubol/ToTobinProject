# Deploy TotoBin Kiosk to Cloud Run

Write-Host "Deploying TotoBin Kiosk to Cloud Run..." -ForegroundColor Green

# Set project and region
$PROJECT_ID = "totobin-kiosk-porametix"
$REGION = "asia-southeast1"
$SERVICE_NAME = "totobin-kiosk"
$IMAGE_URL = "gcr.io/$PROJECT_ID/$SERVICE_NAME"
$GCLOUD_PATH = "$env:USERPROFILE\AppData\Local\Google\Cloud SDK\google-cloud-sdk\bin\gcloud.cmd"

# Deploy to Cloud Run
Write-Host "Deploying service..." -ForegroundColor Yellow
& $GCLOUD_PATH run deploy $SERVICE_NAME `
 --image $IMAGE_URL `
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
 --set-env-vars NODE_ENV=production, PORT=3000 `
 --project $PROJECT_ID

if ($LASTEXITCODE -eq 0) {
 Write-Host "Getting service URL..." -ForegroundColor Yellow
 $SERVICE_URL = & $GCLOUD_PATH run services describe $SERVICE_NAME `
  --region $REGION `
  --format 'value(status.url)' `
  --project $PROJECT_ID

 Write-Host "TotoBin Kiosk deployed at: $SERVICE_URL" -ForegroundColor Green

 # Create domain mapping
 Write-Host "Setting up domain mapping for porametix.online..." -ForegroundColor Yellow
 & $GCLOUD_PATH run domain-mappings create `
  --service $SERVICE_NAME `
  --domain porametix.online `
  --region $REGION `
  --project $PROJECT_ID

 Write-Host "Deployment complete!" -ForegroundColor Green
 Write-Host "Service URL: $SERVICE_URL" -ForegroundColor Cyan
 Write-Host "Custom Domain: https://porametix.online" -ForegroundColor Cyan
 Write-Host ""
 Write-Host "To verify domain mapping, run:" -ForegroundColor Yellow
 Write-Host "gcloud run domain-mappings list --region $REGION --project $PROJECT_ID" -ForegroundColor Gray
}
else {
 Write-Host "Deployment failed!" -ForegroundColor Red
}