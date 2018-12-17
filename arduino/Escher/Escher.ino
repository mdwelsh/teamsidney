#include <vector>

#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>
#include "EscherStepper.h"

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myStepper1 = AFMS.getStepper(200, 1);
Adafruit_StepperMotor *myStepper2 = AFMS.getStepper(200, 2);
void forwardstep1() {  
  myStepper1->onestep(FORWARD, SINGLE);
}
void backwardstep1() {  
  myStepper1->onestep(BACKWARD, SINGLE);
}
void forwardstep2() {  
  myStepper2->onestep(FORWARD, DOUBLE);
}
void backwardstep2() {  
  myStepper2->onestep(BACKWARD, DOUBLE);
}

AccelStepper stepper1(forwardstep1, backwardstep1);
AccelStepper stepper2(forwardstep2, backwardstep2);
EscherStepper escher(stepper1, stepper2);

void square(EscherStepper& e, int side) {
  e.push(side, 0);
  e.push(side, side);
  e.push(0, side);
  e.push(0, 0);
}

void circle(EscherStepper &e, float radius) {
  float theta = 0.0;
  for (theta = 0.0; theta < PI*2.0; theta += (PI*2.0)/100.0) {
    float x = radius * sin(theta);
    float y = radius * cos(theta);
    Serial.printf("Radius %f theta %f x %f y %f floor x %d floor y %d\n", radius, theta, x, y, floor(x), floor(y));
    e.push(floor(x), floor(y));
  }
  e.push(floor(radius * sin(0.0)), floor(radius * cos(0.0)));
}

void setup() {  
  Serial.begin(115200);
  Serial.printf("Starting\n");
  AFMS.begin(); // Start the bottom shield
  //escher.home();
}

void loop() {
  if (!escher.run()) {
    //circle(escher, 200);
    escher.push(200, 200);
    escher.push(-200, -200);
  }
}
