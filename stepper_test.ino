/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-drv8825-stepper-motor-driver
 */

// Include the AccelStepper Library
#include <AccelStepper.h>

// Define pin connections
#define DIR_PIN 3
#define STEP_PIN 4

// Creates an instance
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  // set the maximum speed, acceleration factor,
  // initial speed and the target position
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(200);
  stepper.setSpeed(200);
  stepper.moveTo(200);
}

void loop() {
  // Change direction once the motor reaches target position
  if (stepper.distanceToGo() == 0)
    stepper.moveTo(-stepper.currentPosition());

  stepper.run();  // Move the motor one step
}




