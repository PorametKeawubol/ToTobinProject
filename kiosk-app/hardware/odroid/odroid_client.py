#!/usr/bin/env python3
"""
TotoBin Odroid Client
Connects to TotoBin Kiosk API and controls drink making hardware
"""

import time
import json
import requests
import logging
from datetime import datetime
import RPi.GPIO as GPIO  # For Odroid GPIO control

# Configuration
BASE_URL = "https://porametix.online/api"
HARDWARE_API_KEY = "dev-hardware-key"
HARDWARE_ID = "odroid-001"
POLL_INTERVAL = 5  # seconds
HEARTBEAT_INTERVAL = 30  # seconds

# GPIO Configuration (adjust based on your hardware)
GPIO_PINS = {
    'LED': 18,
    'PUMP': 24,
    'VALVE': 25,
    'SENSOR': 23
}

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/var/log/totobin-hardware.log'),
        logging.StreamHandler()
    ]
)

class TotoBinHardware:
    def __init__(self):
        self.current_order_id = None
        self.is_brewing = False
        self.last_poll_time = 0
        self.last_heartbeat = 0
        
        # Setup GPIO
        self.setup_gpio()
        
        logging.info(f"TotoBin Odroid Hardware initialized - ID: {HARDWARE_ID}")
    
    def setup_gpio(self):
        """Initialize GPIO pins"""
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)
        
        # Setup output pins
        GPIO.setup(GPIO_PINS['LED'], GPIO.OUT)
        GPIO.setup(GPIO_PINS['PUMP'], GPIO.OUT)
        GPIO.setup(GPIO_PINS['VALVE'], GPIO.OUT)
        
        # Setup input pins
        GPIO.setup(GPIO_PINS['SENSOR'], GPIO.IN, pull_up_down=GPIO.PUD_UP)
        
        # Turn off all outputs
        GPIO.output(GPIO_PINS['LED'], GPIO.LOW)
        GPIO.output(GPIO_PINS['PUMP'], GPIO.LOW)
        GPIO.output(GPIO_PINS['VALVE'], GPIO.LOW)
        
        logging.info("GPIO initialized successfully")
    
    def cleanup_gpio(self):
        """Clean up GPIO resources"""
        GPIO.cleanup()
        logging.info("GPIO cleaned up")
    
    def make_api_request(self, method, endpoint, data=None):
        """Make API request with error handling"""
        url = f"{BASE_URL}{endpoint}"
        headers = {
            'X-API-Key': HARDWARE_API_KEY,
            'Content-Type': 'application/json'
        }
        
        try:
            if method == 'GET':
                response = requests.get(url, headers=headers, timeout=10)
            elif method == 'POST':
                response = requests.post(url, headers=headers, json=data, timeout=10)
            else:
                raise ValueError(f"Unsupported method: {method}")
            
            response.raise_for_status()
            return response.json()
            
        except requests.exceptions.RequestException as e:
            logging.error(f"API request failed: {e}")
            return None
    
    def poll_for_orders(self):
        """Poll for new orders from the API"""
        logging.debug("Polling for new orders...")
        
        response = self.make_api_request('GET', f'/hardware/orders?hardwareId={HARDWARE_ID}')
        
        if response and response.get('success'):
            order = response.get('order')
            if order:
                logging.info(f"New order received: {order['id']}")
                logging.info(f"Drink: {order['drinkName']}")
                
                self.current_order_id = order['id']
                self.start_brewing(order)
            else:
                logging.debug("No pending orders")
        else:
            logging.warning("Failed to poll orders")
    
    def start_brewing(self, order):
        """Start the brewing process"""
        if not self.current_order_id:
            return
        
        self.is_brewing = True
        GPIO.output(GPIO_PINS['LED'], GPIO.HIGH)
        
        logging.info(f"Starting brewing process for order: {self.current_order_id}")
        
        try:
            # Step 1: Preparing cup
            self.update_status("preparing", "preparing_cup", "กำลังเตรียมแก้ว")
            time.sleep(3)
            
            # Step 2: Adding toppings
            self.update_status("brewing", "adding_toppings", "ใส่ท็อปปิ้ง")
            GPIO.output(GPIO_PINS['VALVE'], GPIO.HIGH)
            time.sleep(4)
            GPIO.output(GPIO_PINS['VALVE'], GPIO.LOW)
            
            # Step 3: Adding ice
            self.update_status("brewing", "adding_ice", "ใส่น้ำแข็ง")
            time.sleep(3.5)
            
            # Step 4: Brewing drink
            self.update_status("brewing", "brewing_drink", "ใส่เครื่องดื่ม")
            GPIO.output(GPIO_PINS['PUMP'], GPIO.HIGH)
            time.sleep(5)
            GPIO.output(GPIO_PINS['PUMP'], GPIO.LOW)
            
            # Step 5: Completed
            self.update_status("completed", "completed", "เสร็จสิ้น")
            
            logging.info(f"Brewing completed for order: {self.current_order_id}")
            
        except Exception as e:
            logging.error(f"Error during brewing: {e}")
            self.update_status("completed", "completed", f"ข้อผิดพลาด: {str(e)}", error=True)
        
        finally:
            # Reset state
            self.current_order_id = None
            self.is_brewing = False
            GPIO.output(GPIO_PINS['LED'], GPIO.LOW)
            
            # Blink LED to indicate completion
            for _ in range(5):
                GPIO.output(GPIO_PINS['LED'], GPIO.HIGH)
                time.sleep(0.2)
                GPIO.output(GPIO_PINS['LED'], GPIO.LOW)
                time.sleep(0.2)
    
    def update_status(self, status, step, message, error=False):
        """Update order status via API"""
        data = {
            'orderId': self.current_order_id,
            'status': status,
            'step': step,
            'message': message,
            'hardwareId': HARDWARE_ID,
            'error': error
        }
        
        response = self.make_api_request('POST', '/hardware/status', data)
        
        if response and response.get('success'):
            logging.info(f"Status updated: {message}")
        else:
            logging.warning(f"Failed to update status: {message}")
    
    def send_heartbeat(self):
        """Send heartbeat to maintain connection"""
        logging.debug("Sending heartbeat...")
        
        response = self.make_api_request('GET', f'/hardware/status?hardwareId={HARDWARE_ID}')
        
        if response and response.get('success'):
            logging.debug("Heartbeat sent successfully")
        else:
            logging.warning("Heartbeat failed")
    
    def run(self):
        """Main execution loop"""
        logging.info("Starting TotoBin Hardware service...")
        
        try:
            while True:
                current_time = time.time()
                
                # Send heartbeat
                if current_time - self.last_heartbeat >= HEARTBEAT_INTERVAL:
                    self.send_heartbeat()
                    self.last_heartbeat = current_time
                
                # Poll for orders if not brewing
                if not self.is_brewing and current_time - self.last_poll_time >= POLL_INTERVAL:
                    self.poll_for_orders()
                    self.last_poll_time = current_time
                
                time.sleep(0.1)
                
        except KeyboardInterrupt:
            logging.info("Shutting down...")
        except Exception as e:
            logging.error(f"Unexpected error: {e}")
        finally:
            self.cleanup_gpio()

if __name__ == "__main__":
    hardware = TotoBinHardware()
    hardware.run()