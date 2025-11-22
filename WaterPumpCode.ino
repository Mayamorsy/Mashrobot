// Ultrasonic Sensor Pins
const int trigPin = 7;
const int echoPin = 6;

// Relay Pin
const int relayPin = 8;

// Threshold Distance (in cm)
const int thresholdDistance = 10;

// Variables to track motor state and time
bool motorState = false;         // To track motor ON/OFF state
bool handDetected = false;       // To detect if hand is in range

void setup() {
  // Initialize serial monitor
  Serial.begin(9600);

  // Set up pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(relayPin, OUTPUT);

  // Ensure relay is off initially (for active LOW relay)
  digitalWrite(relayPin, LOW); // Relay off (HIGH for active LOW relays)
}

void loop() {
  // Measure distance
  long distance = getDistance();

  // Debugging: Print distance
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Check if hand is detected
  if (distance > 0 && distance <= thresholdDistance) {
    if (!handDetected) {
      handDetected = true;            // Mark hand as detected
    }
  } else {
    handDetected = false; // Reset detection when hand is removed
  }

if (handDetected && motorState == false) {
// Condition to turn ON: detected AND currently off
digitalWrite(relayPin, HIGH); // HIGH to turn ON motor (Active-HIGH)
motorState = true;
Serial.println("Motor ON");

} else if (!handDetected && motorState == true) {
// Condition to turn OFF: NOT detected AND currently on
digitalWrite(relayPin, LOW); // LOW to turn OFF motor (Active-HIGH)
motorState = false;
Serial.println("Motor OFF");
}

  delay(50); // Stabilization delay
}

// Function to measure distance using ultrasonic sensor
long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout to avoid infinite wait
  long distance = duration * 0.034 / 2;

  return (distance > 0 && distance < 400) ? distance : -1; // Return valid distance
}