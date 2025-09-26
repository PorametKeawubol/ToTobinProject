int pirPin = 8;
int ledPin = 2;

unsigned long lastMotionTime = 0;   // เวลาที่ตรวจจับเจอครั้งล่าสุด
int motionTimeout = 10000;          // 10 วินาที

void setup() {
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int val = digitalRead(pirPin);

  if (val == HIGH) {
    Serial.println("Motion detected!");
    lastMotionTime = millis();  // บันทึกเวลา
  }

  // ถ้ายังไม่เกิน timeout → ถือว่ายังมีคน
  if (millis() - lastMotionTime < motionTimeout) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
}
