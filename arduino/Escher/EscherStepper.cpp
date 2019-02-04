#include "EscherStepper.h"

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Adafruit_MotorShield.h>

// In practice, these should probably be calibrated on a per-Etch-a-Sketch basis.
#define BACKLASH_X 10
#define BACKLASH_Y 15

EscherStepper::EscherStepper(MultiStepper &mstepper) :
  _mstepper(mstepper), _stopped(true),
  _backlash_x(0), _backlash_y(0),
  _last_x(0), _last_y(0),
  _dir_x(0), _dir_y(0) {}

void EscherStepper::clear() {
  _pending.clear();
}

void EscherStepper::moveTo(long x, long y) {

#ifndef BACKLASH_X
  Serial.printf("moveTo (%d %d)\n", x, y);
  long target[2] = {x + _backlash_x, y + _backlash_y};
  _mstepper.moveTo(target);

#else
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
    _backlash_x += (dirx * BACKLASH_X);
  }
  if (diry != _dir_y) {
    _backlash_y += (diry * BACKLASH_Y);
  }

  long target[2] = {x + _backlash_x, y + _backlash_y};

  Serial.printf("MDW %d %d %d %d\n", x, y, target[0], target[1]);
  Serial.printf("moveTo (%d %d) target (%d %d) last (%d %d) dir (%d %d) lastdir (%d %d) bl (%d %d)\n",
                x, y, target[0], target[1], _last_x, _last_y, dirx, diry, _dir_x, _dir_y, _backlash_x, _backlash_y);

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
