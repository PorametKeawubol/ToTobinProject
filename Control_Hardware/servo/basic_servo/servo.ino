#include <Servo.h>
Servo myservo;

int pos = 0;       // ตำแหน่งปัจจุบัน
int step = 2;      // ก้าวทีละ 1 องศา (+ ไปข้างหน้า, - ย้อนกลับ)

void setup() {
  myservo.attach(8); 
  myservo.attach(10); 
}

void loop() {
  myservo.write(pos);  // สั่งให้ servo ไปที่ pos
  delay(15);           // หน่วงเล็กน้อยให้ servo ขยับทัน

  pos += step;         // เพิ่มหรือลดตำแหน่ง

  // ถ้าถึงขอบ 0 หรือ 180 ให้กลับทิศ
  if (pos >= 180 || pos <= 0) {
    step = -step;
  }
}
