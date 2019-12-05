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

  // Push a new raw point.
  void push(float x, float y);
  // Remove the last raw point.
  void pop();
  // Commit the raw points to the stepper.
  void commit();
  // Run the stepper, returning true if any steppers are still running.
  bool run();
  // Reset the state of the stepper.
  void reset();
  // Set parameters.
  void setBacklashX(long backlash_x) { backlash_x_ = backlash_x; }
  void setBacklashY(long backlash_y) { backlash_y_ = backlash_y; }
  void setOffsetLeft(int offsetLeft) { offsetLeft_ = offsetLeft; }
  void setOffsetBottom(int offsetBottom) { offsetBottom_ = offsetBottom; }
  void setZoom(float zoom) { zoom_ = zoom; }
  void setScaleToFit(bool scaleToFit) { scaleToFit_ = scaleToFit; }

  private:
  void moveTo(long x, long y);

  MultiStepper& mstepper_;

  // Raw, unprocessed points from the parser.
  std::vector<std::pair<float, float>> raw_;

  // Pending points in stepper units.
  std::vector<std::pair<long, long>> pending_;

  bool committed_;
  bool stopped_;
  long backlash_x_, backlash_y_,
       cur_backlash_x_, cur_backlash_y_,
       last_x_, last_y_,
       dir_x_, dir_y_,
       last_push_x_, last_push_y_,
       offset_x_, offset_y_;
  float scale_;

  // User-supplied parameters.
  int offsetLeft_;
  int offsetBottom_;
  float zoom_;
  bool scaleToFit_;

};

#endif _ESCHERSTEPPER_H