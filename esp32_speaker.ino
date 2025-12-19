#include "SPIFFS.h"

const int DAC_L = 25;   // GPIO25 -> PAM L
const int DAC_R = 26;   // GPIO26 -> PAM R

uint32_t rd32(File &f){
  uint8_t b[4]; f.read(b,4);
  return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}
uint16_t rd16(File &f){
  uint8_t b[2]; f.read(b,2);
  return (uint16_t)b[0] | ((uint16_t)b[1]<<8);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed");
    while (1) delay(1000);
  }
  Serial.println("SPIFFS OK");

  File f = SPIFFS.open("/welcome.wav", "r");
  if (!f) {
    Serial.println("File not found: /welcome.wav");
    while (1) delay(1000);
  }

  char riff[4]; f.readBytes(riff, 4);
  if (memcmp(riff, "RIFF", 4) != 0) { Serial.println("Not RIFF"); while(1){} }
  rd32(f);
  char wave[4]; f.readBytes(wave, 4);
  if (memcmp(wave, "WAVE", 4) != 0) { Serial.println("Not WAVE"); while(1){} }

  uint16_t audioFormat=0, numCh=0, bits=0;
  uint32_t sampleRate=0, dataSize=0;

  while (f.available()) {
    char id[4]; f.readBytes(id, 4);
    uint32_t sz = rd32(f);

    if (memcmp(id, "fmt ", 4) == 0) {
      audioFormat = rd16(f);
      numCh = rd16(f);
      sampleRate = rd32(f);
      rd32(f);
      rd16(f);
      bits = rd16(f);
      if (sz > 16) f.seek(f.position() + (sz - 16));
    } else if (memcmp(id, "data", 4) == 0) {
      dataSize = sz;
      break;
    } else {
      f.seek(f.position() + sz);
    }
  }

  Serial.printf("fmt=%u ch=%u sr=%lu bits=%u data=%lu\n",
                audioFormat, numCh, (unsigned long)sampleRate, bits, (unsigned long)dataSize);

  if (audioFormat != 1 || numCh != 1 || bits != 8 || sampleRate == 0) {
    Serial.println("Convert WAV to PCM 8-bit mono (16k/22k Hz).");
    while (1) delay(1000);
  }

  Serial.println("Playing...");

  const uint32_t usPerSample = 1000000UL / sampleRate;
  uint8_t buf[512];
  uint32_t played = 0;
  uint32_t nextMicros = micros();

  while (played < dataSize && f.available()) {
    int toRead = (dataSize - played) > sizeof(buf) ? sizeof(buf) : (dataSize - played);
    int n = f.read(buf, toRead);
    if (n <= 0) break;

    for (int i = 0; i < n; i++) {
      while ((int32_t)(micros() - nextMicros) < 0) {}
      nextMicros += usPerSample;
      dacWrite(DAC_L, buf[i]);
      dacWrite(DAC_R, buf[i]);
    }
    played += n;
  }

  Serial.println("Done.");
  f.close();
}

void loop() {}
