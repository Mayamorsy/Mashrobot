#include <Stepper.h>
#include <SoftwareSerial.h>

// Connect ESP32 Pin 13 to Arduino Pin 2 (RX)
// Connect ESP32 Pin 14 to Arduino Pin 3 (TX)
SoftwareSerial espSerial(2, 3); // RX, TX

// --- PINS ---
// Stepper (DRV8833) - Pins 8, 9, 10, 11
const int STEPPER_PINS[] = {8, 9, 10, 11};
// Water Pump
const int WATER_PUMP = 12;
// IR Sensor (Start/End detection)
const int IR_SENSOR = 4;

// Ultrasonic Sensors for Stations
const int TRIG_1 = A0; const int ECHO_1 = A1; // Powder Station (Station 1)
const int TRIG_2 = A2; const int ECHO_2 = A3; // Water Station (Station 2)
const int TRIG_3 = A4; const int ECHO_3 = A5; // Milk Station (Station 3)

// --- RECIPE SETTINGS (From Old Code) ---
const int TIME_FULL_CUP   = 5000; // Tea, Black Coffee
const int TIME_HALF_CUP   = 3000; // Milk Tea, Cappuccino
const int TIME_SPLASH     = 1000; // Latte, Mocha (Small shot)

// --- VARS ---
// 200 steps per revolution for standard NEMA 17
Stepper stepper(200, 8, 9, 10, 11);
int currentStation = 0; // 0=Home, 1=Powder, 2=Water, 3=Milk

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  stepper.setSpeed(60);

  pinMode(WATER_PUMP, OUTPUT);
  pinMode(IR_SENSOR, INPUT);
  
  pinMode(TRIG_1, OUTPUT); pinMode(ECHO_1, INPUT);
  pinMode(TRIG_2, OUTPUT); pinMode(ECHO_2, INPUT);
  pinMode(TRIG_3, OUTPUT); pinMode(ECHO_3, INPUT);
  
  Serial.println("Arduino Ready. Waiting for ESP32...");
}

void loop() {
  // LISTEN TO COMMANDS FROM ESP32
  if (espSerial.available()) {
    String cmd = espSerial.readStringUntil('\n');
    cmd.trim();
    
    Serial.print("Command Received: "); Serial.println(cmd);
    
    if (cmd == "CMD:START") {
      waitForCup();
    } 
    else if (cmd == "CMD:NEXT") {
      moveToNextStation();
    }
    // --- WATER COMMANDS (Based on Recipes) ---
    else if (cmd == "CMD:WATER:HIGH") {
      pourWater(TIME_FULL_CUP); // 5 Seconds
    }
    else if (cmd == "CMD:WATER:LOW") {
      pourWater(TIME_HALF_CUP); // 3 Seconds
    }
    else if (cmd == "CMD:WATER:SPLASH") {
      pourWater(TIME_SPLASH);   // 1 Second (For Latte/Mocha)
    }
  }
}

// --- ACTIONS ---

void waitForCup() {
  Serial.println("Waiting for cup at IR...");
  // Wait until IR sensor sees something (Usually LOW = Detected, HIGH = Empty)
  while(digitalRead(IR_SENSOR) == HIGH) { 
    delay(100);
  }
  Serial.println("Cup Detected! Starting sequence.");
  espSerial.println("READY_FOR_CUP");
  // Start Moving immediately to station 1
  moveToNextStation();
}

void moveToNextStation() {
  bool arrived = false;
  Serial.println("Moving to next station...");
  
  while(!arrived) {
    stepper.step(50); // Move a small amount
    
    // Check Sensors based on where we think we are going
    if (currentStation == 0 && checkSonar(TRIG_1, ECHO_1)) {
      currentStation = 1;
      arrived = true;
      Serial.println("Arrived at Station 1 (Powder)");
      // Stop and tell ESP32 so it can dispense powder
      espSerial.println("STATION:1");
    }
    else if (currentStation == 1 && checkSonar(TRIG_2, ECHO_2)) {
      currentStation = 2;
      arrived = true;
      Serial.println("Arrived at Station 2 (Water)");
      // Stop and tell ESP32. ESP32 will then reply with:
      // CMD:WATER:HIGH or CMD:WATER:LOW or CMD:WATER:SPLASH
      espSerial.println("STATION:2");
    }
    else if (currentStation == 2 && checkSonar(TRIG_3, ECHO_3)) {
      currentStation = 3;
      arrived = true;
      Serial.println("Arrived at Station 3 (Milk)");
      // Stop and tell ESP32 so it can dispense milk
      espSerial.println("STATION:3");
    }
    else if (currentStation == 3 && digitalRead(IR_SENSOR) == LOW) {
      currentStation = 0;
      arrived = true;
      Serial.println("Arrived Home");
      espSerial.println("HOME");
    }
  }
}

void pourWater(int duration) {
  Serial.print("Pouring Water for: "); Serial.println(duration);
  digitalWrite(WATER_PUMP, HIGH);
  delay(duration);
  digitalWrite(WATER_PUMP, LOW);
  Serial.println("Water Done.");
  espSerial.println("WATER_DONE");
}

bool checkSonar(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long duration = pulseIn(echo, HIGH, 5000); // Timeout fast so we don't block stepping
  
  if (duration == 0) return false;
  
  int dist = duration * 0.034 / 2;
  // If cup is detected within 10cm, return true
  return (dist > 0 && dist < 10); 
}