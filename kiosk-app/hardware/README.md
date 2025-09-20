# Hardware Requirements

## ESP32 Setup
```bash
# Required Arduino Libraries:
# - WiFi (built-in)
# - HTTPClient (built-in) 
# - ArduinoJson

# Installation via Arduino IDE:
# Tools -> Manage Libraries -> Search "ArduinoJson" -> Install
```

## Odroid Setup
```bash
# Install required Python packages
sudo apt update
sudo apt install python3-pip python3-venv

# Create virtual environment
python3 -m venv /opt/totobin-hardware
source /opt/totobin-hardware/bin/activate

# Install dependencies
pip install requests RPi.GPIO

# Setup systemd service
sudo cp odroid_service.service /etc/systemd/system/totobin-hardware.service
sudo systemctl enable totobin-hardware
sudo systemctl start totobin-hardware
```

## Pin Configuration

### ESP32:
- LED_PIN = 2 (Status indicator)
- PUMP_PIN = 4 (Drink pump)
- VALVE_PIN = 5 (Topping valve)
- SENSOR_PIN = 34 (Cup sensor)

## WiFi Configuration
Update these variables in the code:
- ssid = "YOUR_WIFI_SSID"
- password = "YOUR_WIFI_PASSWORD"

## API Configuration
- baseURL = "https://porametix.online/api"
- hardwareAPIKey = "dev-hardware-key" (change in production)
- hardwareId = "esp32-001" or "odroid-001"