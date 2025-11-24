#include <Servo.h>

// Ultrasonic Sensor Pins
const int trigPin1 = 7;
const int echoPin1 = 6;
const int trigPin2 = 9;  // Connects to Arduino Digital Pin 9
const int echoPin2 = 10;  // Connects to Arduino Digital Pin 8

// Relay Pin
const int relayPin = 8;

// Threshold Distance (in cm)
const int thresholdDistance = 10;

// Variables to track motor state and time
bool motorState = false;         // To track motor ON/OFF state
bool handDetected = false;       // To detect if hand is in range

// --- Servo Motor Pin ---
const int servoPin = 3;  // Connects to Arduino Digital Pin 6 (PWM Pin)

// --- Constants and Variables ---
const int detectionDistance = 20; // Maximum distance (in cm) to trigger the servo
const int initialAngle = 0;       // The "as it was" position
const int activationAngle = 90;   // The angle to move to when triggered
const int holdTime = 2000;        // Time (in milliseconds) to hold the activated angle (2 seconds)

Servo myservo; // Create a servo object
// Function to measure distance using ultrasonic sensor
  long getDistance() {
    digitalWrite(trigPin1, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin1, LOW);

    long duration = pulseIn(echoPin1, HIGH, 30000); // Timeout to avoid infinite wait
    long distance = duration * 0.034 / 2;

    return (distance > 0 && distance < 400) ? distance : -1; // Return valid distance
  }
void waterPump(){
  // Measure distance
  long distance = getDistance();

  // Debugging: Print distance
  Serial.print("Distance 11111111111: ");
  Serial.print(distance);
  Serial.println("cm");

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

void MicroServo(){
  // 1. Measure Distance
  long duration;
  float distanceCm;

  // Clear the trigger pin by setting it LOW for a short period
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);

  // Trigger the ultrasonic burst by setting the trigPin HIGH for 10us
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  // Measure the duration of the sound wave's return time
  duration = pulseIn(echoPin2, HIGH);

  // Calculate the distance in cm
  // Distance = (Time * Speed of Sound) / 2
  // Speed of Sound is approx 0.0343 cm/µs. (0.0343 / 2) is approx 1/58
  distanceCm = duration / 58.0;

  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");

  // 2. Check Detection Condition and Move Servo
  if (distanceCm < detectionDistance && distanceCm > 0) {
    // *** Object Detected! ***
    
    // Move servo to the activated position (90 degrees)
    myservo.write(activationAngle);
    Serial.println(">>> OBJECT DETECTED: Moving to 90 degrees");

    // Hold the position for the specified time (2 seconds)
    delay(holdTime);

    // Go back to the initial position (0 degrees)
    myservo.write(initialAngle);
    Serial.println("<<< Servo returned to 0 degrees");
    
    // Add a short delay to prevent rapid-fire detection/movement 
    // immediately after the cycle completes
    delay(500); 

  } else {
    // No object detected or object is too far away
    // Ensure the servo stays at the initial position
    myservo.write(initialAngle);
    // Add a small delay for stable operation and reading
    delay(100); 
  }
}


void setup() {
  // Initialize serial monitor
  Serial.begin(9600);

  // Set up pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(relayPin, OUTPUT);

  // Ensure relay is off initially (for active LOW relay)
  digitalWrite(relayPin, LOW); // Relay off (HIGH for active LOW relays)
//microservo
  // Set up Ultrasonic pins
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Attach the servo object to the pin and set initial position
  myservo.attach(servoPin);
  myservo.write(initialAngle); 
}

void loop() {
  waterPump();
  MicroServo();
}