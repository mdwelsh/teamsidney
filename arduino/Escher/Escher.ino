#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>

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

void setup()
{  
  AFMS.begin(); // Start the bottom shield
   
  stepper1.setMaxSpeed(100.0);
  stepper1.setAcceleration(100.0);
  stepper1.moveTo(24);
    
  stepper2.setMaxSpeed(200.0);
  stepper2.setAcceleration(100.0);
  stepper2.moveTo(50000);
}

void loop()
{
    // Change direction at the limits
    if (stepper1.distanceToGo() == 0) stepper1.moveTo(-stepper1.currentPosition());

    if (stepper2.distanceToGo() == 0) stepper2.moveTo(-stepper2.currentPosition());

    stepper1.run();
    stepper2.run();
}
