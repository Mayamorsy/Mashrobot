#include <ESP32Servo.h>

// ---------- SERVOS ----------
#define COFFEE_PIN 4
#define TEA_PIN    5
#define CHOCO_PIN  12
#define SUGAR_PIN  13
#define STIR_SERVO_PIN 21

// ---------- STIR MOTOR ----------
#define STIR_MOTOR_ENB 9
#define STIR_MOTOR_IN3 1
#define STIR_MOTOR_IN4 37

// ---------- PERISTALTIC PUMP ----------
#define PUMP_ENA 40
#define PUMP_IN1 2
#define PUMP_IN2 42

// ---------- SERIAL ----------
#define RX_PIN 18
#define TX_PIN 17

Servo coffeeServo, teaServo, chocoServo, sugarServo, stirServo;
bool systemBusy = false;

void dispense(Servo &s) {
  s.write(90); delay(1200);
  s.write(0);  delay(400);
}

void handleOrder() {
  if (systemBusy) return;
  systemBusy = true;
  Serial2.println("BUSY");

  Serial.println("Drink? coffee / tea / choco");
  while (!Serial.available()) delay(10);
  String drink = Serial.readStringUntil('\n');
  drink.trim(); drink.toLowerCase();

  if (drink == "coffee") dispense(coffeeServo);
  else if (drink == "tea") dispense(teaServo);
  else if (drink == "choco") dispense(chocoServo);

  Serial.println("Sugar 0-5?");
  while (!Serial.available()) delay(10);
  int spoons = Serial.readStringUntil('\n').toInt();
  for (int i = 0; i < spoons; i++) { dispense(sugarServo); delay(200); }

  Serial2.println("DONE");
  systemBusy = false;
}

void runStirrer() {
  if (systemBusy) return;
  systemBusy = true;
  Serial2.println("BUSY");

  stirServo.write(110);        // Move MG996R down
  delay(1200); 

  digitalWrite(STIR_MOTOR_IN3, HIGH);
  digitalWrite(STIR_MOTOR_IN4, LOW);
  analogWrite(STIR_MOTOR_ENB, 75); // Lower speed (adjust 0-255)
  
  delay(2000);                 // Stir for 2 seconds

  digitalWrite(STIR_MOTOR_IN3, HIGH); // Active Brake
  digitalWrite(STIR_MOTOR_IN4, HIGH);
  delay(300);
  
  analogWrite(STIR_MOTOR_ENB, 0);    // Power off
  digitalWrite(STIR_MOTOR_IN3, LOW);
  digitalWrite(STIR_MOTOR_IN4, LOW);

  stirServo.write(0);          // Lift MG996R up
  delay(1200); 

  Serial2.println("DONE");
  systemBusy = false;
}

void runPeriPump() {
  if (systemBusy) return;
  systemBusy = true;
  Serial2.println("BUSY");

  digitalWrite(PUMP_IN1, HIGH);
  digitalWrite(PUMP_IN2, LOW);
  analogWrite(PUMP_ENA, 255);
  delay(5000); // pump runs 5s
  analogWrite(PUMP_ENA, 0);

  Serial2.println("DONE");
  systemBusy = false;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  coffeeServo.attach(COFFEE_PIN);
  teaServo.attach(TEA_PIN);
  chocoServo.attach(CHOCO_PIN);
  sugarServo.attach(SUGAR_PIN);
  stirServo.attach(STIR_SERVO_PIN);

  pinMode(PUMP_ENA, OUTPUT);
  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(STIR_MOTOR_ENB, OUTPUT);
  stirServo.write(0);
  pinMode(STIR_MOTOR_IN3, OUTPUT);
  pinMode(STIR_MOTOR_IN4, OUTPUT);

  Serial.println("ESP32 READY");
}

void loop() {
  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();

    if (cmd == "MENU") handleOrder();
    else if (cmd == "STIR") runStirrer();
    else if (cmd == "PERI") runPeriPump();
  }
}