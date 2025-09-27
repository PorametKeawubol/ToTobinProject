#include <Ultrasonic.h>

Ultrasonic ultrasonic(9, 8);  // (Trig, Echo)

void setup() {
  Serial.begin(9600);
}

void loop() {
  long distance = ultrasonic.read(); // หน่วย cm
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(100);
}


// #include <NewPing.h>

// #define TRIG_PIN 9
// #define ECHO_PIN 8
// #define MAX_DISTANCE 1000  // หน่วยเป็น cm (กำหนดระยะสูงสุดที่ต้องการวัด)

// NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// void setup() {
//   Serial.begin(9600);
// }

// void loop() {
//   delay(50);  // หน่วงเล็กน้อย
//   unsigned int distance = sonar.ping_cm();
//   Serial.print("Distance: ");
//   Serial.print(distance);
//   Serial.println(" cm");
// }
