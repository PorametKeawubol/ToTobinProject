#include <Arduino.h>
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

AudioGeneratorWAV* wav = nullptr;
AudioFileSourceLittleFS* file = nullptr;
AudioOutputI2S* out = nullptr;
bool finished = false;

// แปลงเปอร์เซ็นต์ (0-100) -> gain (0.0-1.0)
void setVolumePercent(uint8_t percent) {
  if (!out) return;
  percent = constrain(percent, 0, 30);   // จำกัดให้เบาๆ
  float gain = percent / 100.0f;         // เช่น 5 => 0.05
  out->SetGain(gain);
}

void setup() {
  LittleFS.begin(true);

  file = new AudioFileSourceLittleFS("/precup.wav");    // PCM_U8 mono 8k/16k
  out  = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  out->SetPinout(25, -1, -1);                           // ใช้ DAC1 (GPIO25)
  // out->SetRate(16000);                                // เซ็ตถ้าจำเป็น
  setVolumePercent(5);                                  // เริ่มเบาๆ

  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
}

void loop() {
  if (finished) return;

  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      // ===== เล่นจบ: ปิดเสียงและคืนทรัพยากร =====
      wav->stop();
      if (out) out->SetGain(0.0f);   // ปิดปาก DAC เพื่อกันฮิส
      if (file) file->close();

      // ปล่อย I2S/DAC และปิดไฟล์ให้เกลี้ยง
      delete wav;  wav = nullptr;
      delete file; file = nullptr;

      // บางบอร์ดจำเป็นต้องหยุด I2S ด้วย (ถ้าไลบรารีรองรับ)
      // if (out) out->stop();  // ใช้ได้ในบางเวอร์ชัน
      delete out;  out = nullptr;

      LittleFS.end();
      finished = true;
    }
  }
}
