#!/bin/sh

# Cloud Run startup script for TotoBin Kiosk

echo "Starting TotoBin Kiosk..."
echo "NODE_ENV: $NODE_ENV"
echo "PORT: $PORT"

# Start the Next.js application
exec node server.js