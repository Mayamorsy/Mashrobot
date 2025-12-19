// --- ESP32 CODE ---
#include <ESP32Servo.h>

// PIN DEFINITIONS (Using safe pins, avoiding 6 & 7)
#define COFFEE_PIN 4
#define TEA_PIN    5
#define CHOCO_PIN  12
#define SUGAR_PIN  13

// Arduino Communication
#define RX_PIN 16
#define TX_PIN 17

Servo coffeeServo;
Servo teaServo;
Servo chocoServo;
Servo sugarServo;

// State Machine Variables
bool cupDetected = false;

void setup() {
  // Serial: USB Connection to Computer (Your Terminal)
  Serial.begin(9600);
  
  // Serial2: Connection to Arduino
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // Servo Setup
  coffeeServo.attach(COFFEE_PIN);
  teaServo.attach(TEA_PIN);
  chocoServo.attach(CHOCO_PIN);
  sugarServo.attach(SUGAR_PIN);

  // Reset Servos
  coffeeServo.write(0);
  teaServo.write(0);
  chocoServo.write(0);
  sugarServo.write(0);

  Serial.println("--- SYSTEM STARTED ---");
  Serial.println("Waiting for Arduino to detect a cup...");
}

void dispense(Servo &s) {
  Serial.println("Pouring...");
  s.write(90); // Open
  delay(1500); // Pour for 1.5 seconds
  s.write(0);  // Close
  delay(500);
  Serial.println("Done.");
}

void loop() {
  // 1. Check for Cup Signal from Arduino
  if (Serial2.available() && !cupDetected) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    if (msg == "CUP") {
      cupDetected = true;
      Serial.println("\n!!! CUP DETECTED !!!");
      handleOrder(); // Start the ordering process
      cupDetected = false; // Reset for next customer
      Serial.println("Waiting for next cup...");
    }
  }
}

// Function to handle the user interaction
void handleOrder() {
  // --- STEP 1: ASK FOR DRINK ---
  Serial.println("What would you like? (coffee / tea / choco)");
  
  // Wait until user types something
  while (Serial.available() == 0) {
    // Do nothing, just wait
  }

  String drink = Serial.readStringUntil('\n');
  drink.trim(); // Remove enter key spaces
  drink.toLowerCase(); // Make lowercase

  if (drink == "coffee") {
    dispense(coffeeServo);
  } 
  else if (drink == "tea") {
    dispense(teaServo);
  } 
  else if (drink == "choco" || drink == "chocolate") {
    dispense(chocoServo);
  } 
  else {
    Serial.println("Invalid choice. Order cancelled.");
    return;
  }

  // --- STEP 2: ASK FOR SUGAR ---
  Serial.println("How many sugar spoons? (Enter a number 0-5)");

  while (Serial.available() == 0) {
    // Wait for input
  }

  String sugarInput = Serial.readStringUntil('\n');
  sugarInput.trim();
  int spoons = sugarInput.toInt();

  Serial.print("Adding ");
  Serial.print(spoons);
  Serial.println(" spoons of sugar.");

  for (int i = 0; i < spoons; i++) {
    dispense(sugarServo);
    delay(200); // Wait between spoons
  }

  Serial.println("Your drink is ready! Please remove cup.");
}