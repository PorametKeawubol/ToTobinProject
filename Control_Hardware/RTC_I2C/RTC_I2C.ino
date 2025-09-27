#include <Wire.h>
#include <DS1307RTC.h>
#include <TimeLib.h>
#include <string.h>
#include <stdio.h>

int monthStrToNumber(const char *m) {
  const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  for (int i = 0; i < 12; i++) {
    if (strncmp(m, months + i*3, 3) == 0) return i + 1;
  }
  return 0;
}

void setRTCToCompileTime() {
  tmElements_t tm;
  char m[4]; int d, y, hh, mm, ss;

  // __DATE__ ตัวอย่าง "Sep 13 2025", __TIME__ ตัวอย่าง "18:42:05"
  sscanf(__DATE__, "%3s %d %d", m, &d, &y);
  sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);

  tm.Day    = d;
  tm.Month  = monthStrToNumber(m);
  tm.Year   = CalendarYrToTm(y);  // แปลงปีค.ศ.ไปเป็นฟอร์แมตของ TimeLib
  tm.Hour   = hh;
  tm.Minute = mm;
  tm.Second = ss;

  if (RTC.write(tm)) {
    Serial.println(F("RTC updated to compile time and started."));
  } else {
    Serial.println(F("RTC write failed. Check wiring/battery."));
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println(F("Setting DS1307 to your computer's current time..."));
  setRTCToCompileTime();
  Serial.println(F("Done. Now re-upload your ReadTest sketch to view time."));
}

void loop() {}


// #include <Wire.h>
// #include <DS1307RTC.h>
// #include <TimeLib.h>

// void setup() {
//   Serial.begin(9600);
//   while (!Serial)
//     ;

//   // ดึงเวลา PC แล้วตั้งให้ DS1307
//   setSyncProvider(RTC.get);
//   if (timeStatus() != timeSet) {
//     Serial.println("Unable to sync with the RTC");
//   } else {
//     Serial.println("RTC has set the system time");
//   }
// }

// void loop() {
//   // แสดงเวลาออกมา
//   Serial.print(hour());
//   Serial.print(':');
//   Serial.print(minute());
//   Serial.print(':');
//   Serial.print(second());
//   Serial.print(" ");
//   Serial.print(day());
//   Serial.print('/');
//   Serial.print(month());
//   Serial.print('/');
//   Serial.println(year());
//   delay(1000);
// }
