// ---------- PIN DEFINITIONS ----------
#define TRIG_SERVO 9
#define ECHO_SERVO 10
#define TRIG_PUMP  4
#define ECHO_PUMP  3
#define TRIG_RELAY 7
#define ECHO_RELAY 6

#define RELAY_PIN  8
#define IR_PIN     11

// ---------- STEPPER PINS ----------
#define STEP_PIN  5
#define DIR_PIN   13 

// ---------- THRESHOLDS ----------
int triggerDist = 12; 
int resetDist   = 40; 

// ---------- FLAGS & TIMER ----------
bool servoArmed  = true;
bool pumpArmed   = true;
bool stirArmed   = true;
bool relayArmed  = true;

bool motorState   = false;   
bool handDetected = false; 
unsigned long pumpStartTime = 0;        
const unsigned long PUMP_LIMIT = 5000;  // 5 Seconds limit

// ESP Communication flags
bool espBusy = false;
unsigned long busySince = 0;
const unsigned long BUSY_TIMEOUT = 8000;

// ---------- STEPPER CONTROL ----------
bool stepperInitialized = false;
bool waitingForFirstIR = true;      // Wait for first IR to start sequence
bool sequenceStarted = false;        // Has the sequence started?
bool atStirStation = false;          // Are we at the stir station?
unsigned long sequenceStartTime = 0;
unsigned long stationStopTime = 0;
bool isRotating = false;

// Timing constants
const unsigned long ROTATE_TIME = 3000;    // 3 seconds rotation
const unsigned long STOP_TIME = 10000;     // 10 seconds stop at each station

enum Station {
  STATION_IR,      // Initial position (IR station)
  STATION_MENU,    // Menu station
  STATION_PUMP,    // Water pump station  
  STATION_PERI,    // Peristaltic station
  STATION_STIR     // Stir station (back to IR)
};

Station currentStation = STATION_IR;

// ---------- STEPPER FUNCTIONS ----------
void initializeStepper() {
  if (!stepperInitialized) {
    // 1. Immediately force the pins to a known state to prevent twitching
    digitalWrite(STEP_PIN, LOW); 
    pinMode(STEP_PIN, OUTPUT);
    
    digitalWrite(DIR_PIN, HIGH); 
    pinMode(DIR_PIN, OUTPUT);
    
    // 2. Wait 2 seconds before doing anything
    delay(2000);
    
    stepperInitialized = true;
    Serial.println("[STEPPER] Initialized and ready");
  }
}

void rotateStepper(unsigned long duration) {
  Serial.print("[STEPPER] Rotating for ");
  Serial.print(duration / 1000);
  Serial.println(" seconds...");
  
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(4000);
  }
  
  Serial.println("[STEPPER] Rotation complete");
}

void startSequence() {
  if (!sequenceStarted) {
    sequenceStarted = true;
    sequenceStartTime = millis();
    currentStation = STATION_IR;
    Serial.println("\n[SEQUENCE] Starting automatic sequence...");
    Serial.println("[SEQUENCE] IR detected -> Rotating to MENU station");
    
    // First rotation: IR to MENU
    rotateStepper(ROTATE_TIME);
    currentStation = STATION_MENU;
    stationStopTime = millis();
    Serial.println("[SEQUENCE] Now at MENU station (10s stop)");
    Serial.println("[SEQUENCE] Wave hand at ultrasonic to select drink");
  }
}

void updateSequence() {
  if (!sequenceStarted) return;
  
  unsigned long currentTime = millis();
  
  // Check if we're in the 10-second stop period
  if (currentTime - stationStopTime < STOP_TIME) {
    // We're in the stop period - sensors are active
    // Nothing to do here, sensors will trigger in main loop
    return;
  } else {
    // Stop period is over - move to next station
    
    switch (currentStation) {
      case STATION_MENU:
        // Move from MENU to PUMP
        Serial.println("[SEQUENCE] Moving to PUMP station...");
        rotateStepper(ROTATE_TIME);
        currentStation = STATION_PUMP;
        stationStopTime = millis();
        Serial.println("[SEQUENCE] Now at PUMP station (10s stop)");
        Serial.println("[SEQUENCE] Wave hand at ultrasonic for water pump");
        break;
        
      case STATION_PUMP:
        // Move from PUMP to PERI
        Serial.println("[SEQUENCE] Moving to PERI station...");
        rotateStepper(ROTATE_TIME);
        currentStation = STATION_PERI;
        stationStopTime = millis();
        Serial.println("[SEQUENCE] Now at PERI station (10s stop)");
        Serial.println("[SEQUENCE] Wave hand at ultrasonic for peristaltic");
        break;
        
      case STATION_PERI:
        // Move from PERI to STIR (back to IR position)
        Serial.println("[SEQUENCE] Moving to STIR station...");
        rotateStepper(ROTATE_TIME);
        currentStation = STATION_STIR;
        stationStopTime = millis();
        atStirStation = true;
        Serial.println("[SEQUENCE] Now at STIR station (10s stop)");
        Serial.println("[SEQUENCE] IR will detect cup for stirring");
        break;
        
      case STATION_STIR:
        // Sequence complete, back to start
        Serial.println("[SEQUENCE] Process complete!");
        Serial.println("[SEQUENCE] Remove cup and place new one to restart");
        sequenceStarted = false;
        waitingForFirstIR = true;
        atStirStation = false;
        currentStation = STATION_IR;
        break;
        
      case STATION_IR:
        // Should not happen in sequence
        break;
    }
  }
}

