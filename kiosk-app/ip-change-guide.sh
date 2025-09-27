#!/bin/bash

# IP Change Management Script
# สำหรับเมื่อ IP ของ Odroid เปลี่ยน

echo "🔄 IP Change Management for TotoBin Kiosk"
echo "========================================"

# Get current IP
CURRENT_IP=$(ip route get 1.1.1.1 | awk '{print $7}' | head -1)
echo "Current IP: $CURRENT_IP"

echo ""
echo "📋 Steps when IP changes:"
echo "========================="
echo "1. Update DNS A record: porametix.online → NEW_IP"
echo "2. Update .env.production NEXTAUTH_URL (if needed)"
echo "3. Restart services (usually not needed)"
echo ""

echo "🔧 What to update:"
echo "=================="
echo "DNS Provider:"
echo "  - Change A record from old IP to: $CURRENT_IP"
echo ""

echo "Environment file (.env.production):"
echo "  - NEXTAUTH_URL=https://porametix.online (keep as domain)"
echo "  - Other URLs should use domain name, not IP"
echo ""

echo "📝 Commands to run after IP change:"
echo "==================================="
echo "# 1. Check new IP"
echo "ip route get 1.1.1.1 | awk '{print \$7}' | head -1"
echo ""
echo "# 2. Test local access"
echo "curl -I http://localhost"
echo ""
echo "# 3. Wait for DNS propagation (5-60 minutes)"
echo "nslookup porametix.online"
echo ""
echo "# 4. Test domain access"
echo "curl -I http://porametix.online"
echo ""
echo "# 5. Update SSL if needed (only if certificate issues)"
echo "sudo certbot --nginx -d porametix.online --force-renewal"

echo ""
echo "💡 Pro Tips:"
echo "============"
echo "- Use domain names in configs, not IP addresses"
echo "- Most configs don't need changes when IP changes"
echo "- Only DNS needs updating in most cases"
echo "- Services continue running normally during IP change"