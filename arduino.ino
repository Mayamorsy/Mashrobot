// --- ARDUINO CODE (Dual Sensor) ---
// Upload this to the ARDUINO

// Sensor 1: Detects Cup (Triggers ESP32)
#define TRIG_CUP 9
#define ECHO_CUP 10

// Sensor 2: Detects Hand (Triggers Water Pump)
#define TRIG_WATER 7
#define ECHO_WATER 6
#define RELAY_PIN  8

bool waterRunning = false;
long lastCupCheck = 0;

void setup() {
  Serial.begin(9600); // Communication to ESP32
  
  pinMode(TRIG_CUP, OUTPUT);   pinMode(ECHO_CUP, INPUT);
  pinMode(TRIG_WATER, OUTPUT); pinMode(ECHO_WATER, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  digitalWrite(RELAY_PIN, LOW); // Keep pump off
}

// Helper to get distance
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 20000); // 20ms timeout
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void loop() {
  // --- PRIORITY 1: WATER PUMP (Fast Response) ---
  long distWater = getDistance(TRIG_WATER, ECHO_WATER);
  
  if (distWater > 0 && distWater < 10) {
    if (!waterRunning) {
      digitalWrite(RELAY_PIN, HIGH); // Pump ON
      waterRunning = true;
    }
  } else {
    if (waterRunning) {
      digitalWrite(RELAY_PIN, LOW); // Pump OFF
      waterRunning = false;
    }
  }

  // --- PRIORITY 2: CUP DETECTION (Check every 200ms) ---
  if (millis() - lastCupCheck > 200) {
    long distCup = getDistance(TRIG_CUP, ECHO_CUP);
    
    // If Cup detected
    if (distCup > 0 && distCup < 10) {
      Serial.println("CUP"); // Send message to ESP32
      // Wait 5 seconds so we don't send "CUP" 100 times
      // We use a "smart delay" so the water pump still works during the wait
      long startWait = millis();
      while(millis() - startWait < 5000) {
         // Keep checking water sensor!
         long d = getDistance(TRIG_WATER, ECHO_WATER);
         if(d > 0 && d < 10) digitalWrite(RELAY_PIN, HIGH);
         else digitalWrite(RELAY_PIN, LOW);
      }
    }
    lastCupCheck = millis();
  }
}