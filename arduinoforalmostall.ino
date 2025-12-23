// --- ARDUINO CODE: SENSOR HUB ---
#define TRIG_SERVO 9
#define ECHO_SERVO 10
#define TRIG_PUMP  4
#define ECHO_PUMP  3
#define TRIG_RELAY 7
#define ECHO_RELAY 6
#define RELAY_PIN  8  // Small Pump Relay
#define IR_PIN     11 // IR Sensor for Stirrer

int triggerDist = 12; // Adjusted for stability
int resetDist   = 40;

bool servoArmed = true;
bool pumpArmed  = true;
bool relayArmed = true;
bool stirArmed  = true;

void setup() {
  digitalWrite(RELAY_PIN, HIGH); 
  pinMode(RELAY_PIN, OUTPUT);  
  Serial.begin(9600); // Set ESP32 Serial2 to 9600 to match
  pinMode(TRIG_SERVO, OUTPUT); pinMode(ECHO_SERVO, INPUT);
  pinMode(TRIG_PUMP, OUTPUT);  pinMode(ECHO_PUMP, INPUT);
  pinMode(TRIG_RELAY, OUTPUT); pinMode(ECHO_RELAY, INPUT);
  pinMode(IR_PIN, INPUT);      
  
  
  
  Serial.println("Arduino Ready. Relay forced OFF on startup.");
}

long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 25000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void loop() {
  long dServo = getDistance(TRIG_SERVO, ECHO_SERVO);
  delay(20); // Prevent ultrasonic crosstalk
  long dPump  = getDistance(TRIG_PUMP, ECHO_PUMP);
  delay(20);
  long dRelay = getDistance(TRIG_RELAY, ECHO_RELAY);

  // --- ZONE 1: MENU (ESP32) ---
  if (dServo < triggerDist && servoArmed) {
    Serial.println("MENU");
    servoArmed = false;
  } else if (dServo > resetDist) { servoArmed = true; }

  // --- ZONE 2: PERI PUMP (ESP32) ---
  if (dPump < triggerDist && pumpArmed) {
    Serial.println("PERI");
    pumpArmed = false;
  } else if (dPump > resetDist) { pumpArmed = true; }

  // --- ZONE 3: RELAY (Small Pump) ---
  if (dRelay < triggerDist && relayArmed) {
    Serial.println("RELAY_START");
    
    digitalWrite(RELAY_PIN, LOW);   // <--- THIS TURNS IT ON
    delay(1000);                    // 1 Second pulse
    digitalWrite(RELAY_PIN, HIGH);  // <--- THIS TURNS IT OFF
    
    relayArmed = false;            
  } 
  else if (dRelay > resetDist) {
    relayArmed = true;             
  }
  // --- ZONE 4: IR STIRRER (ESP32) ---
  if (digitalRead(IR_PIN) == LOW && stirArmed) {
    Serial.println("STIR");
    stirArmed = false;
  } else if (digitalRead(IR_PIN) == HIGH) { stirArmed = true; }

  delay(50);
}