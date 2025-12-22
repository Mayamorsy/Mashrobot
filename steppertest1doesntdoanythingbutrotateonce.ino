// --- ARDUINO DRV8825 TROUBLESHOOTING CODE ---
#define STEP_PIN  4
#define DIR_PIN   3
#define M0_PIN    7 // Pins you mentioned
#define M1_PIN    6
#define M2_PIN    5

void setup() {
  Serial.begin(9600);
  
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  
  // DRV8825 Microstepping Pins
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(M2_PIN, OUTPUT);

  // Set to LOW for Full Step Mode (Strongest)
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);
  digitalWrite(M2_PIN, LOW);

  digitalWrite(DIR_PIN, HIGH); 
  Serial.println("DRV8825: Starting slow rotation...");
}

void loop() {
  // Step Pulse
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(10); // DRV8825 needs at least 1.9us
  digitalWrite(STEP_PIN, LOW);
  
  // If it WHINES but doesn't move, INCREASE this number (e.g., 5000)
  // If it VIBRATES but doesn't move, DECREASE this number (e.g., 1000)
  delayMicroseconds(2000); 
}