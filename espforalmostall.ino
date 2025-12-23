#include <ESP32Servo.h>

// Beverage Servos
#define COFFEE_PIN 4
#define TEA_PIN    5
#define CHOCO_PIN  12
#define SUGAR_PIN  13

// Stirrer Components
#define STIR_SERVO_PIN 21
#define STIR_MOTOR_IN3 1
#define STIR_MOTOR_IN4 37

// Communication
#define RX_PIN 18
#define TX_PIN 17

// Peristaltic Pump
#define PUMP_ENA 40
#define PUMP_IN1 2
#define PUMP_IN2 42

Servo coffeeServo, teaServo, chocoServo, sugarServo, stirServo;
bool systemBusy = false;

// --- Fill Variables ---
int pumpTime = 1000; // Default fill time in milliseconds

void pumpFluid(int speed, boolean forward, int duration) {
  if (forward) { digitalWrite(PUMP_IN1, HIGH); digitalWrite(PUMP_IN2, LOW); }
  else { digitalWrite(PUMP_IN1, LOW); digitalWrite(PUMP_IN2, HIGH); }
  
  analogWrite(PUMP_ENA, speed);
  delay(duration);
  stopPump();
}

void stopPump() {
  digitalWrite(PUMP_IN1, LOW); digitalWrite(PUMP_IN2, LOW);
  analogWrite(PUMP_ENA, 0);
}

void dispense(Servo &s) {
  s.write(90); delay(1200);
  s.write(0);  delay(400);
}

void handleOrder() {
  if(systemBusy) return;
  systemBusy = true;

  Serial.println("\n--- DRINK MENU ---");
  Serial.println("Type: coffee / tea / choco");
  while (!Serial.available()) delay(10);
  String drink = Serial.readStringUntil('\n');
  drink.trim(); drink.toLowerCase();

  if (drink == "coffee") dispense(coffeeServo);
  else if (drink == "tea") dispense(teaServo);
  else if (drink == "choco" || drink == "chocolate") dispense(chocoServo);
  else { Serial.println("Invalid selection."); systemBusy = false; return; }

  // --- New Pump Time Option ---
  // Serial.println("Cup size? (1 = Small, 2 = Large)");
  // while (!Serial.available()) delay(10);
  // int sizeChoice = Serial.readStringUntil('\n').toInt();
  // int activePumpTime = (sizeChoice == 2) ? (pumpTime * 2) : pumpTime;

  Serial.println("Sugar spoons 0-5?");
  while (!Serial.available()) delay(10);
  int spoons = Serial.readStringUntil('\n').toInt();
  
  for (int i = 0; i < spoons; i++) { dispense(sugarServo); delay(200); }

  // Serial.println("Dispensing liquid...");
  // pumpFluid(255, true, activePumpTime);
  
  Serial.println("Done.");
  systemBusy = false;
}

void runStirrerSequence() {
  if(systemBusy) return;
  systemBusy = true;
  
  Serial.println("[STIR] Lowering stirrer...");
  stirServo.write(90);
  delay(1000);

  Serial.println("[STIR] Motor spinning...");
  digitalWrite(STIR_MOTOR_IN3, HIGH);
  digitalWrite(STIR_MOTOR_IN4, LOW);
  
  delay(1000); // EXACTLY 1 second

  // --- BRAKING LOGIC ---
  Serial.println("[STIR] Active Braking...");
  digitalWrite(STIR_MOTOR_IN3, HIGH); // Setting both HIGH stops the motor faster
  digitalWrite(STIR_MOTOR_IN4, HIGH);
  delay(100); 
  digitalWrite(STIR_MOTOR_IN3, LOW);
  digitalWrite(STIR_MOTOR_IN4, LOW);
  
  Serial.println("[STIR] Raising stirrer...");
  stirServo.write(0);
  delay(1000);
  
  systemBusy = false;
}

void runRelayPump() {
  // If your small pump is on a relay, we use a digital pin
  // Assuming PUMP_IN1 is your relay trigger pin
  Serial.println("[PUMP] Relay ON for 1 second...");
  
  // For most relay modules: LOW = ON, HIGH = OFF
  digitalWrite(PUMP_IN1, LOW); 
  delay(1000); 
  digitalWrite(PUMP_IN1, HIGH); 
  
  Serial.println("[PUMP] Relay OFF.");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  coffeeServo.attach(COFFEE_PIN, 500, 2400);
  teaServo.attach(TEA_PIN, 500, 2400);
  chocoServo.attach(CHOCO_PIN, 500, 2400);
  sugarServo.attach(SUGAR_PIN, 500, 2400);
  stirServo.attach(STIR_SERVO_PIN, 500, 2400);

  coffeeServo.write(0); teaServo.write(0); 
  chocoServo.write(0); sugarServo.write(0); stirServo.write(0);

  pinMode(PUMP_ENA, OUTPUT); pinMode(PUMP_IN1, OUTPUT); pinMode(PUMP_IN2, OUTPUT);
  pinMode(STIR_MOTOR_IN3, OUTPUT); pinMode(STIR_MOTOR_IN4, OUTPUT);
  
  stopPump();
  Serial.println("--- ESP32 SYSTEM READY ---");
}

void loop() {
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();

    if (msg == "MENU") handleOrder();
    else if (msg == "STIR") runStirrerSequence();
    else if (msg == "PERI") {
       // Dedicated Peristaltic trigger (if used separately from main menu)
       pumpFluid(255, true, pumpTime); 
    }
  }

  if (Serial.available()) handleOrder();
}