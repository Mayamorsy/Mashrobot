#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Servo.h>
#include "Audio.h" // Install "ESP32-audioI2S" by Schreibfaul1

// --- CONFIG ---
const char* WIFI_SSID = "CAPTURE";
const char* WIFI_PASSWORD = "HOPE@9955";
// Ensure this IP matches your computer's IP
const char* SERVER_BASE_URL = "http://192.168.1.6:5000"; 

// --- PINS ---
// Voice
const int MIC_PIN = 1;
const int SPEAKER_PIN = 4;  // Crowtail Speaker
const int BUTTON_PIN = 0;   // BOOT Button
const int LED_PIN = 10;

// Communication with Arduino Uno
#define ARDUINO_TX 13 // Connect to Arduino Pin 2
#define ARDUINO_RX 14 // Connect to Arduino Pin 3

// Ingredients / Mechanics (Controlled directly by ESP32)
const int MILK_PUMP_PIN = 16;
const int STIR_MOTOR_PIN = 18;
const int STIR_SERVO_PIN = 8;

// Powder Servos
Servo sugarS, coffeeS, teaS, cocoaS, extraS, stirServo;
// NOTE: Sugar moved to Pin 2 because Pin 4 is the Speaker
const int PIN_SUGAR  = 2; 
const int PIN_COFFEE = 5;
const int PIN_TEA    = 6;
const int PIN_COCOA  = 7;
const int PIN_EXTRA  = 15;

// Audio I2S Pins
#define I2S_DOUT      SPEAKER_PIN
#define I2S_BCLK      17    // Unused but required by lib config
#define I2S_LRC       16    // Unused but required by lib config

// --- SETTINGS ---
const int SAMPLE_RATE = 16000;
const int RECORD_TIME_SECONDS = 3; 
int zeroLevel = 1550; // Mic Calibration

// --- GLOBALS ---
Audio audio;
int16_t *micBuffer = NULL;
HardwareSerial ArduinoSerial(2); // UART2

// State Tracking
String currentRecipe = "";
int currentSugar = 0;
bool isMakingDrink = false;

// --- FORWARD DECLARATIONS ---
void getWelcomeMessage();
void recordAndSend();
void closeAllServos();
void drop(Servo &s);
void dispensePowders();
void dispenseMilk(int timeMs);
void performStirring();
void handleServerResponse(String response);
void handleArduinoStatus(String status);

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  
  // 1. Setup Comms with Arduino (9600 baud)
  ArduinoSerial.begin(9600, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);

  // 2. Setup Pins
  pinMode(MILK_PUMP_PIN, OUTPUT);
  pinMode(STIR_MOTOR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 3. Setup Servos
  sugarS.attach(PIN_SUGAR); 
  coffeeS.attach(PIN_COFFEE);
  teaS.attach(PIN_TEA); 
  cocoaS.attach(PIN_COCOA);
  extraS.attach(PIN_EXTRA); 
  stirServo.attach(STIR_SERVO_PIN);
  closeAllServos();

  // 4. Audio & WiFi
  analogSetAttenuation(ADC_11db); // Mic Fix
  
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  Serial.println("\nWiFi Connected");
  digitalWrite(LED_PIN, LOW);
  
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(14);   // Low volume reduces static
  audio.forceMono(true);
  
  micBuffer = (int16_t*) malloc(SAMPLE_RATE * RECORD_TIME_SECONDS * sizeof(int16_t));
  if(micBuffer == NULL) Serial.println("RAM Critical Error");
  
  // 5. Start
  getWelcomeMessage();
}

void loop() {
  audio.loop();

  // 1. Handle User Voice Input
  // Only allow recording if we aren't currently making a drink
  if (!isMakingDrink && digitalRead(BUTTON_PIN) == LOW) {
    if(audio.isRunning()) audio.stopSong();
    
    delay(50); 
    if(digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Listening...");
      recordAndSend();
    }
  }

  // 2. Listen for Updates from Arduino Uno
  // Arduino sends status messages like "STATION:1" or "WATER_DONE"
  if (ArduinoSerial.available()) {
    String msg = ArduinoSerial.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) handleArduinoStatus(msg);
  }
}

// --- LOGIC: The "Brain" ---

