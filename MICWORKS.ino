#include <WiFi.h>
#include <HTTPClient.h>

// --- USER CONFIGURATION ---
const char* WIFI_SSID = "Jana ayman";
const char* WIFI_PASSWORD = "hanobano2";
// Ensure this IP matches your computer's current IP (run ipconfig)
const char* SERVER_URL = "http://172.20.10.6:5000/upload"; 

// --- HARDWARE CONFIGURATION ---
// We moved this to GPIO 1 to fix the silence issue
const int MIC_PIN = 1;     
const int LED_PIN = 10;
const int BUTTON_PIN = 0;  // BOOT Button (Built-in)

// --- AUDIO SETTINGS ---
const int SAMPLE_RATE = 16000; 
const int RECORD_TIME_SECONDS = 5; 
const int TOTAL_SAMPLES = SAMPLE_RATE * RECORD_TIME_SECONDS;

// Calibrated Zero Level for MAX9814 (1.25V Bias on 3.3V ADC)
int zeroLevel = 1550; 
int16_t *audioBuffer = NULL; 

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  
  // --- FIX 1: VOLTAGE RANGE ---
  analogSetAttenuation(ADC_11db);
  
  // --- FIX 2: HARDWARE TEST (Diagnostic) ---
  Serial.println("\n--- HARDWARE TEST (PIN 1) ---");
  Serial.println("Reading Mic on GPIO 1... (Please make noise!)");
  long minVal = 4096;
  long maxVal = 0;
  
  // Read for 1 second to find volume swing
  unsigned long testStart = millis();
  while(millis() - testStart < 1000) {
    int val = analogRead(MIC_PIN);
    if(val < minVal) minVal = val;
    if(val > maxVal) maxVal = val;
    delayMicroseconds(100);
  }
  
  int range = maxVal - minVal;
  Serial.print("Mic Range: "); Serial.print(minVal); Serial.print(" -> "); Serial.println(maxVal);
  Serial.print("Volume Swing: "); Serial.println(range);
  
  // Threshold set to 50. If the mic is working, noise alone usually gives >50.
  if (range < 50) {
    Serial.println("[CRITICAL FAILURE] Microphone is sending SILENCE.");
    Serial.println("   -> The wire is likely loose or the Mic is broken.");
    // Blink fast to warn user of hardware fail
    while(1) { digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); }
  } else {
    Serial.println("[HARDWARE OK] Microphone is active.");
  }

  // --- WIFI ---
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Memory Allocation
  audioBuffer = (int16_t*) malloc(TOTAL_SAMPLES * sizeof(int16_t));
  if (audioBuffer == NULL) {
    Serial.println("Error: Out of Memory");
    while(1) delay(1000);
  }

  // Visual Ready Indicator
  digitalWrite(LED_PIN, LOW);
  Serial.println("--- READY: PRESS BOOT BUTTON TO RECORD ---");
}

void loop() {
  // Check Button (Active LOW)
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed -> Recording...");
      recordAudio();
      sendAudio();
      Serial.println("--- READY ---");
    }
  }
}

void recordAudio() {
  digitalWrite(LED_PIN, HIGH); // LED On = Recording
  
  unsigned long startTime = micros();
  const int samplingInterval = 1000000 / SAMPLE_RATE; 

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    unsigned long nextSampleTime = startTime + (i * samplingInterval);
    
    // Strict timing loop
    while (micros() < nextSampleTime); 
    
    int raw = analogRead(MIC_PIN);
    
    // Remove DC Bias (Centering the wave)
    int shifted = raw - zeroLevel; 
    
    // Digital Gain: Multiplies volume by 4. 
    shifted *= 4; 
    
    // Hard Limit (Clip) to 16-bit range
    if (shifted > 32767) shifted = 32767;
    if (shifted < -32768) shifted = -32768;

    audioBuffer[i] = (int16_t)shifted;
  }
  
  digitalWrite(LED_PIN, LOW); // LED Off = Done
}

void sendAudio() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Uploading to server...");
    
    // Create WiFiClient for connection
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, SERVER_URL);
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpResponseCode = http.POST((uint8_t *)audioBuffer, TOTAL_SAMPLES * sizeof(int16_t));
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("RESPONSE: ");
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Error: WiFi Disconnected");
  }
}