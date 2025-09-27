#include <Servo.h>

Servo servo;
volatile bool pressed = false;

int mode = 1;                          // เริ่มที่ Mode 1
int angle = 90;                        // เริ่มที่ 90°
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
  servo.write(angle);      // เริ่มหมุนไปที่ 90° ทันที

  pinMode(3, INPUT_PULLUP);    
  attachInterrupt(digitalPinToInterrupt(3), isr_falling, FALLING);
}

void loop() {
  if (pressed) {
    pressed = false;

    unsigned long now = millis();
    if (now - lastPressMs > DEBOUNCE_MS) {
      if (mode == 1) {
        // ----- Mode 1 → หมุนไป 0° -----
        angle = 30;
        servo.write(angle);

        delay(300);  // รอ 0.5 วิ

        // ----- กลับไป Mode 1 (90°) -----
        angle = 90;
        servo.write(angle);

        // กลับไป mode 1 เสมอ
        mode = 1;
      }
      lastPressMs = now;
    }
  }
}