void handleArduinoStatus(String status) {
  Serial.print("Arduino Status: "); Serial.println(status);

  if (status == "READY_FOR_CUP") {
    // Arduino IR sensor is waiting.
  }
  else if (status == "STATION:1") { 
    // Cup arrived at Powder Station.
    dispensePowders();
    delay(500);
    ArduinoSerial.println("CMD:NEXT"); // Tell Arduino to move to Water
  }
  else if (status == "STATION:2") {
    // Cup arrived at Water Station.
    // Decide water level based on recipe
    if (currentRecipe == "latte" || currentRecipe == "cappuccino" || currentRecipe == "milktea") {
      ArduinoSerial.println("CMD:WATER:LOW"); // Less water for milk drinks
    } else if (currentRecipe == "latte" || currentRecipe == "mocha") {
       ArduinoSerial.println("CMD:WATER:SPLASH");
    } else {
      ArduinoSerial.println("CMD:WATER:HIGH"); // Full water for black coffee/tea
    }
  }
  else if (status == "WATER_DONE") {
    // Water finishing pouring. Move to Milk.
    ArduinoSerial.println("CMD:NEXT");
  }
  else if (status == "STATION:3") {
    // Cup arrived at Milk Station. ESP32 controls this pump directly.
    if (currentRecipe == "latte" || currentRecipe == "milktea" || currentRecipe == "cappuccino" || currentRecipe == "chocolate" || currentRecipe == "mocha") {
      dispenseMilk(3000); // Pour milk for 3 seconds
    }
    // Even if no milk, we must tell Arduino to move on
    ArduinoSerial.println("CMD:NEXT");
  }
  else if (status == "HOME") {
    // Cup is back at the start.
    performStirring();
    isMakingDrink = false; // Ready for next order
    
    // Play completion sound
    getWelcomeMessage(); 
  }
}

// --- PHYSICAL ACTIONS ---

void dispensePowders() {
  Serial.println("Dispensing Powders...");
  
  if(currentRecipe == "coffee" || currentRecipe == "latte" || currentRecipe == "cappuccino" || currentRecipe == "mocha") {
    drop(coffeeS);
  }
  if(currentRecipe == "tea" || currentRecipe == "milktea") {
    drop(teaS);
  }
  if(currentRecipe == "chocolate" || currentRecipe == "mocha") {
    drop(cocoaS);
  }
  
  // Sugar is added to everything requested
  for(int i=0; i<currentSugar; i++) {
    drop(sugarS);
  }
}

void dispenseMilk(int timeMs) {
  Serial.println("Dispensing Milk...");
  digitalWrite(MILK_PUMP_PIN, HIGH);
  delay(timeMs);
  digitalWrite(MILK_PUMP_PIN, LOW);
}

void performStirring() {
  Serial.println("Stirring...");
  stirServo.write(90); // Lower the stirrer mechanism
  delay(1000);
  
  digitalWrite(STIR_MOTOR_PIN, HIGH); // Spin the DC motor
  delay(3000);
  digitalWrite(STIR_MOTOR_PIN, LOW);
  
  stirServo.write(0); // Raise the stirrer
  delay(1000);
}

void drop(Servo &s) {
  s.write(90);   // Open
  delay(1000);   // Wait for powder to fall
  s.write(0);    // Close
  delay(500);    // Wait to close
}

void closeAllServos() {
  sugarS.write(0); coffeeS.write(0); teaS.write(0); 
  cocoaS.write(0); extraS.write(0); stirServo.write(0);
}

// --- NETWORK HANDLERS ---

void handleServerResponse(String response) {
  // Format: "CMD:tea:2:http://..." OR "http://..."
  
  if (response.startsWith("CMD:")) {
    // 1. Parse Command
    int firstColon = response.indexOf(':');
    int secondColon = response.indexOf(':', firstColon+1);
    int lastColon = response.lastIndexOf(':');
    
    currentRecipe = response.substring(4, secondColon); // e.g. "tea"
    currentSugar = response.substring(secondColon+1, lastColon).toInt(); // e.g. 2
    String audioUrl = response.substring(lastColon+1);
    
    // 2. Play Confirmation Audio
    audio.connecttohost(audioUrl.c_str());
    
    // 3. Start the Machine Process
    isMakingDrink = true;
    Serial.println("Starting Machine for: " + currentRecipe);
    ArduinoSerial.println("CMD:START"); // Wake up Arduino
    
  } else if (response.startsWith("http")) {
    // Just conversation
    audio.connecttohost(response.c_str());
  }
}

void getWelcomeMessage() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/reset";
    http.begin(client, url);
    if(http.GET()==200) handleServerResponse(http.getString());
    http.end();
  }
}

void recordAndSend() {
  digitalWrite(LED_PIN, HIGH);
  
  unsigned long startTime = micros();
  const int interval = 1000000 / SAMPLE_RATE;
  int totalSamples = SAMPLE_RATE * RECORD_TIME_SECONDS;
  
  for (int i = 0; i < totalSamples; i++) {
    unsigned long next = startTime + (i * interval);
    while (micros() < next);
    int raw = analogRead(MIC_PIN);
    int shifted = (raw - zeroLevel) * 4;
    if (shifted > 32767) shifted = 32767;
    if (shifted < -32768) shifted = -32768;
    micBuffer[i] = (int16_t)shifted;
  }
  
  digitalWrite(LED_PIN, LOW);
  
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/upload";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpCode = http.POST((uint8_t *)micBuffer, totalSamples * sizeof(int16_t));
    
    if (httpCode == 200) {
      String resp = http.getString();
      Serial.println("Server: " + resp);
      handleServerResponse(resp);
    } else {
      Serial.print("Error: "); Serial.println(httpCode);
    }
    http.end();
  }
}