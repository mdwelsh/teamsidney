#ifndef _ESCHERSTEPPER_H
#define _ESCHERSTEPPER_H

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Adafruit_MotorShield.h>

class EscherStepper {
  public:
  EscherStepper(MultiStepper& mstepper, long etch_width, long etch_height, long backlash_x, long backlash_y);

  // Prepare for accepting data by computing scale factors based on the
  // current parameters.
  void computeScaleFactors();
  // Push a new point in gCode units.
  void push(float x, float y);
  // Remove the last point.
  void pop();
  // Run the stepper, returning true if any steppers are still running.
  bool run();
  // Reset the state of the stepper.
  void reset();
  // Set parameters.
  void setMinX(float minx) { minx_ = minx; }
  void setMaxX(float maxx) { maxx_ = maxx; }
  void setMinY(float miny) { miny_ = miny; }
  void setMaxY(float maxy) { maxy_ = maxy; }
  void setEtchWidth(long etch_width) { etch_width_ = etch_width; }
  void setEtchHeight(long etch_height) { etch_height_ = etch_height; }
  void setBacklashX(long backlash_x) { backlash_x_ = backlash_x; }
  void setBacklashY(long backlash_y) { backlash_y_ = backlash_y; }
  void setOffsetLeft(long offsetLeft) { offsetLeft_ = offsetLeft; }
  void setOffsetBottom(long offsetBottom) { offsetBottom_ = offsetBottom; }
  void setZoom(float zoom) { zoom_ = zoom; }
  void setScaleToFit(bool scaleToFit) { scaleToFit_ = scaleToFit; }

  private:
  void moveTo(long x, long y);
  void scaleToFit();

  MultiStepper& mstepper_;

  // Pending points in stepper units.
  std::vector<std::pair<long, long>> pending_;

  bool committed_;
  bool stopped_;
  long etch_width_, etch_height_,
       backlash_x_, backlash_y_,
       cur_backlash_x_, cur_backlash_y_,
       last_x_, last_y_,
       dir_x_, dir_y_,
       offset_x_, offset_y_,
       last_push_x_, last_push_y_;
  float minx_, miny_, maxx_, maxy_, scale_;

  // User-supplied parameters.
  long offsetLeft_;
  long offsetBottom_;
  float zoom_;
  bool scaleToFit_;

};

#endif _ESCHERSTEPPER_H