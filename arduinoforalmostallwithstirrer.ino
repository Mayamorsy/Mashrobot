#include <Servo.h>

// ---------- PIN DEFINITIONS ----------
#define TRIG_SERVO 9
#define ECHO_SERVO 10
#define TRIG_PUMP  4
#define ECHO_PUMP  3
#define TRIG_RELAY 7
#define ECHO_RELAY 6

#define RELAY_PIN  8
#define IR_PIN     11

// ---------- THRESHOLDS ----------
int triggerDist = 12; 
int resetDist   = 40; 

// ---------- FLAGS & TIMER ----------
bool servoArmed  = true;
bool pumpArmed   = true;
bool stirArmed   = true;
bool relayArmed  = true;  // Fixed: Added missing declaration

bool motorState   = false;   
bool handDetected = false; 
unsigned long pumpStartTime = 0;        
const unsigned long PUMP_LIMIT = 5000;  // 5 Seconds limit

// ESP Communication flags
bool espBusy = false;
unsigned long busySince = 0;
const unsigned long BUSY_TIMEOUT = 8000;

void setup() {
  // RELAY SETUP for HW-482 (ACTIVE HIGH)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start OFF

  pinMode(TRIG_SERVO, OUTPUT); pinMode(ECHO_SERVO, INPUT);
  pinMode(TRIG_PUMP, OUTPUT);  pinMode(ECHO_PUMP, INPUT);
  pinMode(TRIG_RELAY, OUTPUT); pinMode(ECHO_RELAY, INPUT);
  pinMode(IR_PIN, INPUT);

  Serial.begin(9600);
  delay(1000); 
  Serial.println("SYSTEM READY - HW-482 ACTIVE HIGH");
}

long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 25000); 
  if (duration <= 0) return 999; 
  return duration * 0.034 / 2;
}

void loop() {
  // --- Check ESP Communications ---
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg == "BUSY") { espBusy = true; busySince = millis(); }
    else if (msg == "DONE") { espBusy = false; }
  }
  if (espBusy && (millis() - busySince > BUSY_TIMEOUT)) espBusy = false;

  long dServo = getDistance(TRIG_SERVO, ECHO_SERVO);
  long dPump  = getDistance(TRIG_PUMP, ECHO_PUMP);
  long dRelay = getDistance(TRIG_RELAY, ECHO_RELAY);

  // ---------- STATION 1: MENU (SERVO SENSOR) ----------
  if (!espBusy && dServo < triggerDist && servoArmed) {
    Serial.println("MENU");
    espBusy = true; busySince = millis();
    servoArmed = false;
  } else if (dServo > resetDist) servoArmed = true;

  // ---------- STATION 2: PERI (PUMP SENSOR MESSAGE) ----------
  if (!espBusy && dPump < triggerDist && pumpArmed) {
    Serial.println("PERI");
    pumpArmed = false;
  } else if (dPump > resetDist) pumpArmed = true;

  // ---------- STATION 3: RELAY (ACTIVE HIGH + 5s TIMER) ----------
  
  // 1. Detect Hand Presence
  if (dRelay > 0 && dRelay <= triggerDist) {
      handDetected = true;
  } else if (dRelay > resetDist || dRelay == 999) {
      handDetected = false;
      relayArmed = true; // Hand removed, allow next trigger
  }

  // 2. Trigger Pump
  if (handDetected && !motorState && relayArmed) {
      digitalWrite(RELAY_PIN, HIGH); // ON
      motorState = true;
      relayArmed = false;            // Prevent re-triggering while hand is still there
      pumpStartTime = millis();      
      Serial.println("PUMP ON (5s LIMIT)");
  } 

  // 3. Auto-Stop Logic
  if (motorState) {
      // Stop if 5 seconds pass OR if hand is removed
      if ((millis() - pumpStartTime >= PUMP_LIMIT) || !handDetected) {
          digitalWrite(RELAY_PIN, LOW); // OFF
          motorState = false;
          Serial.println("PUMP OFF");
      }
  }

  // ---------- STATION 4: IR STIRRER ----------
  if (!espBusy && digitalRead(IR_PIN) == LOW && stirArmed) {
    Serial.println("STIR");
    espBusy = true; busySince = millis();
    stirArmed = false;
  } else if (digitalRead(IR_PIN) == HIGH) stirArmed = true;

  delay(50);
}