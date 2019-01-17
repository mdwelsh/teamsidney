#include <vector>

#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>
#include "EscherStepper.h"

// Uncomment the following to print generated GCode.
#define USE_GCODE
#ifdef USE_GCODE
#include "gcode.h"
#endif

// Define this to reverse the axes (e.g., if using gears to mate between
// the steppers and the Etch-a-Sketch).
#define REVERSE_AXES
#ifdef REVERSE_AXES
#define FORWARD_STEP BACKWARD
#define BACKWARD_STEP FORWARD
#else
#define FORWARD_STEP FORWARD
#define BACKWARD_STEP BACKWARD
#endif

#define MAX_SPEED 50.0

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myStepper1 = AFMS.getStepper(200, 1);
Adafruit_StepperMotor *myStepper2 = AFMS.getStepper(200, 2);
void forwardstep1() {  
  myStepper1->onestep(FORWARD_STEP, SINGLE);
}
void backwardstep1() {
  myStepper1->onestep(BACKWARD_STEP, SINGLE);
}
void forwardstep2() {  
  myStepper2->onestep(FORWARD_STEP, DOUBLE);
}
void backwardstep2() {  
  myStepper2->onestep(BACKWARD_STEP, DOUBLE);
}

AccelStepper stepper1(forwardstep1, backwardstep1);
AccelStepper stepper2(forwardstep2, backwardstep2);
MultiStepper mstepper;
EscherStepper escher(mstepper);

void square(EscherStepper& e, int side) {
  e.push(side, 0);
  e.push(side, side);
  e.push(0, side);
  e.push(0, 0);
}

// Draw a grid using alternating square waves in a zigzag pattern.
void grid(EscherStepper&e, int side, int rows, int columns) {
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < columns; col++) {
      if (row % 2 == 0) {
        // Even-numbered row, go left to right.
        int bx = col * (2 * side);
        int by = row * side;
        e.push(bx, by);
        e.push(bx, by+side);
        e.push(bx+side, by+side);
        e.push(bx+side, by);
        e.push(bx+(side*2), by);
      } else {
        // Odd-numbered row, go right to left.
        int bx = (columns - col) * (2 * side);
        int by = row * side;
        e.push(bx, by);
        e.push(bx-side, by);
        e.push(bx-side, by+side);
        e.push(bx-(2*side), by+side);
        e.push(bx-(2*side), by);
      }
    }
  }
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

int curPoint = 0;

void setup() {  
  Serial.begin(115200);
  Serial.printf("Starting\n");
  stepper1.setMaxSpeed(MAX_SPEED);
  stepper2.setMaxSpeed(MAX_SPEED);
  mstepper.addStepper(stepper1);
  mstepper.addStepper(stepper2);
  AFMS.begin(); // Start the bottom shield

#ifdef USE_GCODE
  escher.push(_GCODE_POINTS[0].first, _GCODE_POINTS[0].second);
#else
  //grid(escher, 100, 10, 5);
  square(escher, 300);
#endif
}

void loop() {
  if (!escher.run()) {
    
#ifdef USE_GCODE
    curPoint++;
    if (curPoint < GCODE_NUM_POINTS) {
      Serial.printf("Escher is idle, pushing point %d/%d\n", curPoint+1, GCODE_NUM_POINTS);
      escher.push(_GCODE_POINTS[curPoint].first, _GCODE_POINTS[curPoint].second);
    }
#else
    square(escher, 300);
#endif
  }
}
