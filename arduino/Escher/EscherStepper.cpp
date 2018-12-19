#include "EscherStepper.h"

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>

#define MAX_SPEED 500.0

EscherStepper::EscherStepper(AccelStepper& xstepper, AccelStepper& ystepper) :
  _xstepper(xstepper), _ystepper(ystepper), _stopped(true) {
    /*
  _xstepper.setMaxSpeed(MAX_SPEED);
  _xstepper.setAcceleration(ACCELERATION);
  _ystepper.setMaxSpeed(MAX_SPEED);
  _ystepper.setAcceleration(ACCELERATION);
  */
}

void EscherStepper::clear() {
  _pending.clear();
}

void EscherStepper::home() {
  Serial.printf("Waiting for completion of current run...\n");
  _xstepper.runToPosition();
  _ystepper.runToPosition();
  clear();
  moveTo(-2000, -2000);
  Serial.printf("Going home...\n");
  while (_xstepper.distanceToGo() != 0 && _ystepper.distanceToGo() != 0) {
    _xstepper.runSpeed();
    _ystepper.runSpeed();
  }
  _xstepper.setCurrentPosition(0);
  _ystepper.setCurrentPosition(0);
  Serial.printf("Home.\n");
}

void EscherStepper::moveTo(long x, long y) {
  Serial.printf("moveTo (%d, %d)\n", x, y);
  _xstepper.moveTo(x);
  _ystepper.moveTo(y);
}

void EscherStepper::push(long x, long y) {
  Serial.printf("push (%d, %d)\n", x, y);
  std::pair<long, long> coord(x, y);
  _pending.push_back(coord);
}

bool EscherStepper::run() {
  //Serial.printf("ESCHER %d %d\n", _xstepper.currentPosition(), _ystepper.currentPosition());
  if (_xstepper.distanceToGo() == 0 && _ystepper.distanceToGo() == 0) {
    if (!_pending.empty()) {
      // Pick next waypoint.
      _stopped = false;
      std::pair<long, long> front = _pending.front();
      _pending.erase(_pending.begin());
      Serial.printf("Next waypoint: (%d, %d)\n", front.first, front.second);

      // Calculate speed for each stepper to ensure linear path.
      long curx = _xstepper.currentPosition();
      long cury = _ystepper.currentPosition();
      long tgtx = front.first;
      long tgty = front.second;
      float distx = tgtx - curx;
      float disty = tgty - cury;
      float xspeed;
      float yspeed;
      
      if (abs(distx) >= abs(disty)) {
        if (distx == 0) {
          xspeed = 0;
          yspeed = 0; // We know this since abs(distx) >= abs(disty)
        } else {
          xspeed = MAX_SPEED * (distx / abs(distx)); // Just being clever to get the sign right.
          yspeed = MAX_SPEED * (disty / abs(distx));  
        }
      } else {
        if (disty == 0) {
          xspeed = 0; // We know this since abs(disty) > abs(distx)
          yspeed = 0; 
        } else {
          xspeed = MAX_SPEED * (distx / abs(disty));
          yspeed = MAX_SPEED * (disty / abs(disty));  
        } 
      }

      Serial.printf("Moving to %d %d dist %f %f speed %f %f\n", tgtx, tgty, distx, disty, xspeed, yspeed);
      _xstepper.moveTo(tgtx);
      _ystepper.moveTo(tgty);
      _xstepper.setSpeed(xspeed);
      _ystepper.setSpeed(yspeed);
    } else {
      Serial.printf("Nothing to do, stopping.\n");
      // Nothing to do.
      _stopped = true;
      _xstepper.setSpeed(0);
      _ystepper.setSpeed(0);
    }
  }

  if (!_stopped) {
    bool movex = _xstepper.runSpeed();
    bool movey = _ystepper.runSpeed();
    if (movex || movey) {
      Serial.printf("ESCHER %d %d\n", _xstepper.currentPosition(), _ystepper.currentPosition());
    }
    return true;
  } else {
    return false;
  }
}
