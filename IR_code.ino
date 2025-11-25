int IRSensor = 9;  // HW-201 OUT pin
int LED = 13;

void setup() {
  Serial.begin(115200);
  pinMode(IRSensor, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {

  int sensorStatus = digitalRead(IRSensor);

  // HW-201 logic is inverted:
  // LOW  = Object detected (close)
  // HIGH = No object
  if (sensorStatus == LOW) {
    Serial.println("Object within range");
    digitalWrite(LED, HIGH);   // Turn LED ON
  } 
  else {
    Serial.println("No object or too far");
    digitalWrite(LED, LOW);    // LED OFF
  }

  delay(100);
}
