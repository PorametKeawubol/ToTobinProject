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
bool isHolding = false;   // <-- ตัวแปรใหม่: ตรวจว่าปุ่มถูกกดค้าง

int target1 = 180;
int target2 = 0;
int currentTarget = -1;

// ---------- ค่าการเขย่า ----------
int shakeAmplitude = 80;   // แกว่งรวม 80° (±40°)
int shakeDelay    = 150;   // หน่วงต่อการแกว่ง (ms)
int shakeTimes    = 5;     // จำนวนครั้งต่อรอบ (ทำซ้ำถ้ากดค้าง)

// ---------- ISR ----------
void isrToggle() {
  pressed = true;
}

// ---------- ฟังก์ชันเขย่า ----------
void shakeServos() {
  int halfAmp = shakeAmplitude / 2;

  // เขย่าหลายครั้ง (สั้น ๆ) ต่อรอบ
  for (int i = 0; i < shakeTimes; i++) {
    servo1.write(pos - halfAmp);
    servo2.write(pos - halfAmp);
    delay(shakeDelay);

    servo1.write(pos + halfAmp);
    servo2.write(pos + halfAmp);
    delay(shakeDelay);
  }

  // กลับมาตำแหน่งเดิม
  servo1.write(pos);
  servo2.write(pos);
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
  // ตรวจสถานะปุ่มกด
  if (digitalRead(BTN_INT_PIN) == LOW) {
    isHolding = true;   // กำลังกดค้าง
  } else {
    if (isHolding) {
      // เพิ่งปล่อยปุ่ม → หมุนกลับ 0°
      while (pos > 0) {
        pos -= stepSize;
        if (pos < 0) pos = 0;
        servo1.write(pos);
        servo2.write(pos);
        delay(stepDelayMs);
      }
    }
    isHolding = false;
  }

  // ถ้ากดค้าง → เขย่า
  if (isHolding) {
    // หมุนไปถึง 180 ก่อน
    if (pos < 180) {
      pos += stepSize;
      if (pos > 180) pos = 180;
      servo1.write(pos);
      servo2.write(pos);
      delay(stepDelayMs);
    } else {
      // เมื่อถึง 180 → เขย่าเรื่อย ๆ
      shakeServos();
    }
  }
}
