#ifndef _ESCHERSTEPPER_H
#define _ESCHERSTEPPER_H

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>

class EscherStepper {
  public:
  EscherStepper(AccelStepper& xstepper, AccelStepper& ystepper);
  void moveTo(long x, long y);
  void push(long x, long y);
  bool run();
  void home();
  void clear();

  private:
  AccelStepper& _xstepper;
  AccelStepper& _ystepper;
  std::vector<std::pair<long, long>> _pending;
  bool _stopped;
};


#endif _ESCHERSTEPPER_H
