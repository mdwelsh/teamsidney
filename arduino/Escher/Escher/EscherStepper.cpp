#include "EscherStepper.h"

#include <float.h>
#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Adafruit_MotorShield.h>


EscherStepper::EscherStepper(
    MultiStepper &mstepper,
    long etch_width,
    long etch_height,
    long backlash_x,
    long backlash_y) :
  mstepper_(mstepper), stopped_(true),
  etch_width_(etch_width), etch_height_(etch_height),
  backlash_x_(backlash_x), backlash_y_(backlash_y),
  cur_backlash_x_(0), cur_backlash_y_(0),
  last_x_(0), last_y_(0),
  dir_x_(0), dir_y_(0),
  last_push_x_(0), last_push_y_(0),
  offsetLeft_(0), offsetBottom_(0), zoom_(1.0), scaleToFit_(false) {}

void EscherStepper::reset() {
  pending_.clear();
  stopped_ = true;
  cur_backlash_x_ = 0;
  cur_backlash_y_ = 0;
  last_x_ = 0;
  last_y_ = 0;
  dir_x_ = 0;
  dir_y_ = 0;
  last_push_x_ = 0;
  last_push_y_ = 0;
  offsetLeft_ = 0;
  offsetBottom_ = 0;
  zoom_ = 1.0;
  scaleToFit_ = false;
}

// Move to the given position, specified in stepper units.
void EscherStepper::moveTo(long x, long y) {
  // Figure out if we're reversing direction in x or y axes.
  long dirx = x - last_x_;
  if (dirx > 0) {
    dirx = 1;
  } else if (dirx < 0) {
    dirx = -1;
  }
  long diry = y - last_y_;
  if (diry > 0) {
    diry = 1;
  } else if (diry < 0) {
    diry = -1;
  }
  if (dirx != dir_x_) {
    cur_backlash_x_ += (dirx * backlash_x_);
  }
  if (diry != dir_y_) {
    cur_backlash_y_ += (diry * backlash_y_);
  }
  long target[2] = {x + cur_backlash_x_, y + cur_backlash_y_};

  last_x_ = x;
  last_y_ = y;

  // Don't want to add backlash when transitioning from x -> 0 -> x
  if (dirx != 0) {
    dir_x_ = dirx;
  }
  if (diry != 0) {
    dir_y_ = diry;
  }
  mstepper_.moveTo(target);
}

void EscherStepper::push(float x, float y) {
  // Convert from gCode units to stepper units by applying scale and
  // translation factors.
  float tx = (zoom_ * scale_ * x) + (offset_x_ + offsetLeft_);
  float ty = (zoom_ * scale_ * y) + (offset_y_ + offsetBottom_);
  long ltx = floor(tx);
  long lty = floor(ty);

  // Skip duplicate points.
  if (tx == last_push_x_ && ty == last_push_y_) {
    return;
  }
  std::pair<long, long> coord(ltx, lty);
  pending_.push_back(coord);
  last_push_x_ = ltx;
  last_push_y_ = lty;
}

void EscherStepper::pop() {
  pending_.pop_back();
}

// Returns true if any steppers still running.
// Returns false if all have stopped.
bool EscherStepper::run() {
  if (!mstepper_.run()) {
    //Serial.println("EscherStepper: Steppers have stopped");
    stopped_ = true;
  } else {
    //Serial.println("EscherStepper: Steppers still running");
  }
  if (stopped_) {
    if (!pending_.empty()) {
      stopped_ = false;
      std::pair<long, long> front = pending_.front();
      pending_.erase(pending_.begin());
      moveTo(front.first, front.second);
    }
  }
  return !stopped_;
}

void EscherStepper::computeScaleFactors() {
  offset_x_ = 0;
  offset_y_ = 0;
  scale_ = 1.0;

  if (scaleToFit_) {
    // Calculate scaling factor and offsets.
    float aspect_ratio = (etch_width_ * 1.0) / (etch_height_ * 1.0);
    Serial.printf("scaleToFit: width %d height %d ratio %f\n", etch_width_, etch_height_, aspect_ratio);
    Serial.printf("scaleToFit: minx_ %f maxx_ %f miny_ %f maxy_ %f\n", minx_, maxx_, miny_, maxy_);
    float dx = maxx_ - minx_;
    float dy = maxy_ - miny_;
    Serial.printf("scaleToFit: dx %f dy %f\n", dx, dy);
    Serial.printf("scaleToFit: dx/ar %f dy %f\n", (dx/aspect_ratio), dy);
    if ((dx / aspect_ratio) > dy) {
      // The object is wider than it is tall.
      Serial.printf("scaleToFit: wider case\n");
      scale_ = etch_width_ / dx;
      offset_x_ = 0;
      offset_y_ = floor((etch_height_ - (scale_ * dy)) / 2.0);
    } else {
      // The object is taller than it is wide.
      Serial.printf("scaleToFit: taller case\n");
      scale_ = etch_height_ / dy;
      offset_x_ = floor((etch_width_ - (scale_ * dx)) / 2.0);
      offset_y_ = 0;
    }
    Serial.printf("computeScaleFactors: scale_ %f offset_x %d offset_y %d\n", scale_, offset_x_, offset_y_);
    // This ensures that the offsets are with respect to the gCode object
    // having its origin point at the lower left corner.
    offset_x_ -= (minx_ * scale_ * zoom_);
    offset_y_ -= (miny_ * scale_ * zoom_);
  }
  Serial.printf("computeScaleFactors: final scale_ %f offset_x %d offset_y %d\n", scale_, offset_x_, offset_y_);
}