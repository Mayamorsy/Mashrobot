// --- ARDUINO CODE ---
#define TRIG_PIN 9
#define ECHO_PIN 10

void setup() {
  Serial.begin(9600); // Talk to ESP32
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  long duration;
  int distance;

  // Measure distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  // If cup detected (< 10cm)
  if (distance > 0 && distance < 10) {
    Serial.println("CUP"); // Tell ESP32
    delay(5000); // Wait 5 seconds to avoid spamming
  }
  
  delay(100);
}