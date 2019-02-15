#ifndef _ESCHERSTEPPER_H
#define _ESCHERSTEPPER_H

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Adafruit_MotorShield.h>

class EscherStepper {
  public:
  EscherStepper(MultiStepper& mstepper, long backlash_x, long backlash_y);
  void moveTo(long x, long y);
  void push(long x, long y);
  bool run();
  void clear();

  private:
  //AccelStepper& _xstepper;
  //AccelStepper& _ystepper;
  MultiStepper& _mstepper;
  std::vector<std::pair<long, long>> _pending;
  bool _stopped;
  long _backlash_x, _backlash_y,
       _cur_backlash_x, _cur_backlash_y,
       _last_x, _last_y, _dir_x, _dir_y;
};


#endif _ESCHERSTEPPER_H
