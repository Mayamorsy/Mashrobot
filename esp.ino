// --- ESP32 CODE ---
#include <ESP32Servo.h>

// PIN DEFINITIONS
#define COFFEE_PIN 4
#define TEA_PIN    5
#define CHOCO_PIN  12
#define SUGAR_PIN  13

// Communication Pins
#define RX_PIN 16
#define TX_PIN 17

Servo coffeeServo;
Servo teaServo;
Servo chocoServo;
Servo sugarServo;

bool cupDetected = false;

void setup() {
  // Start Serial for PC Monitor
  Serial.begin(9600);
  
  // Start Serial for Arduino Connection
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // Servo Setup
  coffeeServo.attach(COFFEE_PIN, 500, 2400);
  teaServo.attach(TEA_PIN, 500, 2400);
  chocoServo.attach(CHOCO_PIN, 500, 2400);
  sugarServo.attach(SUGAR_PIN, 500, 2400);

  coffeeServo.write(0);
  teaServo.write(0);
  chocoServo.write(0);
  sugarServo.write(0);

  Serial.println("--- ESP32 LISTENING ---");
  Serial.println("Connect Arduino TX to ESP32 Pin 16.");
}

void dispense(Servo &s) {
  Serial.println(">> Dispensing...");
  s.write(90); delay(1500);
  s.write(0);  delay(500);
  Serial.println(">> Done.");
}

void handleOrder() {
  Serial.println("\nWhat would you like? (coffee / tea / choco)");
  // Clear buffer
  while(Serial.available()) Serial.read(); 
  
  // Wait for user input
  while (Serial.available() == 0) {}

  String drink = Serial.readStringUntil('\n');
  drink.trim(); drink.toLowerCase();

  if (drink == "coffee") dispense(coffeeServo);
  else if (drink == "tea") dispense(teaServo);
  else if (drink == "choco" || drink == "chocolate") dispense(chocoServo);
  else { Serial.println("Cancelled."); return; }

  // Sugar
  Serial.println("Sugar spoons? (0-5)");
  while (Serial.available() == 0) {}
  
  int spoons = Serial.readStringUntil('\n').toInt();
  Serial.print("Adding "); Serial.print(spoons); Serial.println(" spoons.");
  
  for (int i = 0; i < spoons; i++) {
    dispense(sugarServo);
    delay(300);
  }
  
  Serial.println("Please use the Water Sensor to fill your cup.");
}

void loop() {
  // Listen for message from Arduino
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    
    // DEBUG: Print whatever we received to the PC
    Serial.print("Received from Arduino: ");
    Serial.println(msg);

    // If the message is correct
    if (msg == "CUP" && !cupDetected) {
      cupDetected = true;
      Serial.println("!!! CUP DETECTED !!!");
      handleOrder();
      cupDetected = false;
      Serial.println("Waiting for next customer...");
    }
  }
}