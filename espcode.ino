#include <ESP32Servo.h>

// --- PIN DEFINITIONS ---

// Servos (Pins 4, 5, 12, 13)
#define COFFEE_PIN 4
#define TEA_PIN    5
#define CHOCO_PIN  12
#define SUGAR_PIN  13

// Communication (RX Pin 18 connects to Arduino TX)
#define RX_PIN 18
#define TX_PIN 17

// NEW: Peristaltic Pump (L298N)
// Using Pin 40 for Enable (PWM Speed) to avoid Serial TX conflict
#define PUMP_ENA 40   
#define PUMP_IN1 2
#define PUMP_IN2 42

// NEW: Signal Input from Arduino (Connects to Arduino Pin 2)
#define SIGNAL_PIN 41

// --- OBJECTS & VARIABLES ---
Servo coffeeServo;
Servo teaServo;
Servo chocoServo;
Servo sugarServo;

// Safety Memory
bool pumpActionDone = false; 

// Forward Declarations
void pumpFluid(int speed, boolean forward);
void stopPump();
void handleOrder();
void dispense(Servo &s);

void setup() {
  // 1. Setup Serial Communication
  Serial.begin(115200); // PC Monitor speed
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Arduino Communication

  // 2. Setup Servos
  coffeeServo.attach(COFFEE_PIN, 500, 2400);
  teaServo.attach(TEA_PIN, 500, 2400);
  chocoServo.attach(CHOCO_PIN, 500, 2400);
  sugarServo.attach(SUGAR_PIN, 500, 2400);

  // Reset Servos to 0 position
  coffeeServo.write(0);
  teaServo.write(0);
  chocoServo.write(0);
  sugarServo.write(0);

  // 3. Setup Pump Pins
  pinMode(PUMP_ENA, OUTPUT);
  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(SIGNAL_PIN, INPUT); // Reads voltage from Arduino Pin 2

  stopPump(); // Ensure pump is OFF at start

  Serial.println("--- ESP32 SYSTEM READY ---");
  Serial.println("System waiting for sensors...");
}

void loop() {
  // --- PART 1: LISTEN FOR TEXT (Zone 1 - Menu Trigger) ---
  // Arduino Sensor 1 sends "CUP" when customer arrives
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    if (msg == "CUP") {
      Serial.println("\n[Zone 1] Customer Detected! Please place cup under nozzle.");
    }
  }

  // --- PART 2: LISTEN FOR VOLTAGE SIGNAL (Zone 2 - Pump Trigger) ---
  // Arduino Sensor 2 sends HIGH signal to Pin 41
  int signalState = digitalRead(SIGNAL_PIN);

  // IF Signal is HIGH (Cup at Nozzle) AND we haven't pumped yet
  if (signalState == HIGH) {
    if (pumpActionDone == false) {
      Serial.println(">> [Zone 2] Cup at Nozzle (Pin 41 HIGH)");
      Serial.println(">> Starting Auto-Sequence...");
      
      delay(1000); // Wait 1 sec for cup to settle

      // 1. Pump Water
      Serial.println("-> Pumping Water...");
      pumpFluid(255, true); // Full Speed
      delay(3000);          // Adjust this duration for cup size

      // 2. Pause
      stopPump();
      delay(1000);

      // 3. Retract (Suck back drips)
      Serial.println("-> Anti-Drip Retract");
      pumpFluid(150, false);
      delay(800);
      stopPump();
      
      Serial.println("-> Water Done.");
      pumpActionDone = true; // Lock pump so it doesn't overflow
      
      // 4. Ask for Powder immediately
      handleOrder(); 
    }
  }

  // IF Signal is LOW (Cup Removed), reset the pump lock
  if (signalState == LOW) {
    if (pumpActionDone == true) {
      Serial.println("[Reset] Cup removed. Ready for next customer.");
      pumpActionDone = false; 
      delay(500);
    }
  }

  // Allow manual ordering via PC Serial Monitor anytime
  if (Serial.available()) {
    handleOrder();
  }
}

// --- HELPER FUNCTIONS ---

void pumpFluid(int speed, boolean forward) {
  if (forward) {
    digitalWrite(PUMP_IN1, HIGH);
    digitalWrite(PUMP_IN2, LOW);
  } else {
    digitalWrite(PUMP_IN1, LOW);
    digitalWrite(PUMP_IN2, HIGH);
  }
  analogWrite(PUMP_ENA, speed); 
}

void stopPump() {
  digitalWrite(PUMP_IN1, LOW);
  digitalWrite(PUMP_IN2, LOW);
  analogWrite(PUMP_ENA, 0);
}

void dispense(Servo &s) {
  Serial.println(">> Dispensing Powder...");
  s.write(90); delay(1500); // Open
  s.write(0);  delay(500);  // Close
  Serial.println(">> Done.");
}

void handleOrder() {
  Serial.println("\n--- DRINK MENU ---");
  Serial.println("Type: 'coffee', 'tea', or 'choco'");
  
  // Clear Serial buffer
  while(Serial.available()) Serial.read(); 
  
  // Wait for user input
  // IMPORTANT: We keep checking if the cup is removed while waiting!
  while (Serial.available() == 0) {
     if(digitalRead(SIGNAL_PIN) == LOW && pumpActionDone == true) {
        Serial.println("[!] Cup removed during order. Cancelled.");
        return; 
     }
  }

  String drink = Serial.readStringUntil('\n');
  drink.trim(); drink.toLowerCase();

  if (drink == "coffee") dispense(coffeeServo);
  else if (drink == "tea") dispense(teaServo);
  else if (drink == "choco" || drink == "chocolate") dispense(chocoServo);
  else { Serial.println("Invalid selection."); return; }

  // Sugar Logic
  Serial.println("How many sugar spoons? (0-5)");
  while (Serial.available() == 0) {
      if(digitalRead(SIGNAL_PIN) == LOW && pumpActionDone == true) return;
  }
  
  int spoons = Serial.readStringUntil('\n').toInt();
  Serial.print("Adding "); Serial.print(spoons); Serial.println(" spoons.");
  
  for (int i = 0; i < spoons; i++) {
    dispense(sugarServo);
    delay(300);
  }
  
  Serial.println(">>> Drink Ready! Enjoy.");
}