#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Audio.h" // Install "ESP32-audioI2S" by Schreibfaul1

// --- USER CONFIGURATION ---
const char* WIFI_SSID = "WE_1C368E";
const char* WIFI_PASSWORD = "lbp08202";
// Update this with your Laptop's IP Address!
const char* SERVER_BASE_URL = "http://192.168.1.25:5000"; 

// --- PIN CONFIGURATION ---
const int MIC_PIN = 1;      // Mic OUT (GPIO 1)
const int SPEAKER_PIN = 17;  // Crowtail Yellow Wire (GPIO 4)
const int BUTTON_PIN = 0;   // BOOT Button (GPIO 0)
const int LED_PIN = 10;

// Communication with Arduino Uno (Pin 13 -> Uno Pin 2)
#define ARDUINO_TX_PIN 13 

// Audio I2S Pins (Required by library)
#define I2S_DOUT      SPEAKER_PIN
#define I2S_BCLK      17    // Not connected, but required by config
#define I2S_LRC       16    // Not connected, but required by config

// --- SETTINGS ---
const int SAMPLE_RATE = 16000;
const int RECORD_TIME_SECONDS = 3; 
// Mic Calibration (1.25V Bias on 3.3V ADC approx 1550)
int zeroLevel = 1550; 

Audio audio;
int16_t *micBuffer = NULL;
HardwareSerial ArduinoSerial(2); // UART2 for communicating with Arduino

void setup() {
  Serial.begin(115200);
  
  // 1. Setup Arduino Communication
  ArduinoSerial.begin(9600, SERIAL_8N1, 14, ARDUINO_TX_PIN); 

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 2. Setup Mic
  // This is critical for ESP32 S3 ADC to work with the Mic
  analogSetAttenuation(ADC_11db);

  // 3. Setup WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  Serial.println("\nWiFi Connected");
  digitalWrite(LED_PIN, LOW);

  // 4. Setup Speaker
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(14);   // Lower volume = Less Static noise
  audio.forceMono(true); // Combine channels for better clarity

  // 5. Allocate Memory
  micBuffer = (int16_t*) malloc(SAMPLE_RATE * RECORD_TIME_SECONDS * sizeof(int16_t));
  if(micBuffer == NULL) {
    Serial.println("CRITICAL ERROR: Not enough RAM for audio buffer!");
    while(1) delay(100);
  }

  // 6. Say Hello
  getWelcomeMessage();
}

void loop() {
  // Keep the audio engine running
  audio.loop();

  // Check Button
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Stop playing audio immediately if user wants to speak
    if(audio.isRunning()) {
      audio.stopSong();
    }
    
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed -> Listening...");
      recordAndSend();
    }
  }
}

// --- NETWORK & AUDIO FUNCTIONS ---

void handleServerResponse(String response) {
  // Format 1: Command -> "CMD:tea:2:http://..."
  // Format 2: Audio Only -> "http://..."
  
  if (response.startsWith("CMD:")) {
    // It's a command!
    int lastColon = response.lastIndexOf(':');
    if (lastColon != -1) {
      String commandPart = response.substring(4, lastColon); // "tea:2"
      String audioURL = response.substring(lastColon + 1);   // "http://..."
      
      Serial.print("Sending Command to Arduino: "); Serial.println(commandPart);
      ArduinoSerial.println(commandPart); 
      
      // Play the confirmation audio
      audio.connecttohost(audioURL.c_str());
    }
  } else if (response.startsWith("http")) {
    // Just a conversation reply
    Serial.println("Playing Audio Response...");
    audio.connecttohost(response.c_str());
  }
}

void getWelcomeMessage() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/reset"; 
    
    // Using correct syntax for modern ESP32 boards
    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String response = http.getString();
      handleServerResponse(response);
    } else {
      Serial.print("Welcome Error: "); Serial.println(httpCode);
    }
    http.end();
  }
}

void recordAndSend() {
  digitalWrite(LED_PIN, HIGH); // LED ON
  
  unsigned long startTime = micros();
  const int interval = 1000000 / SAMPLE_RATE;
  int totalSamples = SAMPLE_RATE * RECORD_TIME_SECONDS;
  
  // Record Loop
  for (int i = 0; i < totalSamples; i++) {
    unsigned long next = startTime + (i * interval);
    while (micros() < next); // Wait for exact timing
    
    int raw = analogRead(MIC_PIN);
    
    // Process Audio (Center & Amplify)
    int shifted = (raw - zeroLevel) * 4; 
    if (shifted > 32767) shifted = 32767;
    if (shifted < -32768) shifted = -32768;
    
    micBuffer[i] = (int16_t)shifted;
  }
  
  digitalWrite(LED_PIN, LOW); // LED OFF
  
  // Upload Loop
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Uploading...");
    WiFiClient client;
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/upload";
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpCode = http.POST((uint8_t *)micBuffer, totalSamples * sizeof(int16_t));
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.println("Server: " + response);
      handleServerResponse(response);
    } else {
      Serial.print("Upload Error: "); Serial.println(httpCode);
    }
    http.end();
  }
}