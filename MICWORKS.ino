#include <Servo.h>

//
// ---------- PIN DEFINITIONS ----------
//

// Pumps (Relays)
const int waterRelayPin = 8;
const int milkRelayPin  = 4; // NEW: Connect Milk Pump Relay here

// Ingredient Ultrasonic (Checks if cup is present)
const int trigPinCup = 9;
const int echoPinCup = 10;

// Servos
const int sugarPin  = 3;  
const int teaPin    = 5;
const int coffeePin = 11;
const int cocoaPin  = 12;

//
// ---------- CONFIGURATION CONSTANTS ----------
//

// Distances
const int cupDetectionDist = 15; // cm (Max distance to detect a cup)

// Servo Angles
const int angleClosed = 0;
const int angleOpen   = 90;
const int dropTime    = 1500; // How long servo stays open to drop powder

// Pump Timings (Milliseconds) - ADJUST THESE FOR YOUR CUP SIZE!
const int TIME_FULL_CUP   = 5000; // e.g., 5 seconds for full water
const int TIME_HALF_CUP   = 2500; 
const int TIME_SPLASH     = 1000; // Small amount
const int TIME_MOSTLY_CUP = 4000; 

//
// ---------- GLOBALS ----------
//
Servo sugarServo, teaServo, coffeeServo, cocoaServo;

//
// ---------- HELPER FUNCTIONS ----------
//

// 1. Measure Distance
long getDistance() {
  digitalWrite(trigPinCup, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinCup, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinCup, LOW);

  long duration = pulseIn(echoPinCup, HIGH, 30000);
  if (duration == 0) return 999; // No echo
  return duration * 0.034 / 2;
}

// 2. Wait for Cup
void waitForCup() {
  Serial.println("Waiting for cup...");
  while (true) {
    long d = getDistance();
    if (d > 0 && d < cupDetectionDist) {
      Serial.println("Cup Detected!");
      delay(1000); // Give user 1 sec to stabilize cup
      break; 
    }
    delay(100);
  }
}

// 3. Generic Dispense Powder Function
void dispensePowder(Servo &s, String name) {
  waitForCup(); // Safety check
  Serial.print("Dispensing: "); Serial.println(name);
  
  s.write(angleOpen);
  delay(dropTime);
  s.write(angleClosed);
  delay(500); // Wait for servo to close
}

// 4. Generic Dispense Liquid Function
void dispenseLiquid(int pin, int duration, String name) {
  waitForCup(); // Safety check
  Serial.print("Pouring: "); Serial.println(name);
  
  digitalWrite(pin, HIGH); // Pump ON
  delay(duration);
  digitalWrite(pin, LOW);  // Pump OFF
  delay(500); // Drip time
}

//
// ---------- INGREDIENT ACTIONS ----------
//

void addTea()    { dispensePowder(teaServo, "Tea Powder"); }
void addCoffee() { dispensePowder(coffeeServo, "Coffee Powder"); }
void addCocoa()  { dispensePowder(cocoaServo, "Cocoa Powder"); }
void addSugar()  { dispensePowder(sugarServo, "Sugar"); }

void addWater(int time) { dispenseLiquid(waterRelayPin, time, "Water"); }
void addMilk(int time)  { dispenseLiquid(milkRelayPin, time, "Milk"); }

//
// ---------- RECIPES ----------
//

void makeTea(int sugarCount) {
  Serial.println("\n--- RECIPE: Tea ---");
  addTea();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_FULL_CUP); 
  Serial.println("Done!");
}

void makeMilkTea(int sugarCount) {
  Serial.println("\n--- RECIPE: Milk Tea ---");
  addTea();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_HALF_CUP);
  addMilk(TIME_HALF_CUP);
  Serial.println("Done!");
}

void makeBlackCoffee(int sugarCount) {
  Serial.println("\n--- RECIPE: Black Coffee ---");
  addCoffee();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_FULL_CUP);
  Serial.println("Done!");
}

void makeLatte(int sugarCount) {
  Serial.println("\n--- RECIPE: Latte ---");
  addCoffee();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_SPLASH);     // Just a shot of coffee water
  addMilk(TIME_MOSTLY_CUP);  // Mostly milk
  Serial.println("Done!");
}

void makeCappuccino(int sugarCount) {
  Serial.println("\n--- RECIPE: Cappuccino ---");
  addCoffee();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_HALF_CUP);
  addMilk(TIME_HALF_CUP);
  Serial.println("Done!");
}

void makeMocha(int sugarCount) {
  Serial.println("\n--- RECIPE: Mocha ---");
  addCoffee();
  addCocoa();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_SPLASH);
  addMilk(TIME_MOSTLY_CUP);
  Serial.println("Done!");
}

void makeHotChocolate(int sugarCount) {
  Serial.println("\n--- RECIPE: Hot Chocolate ---");
  addCocoa();
  for(int i=0; i<sugarCount; i++) addSugar();
  addWater(TIME_HALF_CUP); // Some hot water to melt powder
  addMilk(TIME_HALF_CUP);  // Creamy milk
  Serial.println("Done!");
}

//
// ---------- SYSTEM SETUP ----------
//
void setup() {
  Serial.begin(9600);

  // Setup Relays
  pinMode(waterRelayPin, OUTPUT);
  pinMode(milkRelayPin, OUTPUT);
  digitalWrite(waterRelayPin, LOW); // Ensure OFF
  digitalWrite(milkRelayPin, LOW);  // Ensure OFF

  // Setup Ultrasonic
  pinMode(trigPinCup, OUTPUT);
  pinMode(echoPinCup, INPUT);

  // Setup Servos
  sugarServo.attach(sugarPin);
  teaServo.attach(teaPin);
  coffeeServo.attach(coffeePin);
  cocoaServo.attach(cocoaPin);

  // Reset Servos to closed
  sugarServo.write(angleClosed);
  teaServo.write(angleClosed);
  coffeeServo.write(angleClosed);
  cocoaServo.write(angleClosed);

  Serial.println("BARISTA MACHINE READY.");
  Serial.println("Commands: tea, milktea, coffee, latte, cappuccino, mocha, chocolate");
}

//
// ---------- MAIN LOOP ----------
//
void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase(); // Handle capital letters

    if (command == "") return;

    Serial.print("Selected: "); Serial.println(command);
    
    // Ask for sugar
    Serial.println("Enter Sugar Spoons (0-4):");
    while (!Serial.available()); // Wait for input
    int sugar = Serial.parseInt();
    
    // Execute Recipe
    if      (command == "tea")        makeTea(sugar);
    else if (command == "milktea")    makeMilkTea(sugar);
    else if (command == "coffee")     makeBlackCoffee(sugar);
    else if (command == "black coffee") makeBlackCoffee(sugar);
    else if (command == "latte")      makeLatte(sugar);
    else if (command == "cappuccino") makeCappuccino(sugar);
    else if (command == "mocha")      makeMocha(sugar);
    else if (command == "chocolate")  makeHotChocolate(sugar);
    else if (command == "hot chocolate") makeHotChocolate(sugar);
    else Serial.println("Unknown Recipe!");

    Serial.println("\nWaiting for next order...");
    // Clear buffer
    while(Serial.available()) Serial.read(); 
  }
}