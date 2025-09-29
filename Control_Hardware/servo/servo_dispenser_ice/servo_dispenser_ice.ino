#include <Servo.h>

Servo servo;
volatile bool pressed = false;

int angle = 10;                          // เริ่มที่ 0°
unsigned long lastPressMs = 0;         
const unsigned long DEBOUNCE_MS = 50;

// ---------- ISR ----------
void isr_falling() {
  if (digitalRead(3) == LOW) {
    pressed = true;
  }
}

void setup() {
  servo.attach(11);        
  servo.write(angle);      // เริ่มต้นที่ 0°

  pinMode(3, INPUT_PULLUP);    
  attachInterrupt(digitalPinToInterrupt(3), isr_falling, FALLING);
}

void loop() {
  if (pressed) {
    pressed = false;

    unsigned long now = millis();
    if (now - lastPressMs > DEBOUNCE_MS) {
      // ----- เมื่อกดปุ่ม → หมุนไปที่ 55° -----
      angle = 55;
      servo.write(angle);

      delay(300);
      angle = 10; 
      servo.write(angle);

      lastPressMs = now;
    }
  }
}