// ---------- SETUP ----------
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
  
  // Initialize stepper
  initializeStepper();
  
  Serial.println("========================================");
  Serial.println("SYSTEM READY - HW-482 ACTIVE HIGH");
  Serial.println("========================================");
  Serial.println("Stepper: Pin 5=STEP, Pin 13=DIR");
  Serial.println("Sequence: 3s rotate, 10s stop at each station");
  Serial.println("\nWaiting for IR detection to start sequence...");
  Serial.println("Place cup at IR station");
  Serial.println("========================================\n");
}

// ---------- ULTRASONIC FUNCTION ----------
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 25000); 
  if (duration <= 0) return 999; 
  return duration * 0.034 / 2;
}

// ---------- MAIN LOOP ----------
void loop() {
  // --- Check ESP Communications ---
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg == "BUSY") { 
      espBusy = true; 
      busySince = millis(); 
    }
    else if (msg == "DONE") { 
      espBusy = false; 
    }
  }
  
  // Reset busy flag if timeout
  if (espBusy && (millis() - busySince > BUSY_TIMEOUT)) {
    espBusy = false;
    Serial.println("[WARNING] ESP32 timeout - resetting busy flag");
  }

  // --- Check IR for first detection ---
  if (waitingForFirstIR && digitalRead(IR_PIN) == LOW) {
    waitingForFirstIR = false;
    Serial.println("\n[IR] First cup detected!");
    startSequence();  // Start the automatic sequence
  }

  // --- Update sequence if started ---
  if (sequenceStarted) {
    updateSequence();
  }

  // --- Read all sensors ---
  long dServo = getDistance(TRIG_SERVO, ECHO_SERVO);
  long dPump  = getDistance(TRIG_PUMP, ECHO_PUMP);
  long dRelay = getDistance(TRIG_RELAY, ECHO_RELAY);

  // ---------- STATION 1: MENU (SERVO SENSOR) ----------
  // Only trigger if we're at the MENU station during stop period
  if (sequenceStarted && currentStation == STATION_MENU && 
      (millis() - stationStopTime < STOP_TIME) &&
      !espBusy && dServo < triggerDist && servoArmed) {
    Serial.println("[MENU] Hand detected! Sending to ESP32...");
    Serial.println("MENU");
    espBusy = true; 
    busySince = millis();
    servoArmed = false;
  } else if (dServo > resetDist) {
    servoArmed = true;
  }

  // ---------- STATION 2: PERI (PUMP SENSOR MESSAGE) ----------
  // Only trigger if we're at the PERI station during stop period
  if (sequenceStarted && currentStation == STATION_PERI && 
      (millis() - stationStopTime < STOP_TIME) &&
      !espBusy && dPump < triggerDist && pumpArmed) {
    Serial.println("[PERI] Hand detected! Sending to ESP32...");
    Serial.println("PERI");
    pumpArmed = false;
  } else if (dPump > resetDist) {
    pumpArmed = true;
  }

  // ---------- STATION 3: RELAY (ACTIVE HIGH + 5s TIMER) ----------
  // Only trigger if we're at the PUMP station during stop period
  if (sequenceStarted && currentStation == STATION_PUMP && 
      (millis() - stationStopTime < STOP_TIME)) {
    
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
      relayArmed = false;
      pumpStartTime = millis();
      Serial.println("[PUMP] Hand detected! Running water pump for 5s...");
    } 

    // 3. Auto-Stop Logic
    if (motorState) {
      // Stop if 5 seconds pass OR if hand is removed
      if ((millis() - pumpStartTime >= PUMP_LIMIT) || !handDetected) {
        digitalWrite(RELAY_PIN, LOW); // OFF
        motorState = false;
        Serial.println("[PUMP] Water pump stopped.");
      }
    }
  } else {
    // Make sure pump is off if we're not at PUMP station
    if (motorState) {
      digitalWrite(RELAY_PIN, LOW);
      motorState = false;
    }
  }

  // ---------- STATION 4: IR STIRRER ----------
  // Only trigger if we're at the STIR station during stop period
  if (sequenceStarted && currentStation == STATION_STIR && 
      (millis() - stationStopTime < STOP_TIME) &&
      !espBusy && digitalRead(IR_PIN) == LOW && stirArmed) {
    Serial.println("[STIR] Cup detected! Sending to ESP32...");
    Serial.println("STIR");
    espBusy = true; 
    busySince = millis();
    stirArmed = false;
  } else if (digitalRead(IR_PIN) == HIGH) {
    stirArmed = true;
  }

  delay(50);
}
