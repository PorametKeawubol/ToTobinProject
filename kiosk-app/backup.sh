#!/bin/bash

# Backup script for Kiosk App

DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="backups"
BACKUP_NAME="kiosk-backup-${DATE}"

echo "🔄 Creating backup: $BACKUP_NAME"

# Create backup directory
mkdir -p $BACKUP_DIR

# Stop services
echo "Stopping services..."
docker-compose -f docker-compose.prod.yml down

# Create backup
echo "Creating backup archive..."
tar -czf "${BACKUP_DIR}/${BACKUP_NAME}.tar.gz" \
    --exclude='.git' \
    --exclude='node_modules' \
    --exclude='.next' \
    --exclude='backups' \
    .

# Backup Docker volumes
echo "Backing up SSL certificates..."
docker run --rm -v kiosk-app_certbot-etc:/data -v $(pwd)/${BACKUP_DIR}:/backup alpine tar czf /backup/ssl-${DATE}.tar.gz -C /data .

# Start services again
echo "Starting services..."
docker-compose -f docker-compose.prod.yml up -d

echo "✅ Backup completed: ${BACKUP_DIR}/${BACKUP_NAME}.tar.gz"
echo "✅ SSL backup: ${BACKUP_DIR}/ssl-${DATE}.tar.gz"