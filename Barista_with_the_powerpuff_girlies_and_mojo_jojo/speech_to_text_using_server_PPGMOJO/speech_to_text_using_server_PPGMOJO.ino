#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "";
const char* password = "";
const char* serverUrl = "http://192.168.1.9:5000/upload"; // Replace X with your PC's IP

const int micPin = 4;
const int ledPin = 5;
const int buttonPin = 0;

// Audio settings
const int sampleRate = 16000; 
const int recordTime = 4; // Record for 3 seconds
uint16_t *audioBuffer;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Allocate memory for 3 seconds of audio
  audioBuffer = (uint16_t*) malloc(sampleRate * recordTime * sizeof(uint16_t));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void loop() {
  if (digitalRead(buttonPin) == LOW) { // Button pressed
    recordAudio();
    sendAudio();
  }
}

void recordAudio() {
  Serial.println("Recording...");
  long startMillis = millis();
  
  for (int i = 0; i < sampleRate * recordTime; i++) {
    int val = analogRead(micPin);
    audioBuffer[i] = val;
    
    // LED Logic: If loud enough, light up
    // 2000 is roughly half of 4095 (3.3V center bias)
    // Adjust 200 threshold sensitivity
    if (abs(val - 2000) > 200) { 
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
    
    // Rudimentary delay to control sample rate (approx)
    // Real code should use I2S or Timers for precision
    delayMicroseconds(40); 
  }
  digitalWrite(ledPin, LOW);
  Serial.println("Finished Recording.");
}

void sendAudio() {
  Serial.println("Sending to Server...");
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/octet-stream");
  
  // Create a WAV header here ideally, or send raw and handle in Python
  int responseCode = http.POST((uint8_t*)audioBuffer, sampleRate * recordTime * sizeof(uint16_t));
  
  if (responseCode > 0) {
    String response = http.getString();
    Serial.println(responseCode);
    Serial.println(response);
  } else {
    Serial.printf("Error: %s\n", http.errorToString(responseCode).c_str());
  }
  http.end();
}