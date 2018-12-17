#ifndef _ESCHERSTEPPER_H
#define _ESCHERSTEPPER_H

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>

class EscherStepper {
  public:
  EscherStepper(AccelStepper& xstepper, AccelStepper& ystepper);
  void moveTo(int x, int y);
  void push(int x, int y);
  bool run();
  void home();
  void clear();

  private:
  AccelStepper& _xstepper;
  AccelStepper& _ystepper;
  std::vector<std::pair<int, int>> _pending;
};


#endif _ESCHERSTEPPER_H
