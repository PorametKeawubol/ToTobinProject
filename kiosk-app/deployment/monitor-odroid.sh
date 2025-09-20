#!/bin/bash
# ODROID C4 Performance Monitor for TotoBin Kiosk
# Lightweight monitoring without heavy dependencies

echo "📊 TotoBin Kiosk - ODROID C4 Performance Monitor"
echo "Domain: porametix.online"
echo "$(date)"
echo "=========================================="

# Colors
G='\033[0;32m'
Y='\033[1;33m'
R='\033[0;31m'
B='\033[0;34m'
NC='\033[0m'

info() { echo -e "${G}[INFO]${NC} $1"; }
warn() { echo -e "${Y}[WARN]${NC} $1"; }
error() { echo -e "${R}[ERROR]${NC} $1"; }
header() { echo -e "${B}=== $1 ===${NC}"; }

# System Information
header "System Information"
echo "Hostname: $(hostname)"
echo "Uptime: $(uptime -p)"
echo "Load Average: $(cat /proc/loadavg | cut -d' ' -f1-3)"
echo "Architecture: $(uname -m)"

# CPU Information
header "CPU Status"
echo "CPU Model: $(cat /proc/cpuinfo | grep 'model name' | head -1 | cut -d':' -f2 | xargs)"
echo "CPU Cores: $(nproc)"
echo "CPU Frequency:"
for i in {0..3}; do
    if [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_cur_freq" ]; then
        freq=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_cur_freq)
        freq_mhz=$((freq / 1000))
        echo "  CPU$i: ${freq_mhz} MHz"
    fi
done

# CPU Temperature
if [ -f "/sys/class/thermal/thermal_zone0/temp" ]; then
    temp=$(cat /sys/class/thermal/thermal_zone0/temp)
    temp_c=$((temp / 1000))
    echo "CPU Temp: ${temp_c}°C"
    
    if [ $temp_c -gt 70 ]; then
        warn "High CPU temperature detected!"
    fi
fi

# Memory Usage
header "Memory Usage"
free -h | grep -E 'Mem|Swap'

# Disk Usage
header "Disk Usage"
df -h / | tail -1

# Network Status
header "Network Status"
if command -v ip >/dev/null; then
    ip addr show | grep -A2 "state UP" | grep inet | head -2
else
    ifconfig | grep "inet " | head -2
fi

# Service Status
header "Service Status"
services=("totobin-kiosk" "nginx")
for service in "${services[@]}"; do
    if systemctl is-active --quiet $service; then
        info "✅ $service is running"
        
        # Show memory usage for the service
        if [ "$service" = "totobin-kiosk" ]; then
            pid=$(systemctl show --property MainPID --value $service)
            if [ "$pid" != "0" ]; then
                mem_kb=$(ps -o rss= -p $pid 2>/dev/null | xargs)
                if [ -n "$mem_kb" ]; then
                    mem_mb=$((mem_kb / 1024))
                    echo "   Memory: ${mem_mb} MB"
                fi
            fi
        fi
    else
        error "❌ $service is not running"
    fi
done

# Application Health Check
header "Application Health"
if curl -s -f https://porametix.online >/dev/null; then
    info "✅ Website is accessible"
else
    if curl -s -f http://localhost:3000 >/dev/null; then
        warn "⚠️  Local app OK, but HTTPS may have issues"
    else
        error "❌ Application is not responding"
    fi
fi

# API Health Check
if curl -s -f https://porametix.online/api/hardware/current-status >/dev/null; then
    info "✅ Hardware API is responding"
else
    warn "⚠️  Hardware API may have issues"
fi

# Recent Logs (last 5 lines)
header "Recent Application Logs"
journalctl -u totobin-kiosk --lines=5 --no-pager | tail -5

# Port Usage
header "Port Status"
if command -v ss >/dev/null; then
    echo "Listening ports:"
    ss -tlnp | grep -E ':(80|443|3000) '
else
    echo "Listening ports:"
    netstat -tlnp | grep -E ':(80|443|3000) '
fi

# Performance Recommendations
header "Performance Recommendations"

# Check CPU governor
if [ -f "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor" ]; then
    governor=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
    if [ "$governor" != "performance" ]; then
        warn "CPU governor is '$governor'. Consider 'performance' for better response times"
        echo "   Fix: echo 'performance' | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor"
    else
        info "CPU governor is optimized for performance"
    fi
fi

# Check memory usage
mem_usage=$(free | awk 'NR==2{printf "%.1f", $3/$2*100}')
if (( $(echo "$mem_usage > 80" | bc -l) )); then
    warn "High memory usage: ${mem_usage}%"
    echo "   Consider restarting the application if it's using too much memory"
fi

# Check swap usage
swap_usage=$(free | awk 'NR==3{if($2>0) printf "%.1f", $3/$2*100; else print "0"}')
if (( $(echo "$swap_usage > 10" | bc -l) )); then
    warn "Swap is being used: ${swap_usage}%"
    echo "   This may slow down the application"
fi

echo ""
echo "=========================================="
echo "Monitoring completed at $(date)"
echo ""
echo "📋 Quick Commands:"
echo "   Restart app: sudo systemctl restart totobin-kiosk"
echo "   Check logs: sudo journalctl -fu totobin-kiosk"
echo "   Update app: cd /opt/totobin-kiosk && sudo git pull && cd kiosk-app && sudo yarn build && sudo systemctl restart totobin-kiosk"
echo "   Monitor real-time: watch -n 5 'bash $(basename $0)'"