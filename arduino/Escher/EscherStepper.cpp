#include "EscherStepper.h"

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Adafruit_MotorShield.h>

EscherStepper::EscherStepper(MultiStepper &mstepper) :
  _mstepper(mstepper), _stopped(true) {}

void EscherStepper::clear() {
  _pending.clear();
}

void EscherStepper::moveTo(long x, long y) {
  //Serial.printf("moveTo %d %d\n", x, y);
  long target[2] = {x, y};
  _mstepper.moveTo(target);
}

void EscherStepper::push(long x, long y) {
  //Serial.printf("push %d %d\n", x, y);
  std::pair<long, long> coord(x, y);
  _pending.push_back(coord);
}

// Returns true if any steppers still running.
// Returns false if all have stopped.
bool EscherStepper::run() {
  if (!_mstepper.run()) {
    _stopped = true;
  }
  if (_stopped) {
    if (!_pending.empty()) {
      // Pick next waypoint.
      _stopped = false;
      std::pair<long, long> front = _pending.front();
      _pending.erase(_pending.begin());
      moveTo(front.first, front.second);
    }
  }
  return !_stopped;
}

#if 0
bool EscherStepper::run() {
  //Serial.printf("ESCHER %d %d\n", _xstepper.currentPosition(), _ystepper.currentPosition());
  if (_xstepper.distanceToGo() <= 0 && _ystepper.distanceToGo() <= 0) {
    if (!_pending.empty()) {
      // Pick next waypoint.
      _stopped = false;
      std::pair<long, long> front = _pending.front();
      _pending.erase(_pending.begin());
      //Serial.printf("Next waypoint: (%d, %d)\n", front.first, front.second);

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

      //Serial.printf("Moving to %d %d dist %f %f speed %f %f\n", tgtx, tgty, distx, disty, xspeed, yspeed);
      _xstepper.moveTo(tgtx);
      _ystepper.moveTo(tgty);
      _xstepper.setSpeed(abs(xspeed));
      _ystepper.setSpeed(abs(yspeed));
    } else {
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
      //Serial.printf("ESCHER %d %d\n", _xstepper.currentPosition(), _ystepper.currentPosition());
    }
    return true;
  } else {
    return false;
  }
}
#endif
