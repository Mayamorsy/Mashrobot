
#include <WiFi.h>
#include <HTTPClient.h>
#include "SPIFFS.h"

// ---------------- USER CONFIG ----------------
const char* WIFI_SSID     = "Mayousha";
const char* WIFI_PASSWORD = "mayoosh123";
const char* SERVER_UPLOAD = "http://172.20.10.6:5000/upload";
const char* SERVER_RESET  = "http://172.20.10.6:5000/reset";

// ---------------- PINS ----------------
const int MIC_PIN    = 36;   // MAX9814 OUT
const int LED_PIN    = 2;    // onboard LED
const int BUTTON_PIN = 0;    // BOOT button

// DAC pins -> PAM8403 inputs
const int DAC_L = 25;        // PAM L-IN

// ---------------- AUDIO RECORD SETTINGS ----------------
const int SAMPLE_RATE = 16000;
const int RECORD_TIME_SECONDS = 3;
const int TOTAL_SAMPLES = SAMPLE_RATE * RECORD_TIME_SECONDS;

// Your “zeroLevel” baseline (works, but we also compute a quick baseline each record)
int zeroLevel = 1800;
int16_t *audioBuffer = nullptr;

// Optional: prevent playing the same action twice in a row
bool SUPPRESS_SAME_ACTION = false;
String lastAction = "";

// ---------- WAV helpers (SPIFFS player) ----------
uint32_t rd32(File &f){
  uint8_t b[4]; f.read(b,4);
  return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}
uint16_t rd16(File &f){
  uint8_t b[2]; f.read(b,2);
  return (uint16_t)b[0] | ((uint16_t)b[1]<<8);
}

bool playWavFromSPIFFS(const char* path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    Serial.print("File not found: ");
    Serial.println(path);
    return false;
  }

  char riff[4]; f.readBytes(riff, 4);
  if (memcmp(riff, "RIFF", 4) != 0) { Serial.println("Not RIFF"); f.close(); return false; }
  rd32(f);
  char wave[4]; f.readBytes(wave, 4);
  if (memcmp(wave, "WAVE", 4) != 0) { Serial.println("Not WAVE"); f.close(); return false; }

  uint16_t audioFormat=0, numCh=0, bits=0;
  uint32_t sampleRate=0, dataSize=0;

  while (f.available()) {
    char id[4]; f.readBytes(id, 4);
    uint32_t sz = rd32(f);

    if (memcmp(id, "fmt ", 4) == 0) {
      audioFormat = rd16(f);
      numCh = rd16(f);
      sampleRate = rd32(f);
      rd32(f);   // byteRate
      rd16(f);   // blockAlign
      bits = rd16(f);
      if (sz > 16) f.seek(f.position() + (sz - 16));
    } else if (memcmp(id, "data", 4) == 0) {
      dataSize = sz;
      break;
    } else {
      f.seek(f.position() + sz);
    }
  }

  Serial.printf("Playing %s | fmt=%u ch=%u sr=%lu bits=%u data=%lu\n",
                path, audioFormat, numCh, (unsigned long)sampleRate, bits, (unsigned long)dataSize);

  // Your player expects PCM, mono, 8-bit
  if (audioFormat != 1 || numCh != 1 || bits != 8 || sampleRate == 0 || dataSize == 0) {
    Serial.println("WAV must be PCM, mono, 8-bit.");
    f.close();
    return false;
  }

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

      // buf[i] is 0..255 => perfect for dacWrite
      dacWrite(DAC_L, buf[i]);
    }
    played += n;
  }

  f.close();
  Serial.println("Done playing.");
  return true;
}

// ---------- Networking ----------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  digitalWrite(LED_PIN, LOW);
  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Call /reset once at boot so server starts in known state and returns PLAY:WELCOME
String callReset() {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  WiFiClient client;
  http.begin(client, SERVER_RESET);
  int code = http.GET();
  String resp = (code > 0) ? http.getString() : "";
  http.end();

  Serial.print("RESET resp: ");
  Serial.println(resp);
  return resp;
}

