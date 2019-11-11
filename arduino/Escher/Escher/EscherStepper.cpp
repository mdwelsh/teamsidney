#include "EscherStepper.h"

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Adafruit_MotorShield.h>


EscherStepper::EscherStepper(
    MultiStepper &mstepper,
    long backlash_x,
    long backlash_y) :
  _mstepper(mstepper), _stopped(true),
  _backlash_x(backlash_x), _backlash_y(backlash_y),
  _cur_backlash_x(0), _cur_backlash_y(0),
  _last_x(0), _last_y(0),
  _dir_x(0), _dir_y(0),
  _last_push_x(0), _last_push_y(0) {}

void EscherStepper::clear() {
  _pending.clear();
}

void EscherStepper::moveTo(long x, long y) {

  // Figure out if we're reversing direction in x or y axes.
  long dirx = x - _last_x;
  if (dirx > 0) {
    dirx = 1;
  } else if (dirx < 0) {
    dirx = -1;
  }
  long diry = y - _last_y;
  if (diry > 0) {
    diry = 1;
  } else if (diry < 0) {
    diry = -1;
  }
  if (dirx != _dir_x) {
    _cur_backlash_x += (dirx * _backlash_x);
  }
  if (diry != _dir_y) {
    _cur_backlash_y += (diry * _backlash_y);
  }

  long target[2] = {x + _cur_backlash_x, y + _cur_backlash_y};

  _last_x = x;
  _last_y = y;

  // Don't want to add backlash when transitioning from x -> 0 -> x
  if (dirx != 0) {
    _dir_x = dirx;
  }
  if (diry != 0) {
    _dir_y = diry;
  }

  //Serial.printf("EscherStepper: moving to %d %d\n", target[0], target[1]);
  _mstepper.moveTo(target);
}

void EscherStepper::push(float x, float y) {
  // XXX MDW TODO: Offset adjustment and scaling if needed.
  long tx = (long)x;
  long ty = (long)y;
  // Skip duplicate points.
  if (tx == _last_push_x && ty != _last_push_y) {
    return;
  }
  std::pair<long, long> coord(tx, ty);
  _pending.push_back(coord);
  _last_push_x = tx;
  _last_push_y = ty;
}

void EscherStepper::pop() {
  _pending.pop_back();
}

// Returns true if any steppers still running.
// Returns false if all have stopped.
bool EscherStepper::run() {
  if (!_mstepper.run()) {
    //Serial.println("EscherStepper: Steppers have stopped");
    _stopped = true;
  } else {
    //Serial.println("EscherStepper: Steppers still running");
  }
  if (_stopped) {
    if (!_pending.empty()) {
      //Serial.println("EscherStepper: Got another waypoint");
      // Pick next waypoint.
      _stopped = false;
      std::pair<long, long> front = _pending.front();
      _pending.erase(_pending.begin());
      moveTo(front.first, front.second);
    }
  }
  //Serial.printf("EscherStepper.run() returning %s\n", (!_stopped)?"true":"false");
  return !_stopped;
}
