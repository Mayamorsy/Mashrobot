#include <Servo.h>

//
// ---------- PIN DEFINITIONS ----------
//

// Water pump ultrasonic
const int trigPin1 = 7;
const int echoPin1 = 6;
const int relayPin = 8;

// Ingredient ultrasonic (for detecting cup/hand under powder)
const int trigPin2 = 9;
const int echoPin2 = 10;

// Servo pins
const int sugarPin  = 3;   // CONFIRMED
const int teaPin    = 5;
const int coffeePin = 11;
const int cocoaPin  = 12;

//
// ---------- CONSTANTS ----------
//
const int thresholdDistance = 10;   // Water pump ultrasonic trigger distance
const int detectionDistance = 20;   // Powder-dispensing ultrasonic trigger

const int initialAngle = 0;
const int activationAngle = 90;
const int holdTime = 2000; // 2 seconds

//
// ---------- GLOBALS ----------
//
Servo sugarServo, teaServo, coffeeServo, cocoaServo;

bool motorState = false;
bool handDetected = false;

//
// ---------- FUNCTIONS ----------
//

// Measure distance helper
long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return (distance > 0 && distance < 400) ? distance : -1;
}

// Water pump controller
void waterPump() {
  long distance = getDistance(trigPin1, echoPin1);

  if (distance > 0 && distance <= thresholdDistance) {
    if (!handDetected) handDetected = true;
  } else {
    handDetected = false;
  }

  if (handDetected && !motorState) {
    digitalWrite(relayPin, HIGH);
    motorState = true;
    Serial.println("Water Pump ON");
  } else if (!handDetected && motorState) {
    digitalWrite(relayPin, LOW);
    motorState = false;
    Serial.println("Water Pump OFF");
  }

  delay(50);
}

// Run a servo once (0 → 90 → 0)
void dispense(Servo &S) {
  // Wait until a hand/cup is detected
  while (true) {
    long d = getDistance(trigPin2, echoPin2);
    if (d > 0 && d < detectionDistance) break;
    delay(50);
  }

  S.write(activationAngle);
  delay(holdTime);
  S.write(initialAngle);
  delay(500);
}

// ------------- Ingredient functions -------------
void dispenseTea() { dispense(teaServo); }
void dispenseCoffee() { dispense(coffeeServo);
dispense(coffeeServo); }
void dispenseCocoa() { dispense(cocoaServo); 
dispense(cocoaServo);}
void dispenseSugar() { dispense(sugarServo); }

// Recipes
void makeTea(int sugarAmount) {
  Serial.println("Making Tea...");
  dispenseTea();
  for (int i = 0; i < sugarAmount; i++) dispenseSugar();
}

void makeCoffee(int sugarAmount) {
  Serial.println("Making Coffee...");
  dispenseCoffee();
  for (int i = 0; i < sugarAmount; i++) dispenseSugar();
}

void makeHotChocolate(int sugarAmount) {
  Serial.println("Making Hot Chocolate...");
  dispenseCocoa();
  for (int i = 0; i < sugarAmount; i++) dispenseSugar();
}

//
// ---------- SETUP ----------
//
void setup() {
  Serial.begin(9600);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(relayPin, OUTPUT);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  sugarServo.attach(sugarPin);
  teaServo.attach(teaPin);
  coffeeServo.attach(coffeePin);
  cocoaServo.attach(cocoaPin);

  sugarServo.write(0);
  teaServo.write(0);
  coffeeServo.write(0);
  cocoaServo.write(0);

  digitalWrite(relayPin, LOW);

  Serial.println("READY!");
  Serial.println("Enter recipe: tea / coffee / chocolate");
}

//
// ---------- MAIN LOOP ----------
//
void loop() {
  waterPump();

  if (Serial.available()) {
    String recipe = Serial.readStringUntil('\n');
    recipe.trim();

    Serial.println("How many teaspoons of sugar? (0-5)");
    while (!Serial.available());
    int sugar = Serial.parseInt();

    if (recipe == "tea") makeTea(sugar);
    else if (recipe == "coffee") makeCoffee(sugar);
    else if (recipe == "chocolate") makeHotChocolate(sugar);
    else Serial.println("Invalid recipe name!");
    
    Serial.println("Done! Enter next recipe:");
  }
}