// Upload raw PCM16 buffer to /upload
String uploadAudio() {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  WiFiClient client;

  http.begin(client, SERVER_UPLOAD);
  http.setReuse(false);
  http.addHeader("Content-Type", "application/octet-stream");

  int bytesLen = TOTAL_SAMPLES * sizeof(int16_t);
  int code = http.POST((uint8_t*)audioBuffer, bytesLen);

  String resp = (code > 0) ? http.getString() : "";
  Serial.print("UPLOAD HTTP code: ");
  Serial.println(code);
  Serial.print("UPLOAD resp: ");
  Serial.println(resp);

  http.end();
  return resp;
}

// ---------- Parse server response ----------
bool parseAction(const String& resp, String &heardOut, String &actionOut) {
  // Expected: HEARD:<text>|PLAY:<ACTION>
  int sep = resp.indexOf('|');
  if (sep < 0) return false;

  String left = resp.substring(0, sep);
  String right = resp.substring(sep + 1);

  if (left.startsWith("HEARD:")) heardOut = left.substring(6);
  else heardOut = left;

  actionOut = right;  // e.g. PLAY:WELCOME
  actionOut.trim();
  return true;
}

const char* actionToWav(const String& action) {
  // action example: "PLAY:WELCOME"
  if (action == "PLAY:WELCOME")       return "/welcome.wav";
  if (action == "PLAY:SUGARQUESTION") return "/sugarquestion.wav";
  if (action == "PLAY:STARTORDER")    return "/startorder.wav";
  return nullptr;
}

// ---------- Recording ----------
void recordAudio() {
  // ADC setup (your working setup)
  analogSetAttenuation(ADC_11db);
  analogReadResolution(12);

  // Quick baseline each time (more reliable than a fixed zeroLevel)
  long sum = 0;
  const int N = 200;
  for (int i = 0; i < N; i++) {
    sum += analogRead(MIC_PIN);
    delay(2);
  }
  int baseline = sum / N;
  zeroLevel = baseline; // keep it updated

  Serial.print("Baseline: ");
  Serial.println(zeroLevel);

  digitalWrite(LED_PIN, HIGH);

  unsigned long startTime = micros();
  const int samplingInterval = 1000000 / SAMPLE_RATE;

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    unsigned long nextSampleTime = startTime + (i * samplingInterval);
    while (micros() < nextSampleTime) {}

    int raw = analogRead(MIC_PIN);
    int shifted = raw - zeroLevel;

    // Your gain
    shifted *= 4;

    // Clip
    if (shifted > 32767) shifted = 32767;
    if (shifted < -32768) shifted = -32768;

    audioBuffer[i] = (int16_t)shifted;
  }

  digitalWrite(LED_PIN, LOW);
}

// ---------- Main flow ----------
void handleServerResponse(const String& resp) {
  
  Serial.print("RAW SERVER DATA: "); // <--- ADD THIS
  Serial.println(resp);

  String heard, action;
  if (!parseAction(resp, heard, action)) {
    Serial.println("Bad response format (no '|').");
    return;
  }

  Serial.print("Heard: ");
  Serial.println(heard);
  Serial.print("Action: ");
  Serial.println(action);

  if (SUPPRESS_SAME_ACTION && action == lastAction) {
    Serial.println("Same action as last time -> suppressed.");
    return;
  }

  const char* wavPath = actionToWav(action);
  if (!wavPath) {
    Serial.println("Unknown action, no WAV mapped.");
    lastAction = action; // still update to avoid spam
    return;
  }

  playWavFromSPIFFS(wavPath);
  lastAction = action;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  // SPIFFS
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed");
    while (1) delay(1000);
  }
  Serial.println("SPIFFS OK");

  // Allocate audio buffer
  audioBuffer = (int16_t*) malloc(TOTAL_SAMPLES * sizeof(int16_t));
  if (!audioBuffer) {
    Serial.println("Out of memory for audioBuffer!");
    while (1) delay(1000);
  }

  // WiFi
  connectWiFi();

  // Reset server session and play whatever it returns (usually welcome)
  String resetResp = callReset();
  if (resetResp.length()) {
    handleServerResponse(resetResp);
  }

  Serial.println("--- READY: PRESS BOOT BUTTON TO RECORD ---");
}

void loop() {
  // Button press
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(60); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Recording...");
      recordAudio();

      delay(200);

      Serial.println("Uploading...");
      String resp = uploadAudio();
      if (resp.length()) handleServerResponse(resp);

      Serial.println("--- READY ---");

      // Wait for release so it doesn't trigger multiple times
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }

  delay(10);
}
