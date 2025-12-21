#include <SoftwareSerial.h>

const int irPin = 11;
const int txPin = 12;

// SoftwareSerial(RX, TX)
SoftwareSerial espSerial(-1, txPin); 

void setup() {
  pinMode(irPin, INPUT);
  espSerial.begin(9600);
}

void loop() {
  // If IR sensor sees a cup
  if (digitalRead(irPin) == LOW) {
    espSerial.print('C');
    delay(6000); // Wait for the motor process to finish
  }
}