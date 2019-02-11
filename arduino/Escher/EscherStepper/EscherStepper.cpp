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
  _dir_x(0), _dir_y(0) {}

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

  _mstepper.moveTo(target);
#endif
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
