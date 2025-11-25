#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Audio.h" // Install "ESP32-audioI2S" by Schreibfaul1

// --- USER CONFIGURATION ---
const char* WIFI_SSID = "Mayousha";
const char* WIFI_PASSWORD = "mayoosh123";
// Ensure this IP matches your computer's current IP (run ipconfig)
const char* SERVER_BASE_URL = "http://172.20.10.6:5000"; 

// --- PINS ---
const int MIC_PIN = 1;      // Mic OUT (Check your wiring!)
const int BUTTON_PIN = 0;   // BOOT Button
const int LED_PIN = 10;

// Speaker Pins (I2S Interface)
// We only use DOUT for the speaker signal. BCLK and LRC are required by the library logic.
#define I2S_DOUT      18    // Connect this to PAM8403 Input (via Resistor)
#define I2S_BCLK      17    // Not connected
#define I2S_LRC       16    // Not connected

// --- AUDIO SETTINGS ---
const int SAMPLE_RATE = 16000;
const int RECORD_TIME_SECONDS = 5;
// MAX9814 Bias (1.25V) is roughly 1550 on ADC
int zeroLevel = 1550; 

Audio audio;
int16_t *micBuffer = NULL;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 1. Setup Microphone Voltage Range
  analogSetAttenuation(ADC_11db);

  // 2. Setup WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  Serial.println("\nWiFi Connected");

  // 3. Setup Speaker (Audio Player)
  // We use the default I2S settings. The library handles the MP3 decoding.
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21); // Volume 0-21

  // 4. Allocate Memory for Recording
  // 16000 samples/sec * 5 sec * 2 bytes = 160,000 bytes
  micBuffer = (int16_t*) malloc(SAMPLE_RATE * RECORD_TIME_SECONDS * sizeof(int16_t));
  if(micBuffer == NULL) {
    Serial.println("ERROR: Not enough RAM for audio buffer!");
    while(1) delay(100);
  }

  digitalWrite(LED_PIN, LOW);
  Serial.println("--- READY: Press BOOT Button to Speak ---");
}

void loop() {
  // This keeps the audio player running smoothly
  audio.loop();

  // Check Button
  if (digitalRead(BUTTON_PIN) == LOW) {
    // If audio is playing, stop it first
    if(audio.isRunning()) {
      audio.stopSong();
    }

    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed -> Recording...");
      recordAndSend();
      Serial.println("--- READY ---");
    }
  }
}

void recordAndSend() {
  digitalWrite(LED_PIN, HIGH); // LED ON = Recording
  
  // --- 1. RECORDING ---
  unsigned long startTime = micros();
  const int interval = 1000000 / SAMPLE_RATE;
  int totalSamples = SAMPLE_RATE * RECORD_TIME_SECONDS;
  
  for (int i = 0; i < totalSamples; i++) {
    unsigned long next = startTime + (i * interval);
    while (micros() < next); // Wait for exact timing
    
    int raw = analogRead(MIC_PIN);
    
    // Process Audio (Remove DC Bias & Amplify)
    int shifted = (raw - zeroLevel) * 4; 
    
    // Clip
    if (shifted > 32767) shifted = 32767;
    if (shifted < -32768) shifted = -32768;
    
    micBuffer[i] = (int16_t)shifted;
  }
  
  digitalWrite(LED_PIN, LOW); // LED OFF = Finished Recording
  
  // --- 2. UPLOADING ---
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Uploading to server...");
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/upload";
    
    http.begin(url);
    http.addHeader("Content-Type", "application/octet-stream");
    
    // Send the recording
    int httpCode = http.POST((uint8_t *)micBuffer, totalSamples * sizeof(int16_t));
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.print("Server says: "); Serial.println(response);
      
      // If the response is a URL (starts with http), Play it!
      if (response.startsWith("http")) {
        Serial.println("Playing Response...");
        audio.connecttohost(response.c_str());
      }
    } else {
      Serial.print("Upload Error: "); Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// Optional: Debugging info from Audio Library
void audio_info(const char *info) {
    Serial.print("audio_info: "); Serial.println(info);
}