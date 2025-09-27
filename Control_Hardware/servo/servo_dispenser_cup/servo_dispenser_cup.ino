#include <Servo.h>

Servo servo1;   // D8
Servo servo2;   // D9

int pos = 0;                          
const uint8_t stepSize = 10;          
const unsigned long stepDelayMs = 20; 

const uint8_t BTN_INT_PIN = 3;
volatile bool pressed = false;        
unsigned long lastPressMs = 0;        
const unsigned long DEBOUNCE_MS = 200;

bool comboRunning = false;            
int target1 = 180;                    
int target2 = 180;                      
int currentTarget = -1;               

// ---------- ISR ----------
void isrToggle() {
  pressed = true;
}

void setup() {
  servo1.attach(9);
  servo2.attach(10);

  pos = 0;
  servo1.write(pos);           
  servo2.write(pos);     

  pinMode(BTN_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_INT_PIN), isrToggle, FALLING);
}

void loop() {
  if (pressed) {
    unsigned long now = millis();
    if (now - lastPressMs >= DEBOUNCE_MS) {
      if (!comboRunning) {
        comboRunning = true;
        if (pos < 90) {
          target1 = 180;
          target2 = 0;
        } else {
          target1 = 0;
          target2 = 180;
        }
        currentTarget = target1;
      }
      lastPressMs = now;
    }
    pressed = false;
  }

  if (comboRunning) {
    if (pos < currentTarget) {
      pos += stepSize;
      if (pos > currentTarget) pos = currentTarget;
    } else if (pos > currentTarget) {
      pos -= stepSize;
      if (pos < currentTarget) pos = currentTarget;
    }

    servo1.write(pos);          
    servo2.write(pos);    
    delay(stepDelayMs);

    if (pos == currentTarget) {
      if (currentTarget == target1) {
        currentTarget = target2;
      } else {
        comboRunning = false;
        currentTarget = -1;
      }
    }
  } else {
    delay(5);
  }
}
