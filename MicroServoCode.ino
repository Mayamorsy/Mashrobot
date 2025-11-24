#include <Servo.h>

// --- Ultrasonic Sensor Pins ---
const int trigPin = 9;  // Connects to Arduino Digital Pin 9
const int echoPin = 10;  // Connects to Arduino Digital Pin 8

// --- Servo Motor Pin ---
const int servoPin = 3;  // Connects to Arduino Digital Pin 6 (PWM Pin)

// --- Constants and Variables ---
const int detectionDistance = 20; // Maximum distance (in cm) to trigger the servo
const int initialAngle = 0;       // The "as it was" position
const int activationAngle = 90;   // The angle to move to when triggered
const int holdTime = 2000;        // Time (in milliseconds) to hold the activated angle (2 seconds)

Servo myservo; // Create a servo object

void setup() {
  // Initialize Serial communication for debugging (optional)
  Serial.begin(9600);

  // Set up Ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Attach the servo object to the pin and set initial position
  myservo.attach(servoPin);
  myservo.write(initialAngle); 
}

void loop() {
  // 1. Measure Distance
  long duration;
  float distanceCm;

  // Clear the trigger pin by setting it LOW for a short period
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Trigger the ultrasonic burst by setting the trigPin HIGH for 10us
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the duration of the sound wave's return time
  duration = pulseIn(echoPin, HIGH);

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