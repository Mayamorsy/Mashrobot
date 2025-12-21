#include <ESP32Servo.h>

const int rxPin = 3;
const int servoPin = 21;
const int in3 = 1;      // Pin 35 was input only, changed to Pin 1
const int in4 = 37;

Servo myServo;

void setup() {
  // Serial1 used for Arduino communication
  Serial1.begin(9600, SERIAL_8N1, rxPin, -1);
  
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);

  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(0); // Set initial position
}

void loop() {
  if (Serial1.available() > 0) {
    char signal = Serial1.read();
    if (signal == 'C') {
      // 1. Move Servo
      myServo.write(90);
      delay(1000);

      // 2. Start Motor
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      delay(3000);

      // 3. Stop Motor
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);

      // 4. Reset Servo
      myServo.write(0);
    }
  }
}