// --- ARDUINO CODE (SENDER) ---
// Controls 3 Sensors: Servos (Menu), Peristaltic Pump, and Water Relay

// --- SENSORS DEFINITIONS ---

// 1. SENSOR FOR MICROSERVOS (Menu Trigger)
// Logic: Sends text "CUP" to ESP32 Serial
#define TRIG_SERVO 9
#define ECHO_SERVO 10

// 2. SENSOR FOR PERISTALTIC PUMP (Nozzle Trigger)
// Logic: Sends 5V Signal to ESP32 Pin 41
// NEW PINS: 9 and 10
#define TRIG_PUMP  4
#define ECHO_PUMP  3

// 3. SENSOR FOR RELAY (Hand Wash)
// Logic: Controls Relay Pin 8 directly
#define TRIG_RELAY 7
#define ECHO_RELAY 6
#define RELAY_PIN  8

// --- COMMUNICATION OUTPUTS ---
// Pin 2 goes to ESP32 Pin 41 (Voltage Divider required!)
#define SIGNAL_PIN 2 

// --- SETTINGS ---
int triggerDist = 10;   // Objects must be closer than 10cm
bool relayRunning = false;
long lastServoMsg = 0;  // Timer to stop spamming Serial messages

void setup() {
  Serial.begin(9600); // Communication to ESP32 (TX Pin 1 -> ESP32 RX Pin 18)

  // Setup Sensor 1 (Servos)
  pinMode(TRIG_SERVO, OUTPUT);
  pinMode(ECHO_SERVO, INPUT);

  // Setup Sensor 2 (Peristaltic Pump)
  pinMode(TRIG_PUMP, OUTPUT);
  pinMode(ECHO_PUMP, INPUT);

  // Setup Sensor 3 (Relay)
  pinMode(TRIG_RELAY, OUTPUT);
  pinMode(ECHO_RELAY, INPUT);

  // Setup Outputs
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SIGNAL_PIN, OUTPUT);

  // Start with everything OFF
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(SIGNAL_PIN, LOW);

  Serial.println("Arduino: 3-Sensor System Ready.");
}

// --- HELPER: Get Distance ---
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long duration = pulseIn(echo, HIGH, 20000); // 20ms timeout
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void loop() {
  
  // ==========================================================
  // ZONE 1: MICROSERVOS (Start the Menu)
  // Sensor Pins: 4 & 3
  // Action: Send "CUP" text via Serial (TX)
  // ==========================================================
  long distServo = getDistance(TRIG_SERVO, ECHO_SERVO);

  if (distServo > 0 && distServo < triggerDist) {
    // Only send message once every 5 seconds to avoid flooding ESP32
    if (millis() - lastServoMsg > 5000) {
      Serial.println("CUP"); 
      // You can view this on ESP32 Serial Monitor
      lastServoMsg = millis();
    }
  }

  // ==========================================================
  // ZONE 2: PERISTALTIC PUMP (Dispense Liquid)
  // Sensor Pins: 9 & 10
  // Action: Set Pin 2 HIGH (Trigger ESP32 Pin 41)
  // ==========================================================
  long distPump = getDistance(TRIG_PUMP, ECHO_PUMP);

  if (distPump > 0 && distPump < triggerDist) {
    digitalWrite(SIGNAL_PIN, HIGH); // TRIGGER ON
  } else {
    digitalWrite(SIGNAL_PIN, LOW);  // TRIGGER OFF
  }

  // ==========================================================
  // ZONE 3: WATER RELAY (Hand Wash)
  // Sensor Pins: 7 & 6
  // Action: Turn Relay Pin 8 ON/OFF directly
  // ==========================================================
  long distRelay = getDistance(TRIG_RELAY, ECHO_RELAY);

  if (distRelay > 0 && distRelay < triggerDist) {
    if (!relayRunning) {
      digitalWrite(RELAY_PIN, HIGH);
      relayRunning = true;
    }
  } else {
    if (relayRunning) {
      digitalWrite(RELAY_PIN, LOW);
      relayRunning = false;
    }
  }

  delay(50); // Small delay for stability
}