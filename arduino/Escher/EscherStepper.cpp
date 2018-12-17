#include "EscherStepper.h"

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>

#define MAX_SPEED 100.0
#define ACCELERATION 500.0

EscherStepper::EscherStepper(AccelStepper& xstepper, AccelStepper& ystepper) :
  _xstepper(xstepper), _ystepper(ystepper) {
  _xstepper.setMaxSpeed(MAX_SPEED);
  _xstepper.setAcceleration(ACCELERATION);
  _ystepper.setMaxSpeed(MAX_SPEED);
  _ystepper.setAcceleration(ACCELERATION);
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
    _xstepper.run();
    _ystepper.run();
  }
  _xstepper.setCurrentPosition(0);
  _ystepper.setCurrentPosition(0);
  Serial.printf("Home.\n");
}

void EscherStepper::moveTo(int x, int y) {
  Serial.printf("moveTo (%d, %d)\n", x, y);
  _xstepper.moveTo(x);
  _ystepper.moveTo(y);
}

void EscherStepper::push(int x, int y) {
  Serial.printf("push (%d, %d)\n", x, y);
  std::pair<int, int> coord(x, y);
  _pending.push_back(coord);
}

bool EscherStepper::run() {
  if (_xstepper.distanceToGo() == 0 && _ystepper.distanceToGo() == 0 &&
      !_pending.empty()) {
    std::pair<int, int> front = _pending.front();
    Serial.printf("Next waypoint: (%d, %d)\n", front.first, front.second);
    _xstepper.moveTo(front.first);
    _ystepper.moveTo(front.second);
    _pending.erase(_pending.begin());
  }

  _xstepper.run();
  _ystepper.run();

  if (_xstepper.distanceToGo() == 0 && _ystepper.distanceToGo() == 0 &&
      _pending.empty()) {
    return false;
  } else {
    return true;
  }
}
