#include <Arduino.h>
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
AudioGeneratorWAV* wav;
AudioFileSourceLittleFS* file;
AudioOutputI2S* out;
void setup() {
  LittleFS.begin(true);
  file = new AudioFileSourceLittleFS("/precup.wav");  // pcm_u8, mono, 8k/16kHz // ใช้ DAC ภายใน ESP32 → GPIO25 เท่านั้น
  out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  out->SetPinout(25, -1, -1);  // เปิดแค่ GPIO25
  out->SetGain(0.1f);          // ลดความดัง
  // out->SetRate(16000);         // ให้ตรงกับไฟล์ (8k หรือ 16k)
  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
}
void loop() {
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
    }
  }
}