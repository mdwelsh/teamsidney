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
  last_push_x_(0.0), last_push_y_(0.0),
  offsetLeft_(0), offsetBottom_(0), zoom_(1.0), scaleToFit_(false) {}

void EscherStepper::reset() {
  raw_.clear();
  pending_.clear();
  stopped_ = true;
  cur_backlash_x_ = 0;
  cur_backlash_y_ = 0;
  last_x_ = 0;
  last_y_ = 0;
  dir_x_ = 0;
  dir_y_ = 0;
  last_push_x_ = 0.0;
  last_push_y_ = 0.0;
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

  Serial.printf("EscherStepper: moving to %d %d\n", target[0], target[1]);
  mstepper_.moveTo(target);
}

void EscherStepper::push(float x, float y) {
  // Skip duplicate points.
  if (x == last_push_x_ && y != last_push_y_) {
    return;
  }
  std::pair<float, float> coord(x, y);
  raw_.push_back(coord);
  last_push_x_ = x;
  last_push_y_ = y;
}

void EscherStepper::pop() {
  raw_.pop_back();
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
      //Serial.println("EscherStepper: Got another waypoint");
      // Pick next waypoint.
      stopped_ = false;
      std::pair<long, long> front = pending_.front();
      pending_.erase(pending_.begin());
      moveTo(front.first, front.second);
    }
  }
  //Serial.printf("EscherStepper.run() returning %s\n", (!_stopped)?"true":"false");
  return !stopped_;
}

// Determine offset_x_, offset_y_, and scale_ based on gcode points.
void EscherStepper::scaleToFit() {
  // First, get bounding dimensions of the waypoints.
  float minx = FLT_MAX;
  float maxx = 0.0;
  float miny = FLT_MAX;
  float maxy = 0.0;
  for (auto it = std::begin(raw_); it != std::end(raw_); ++it) {
    float x = it->first;
    float y = it->second;
    if (x < minx) {
      minx = x;
    }
    if (x > maxx) {
      maxx = x;
    }
    if (y < miny) {
      miny = y;
    }
    if (y > maxy) {
      maxy = y;
    }
  }
  // Translate gCode object to lower left corner.
  for (auto it = begin(raw_); it != end(raw_); ++it) {
    it->first -= minx;
    it->second -= miny;
  }
  // Calculate scaling factor and offsets.
  float aspect_ratio = (etch_width_ * 1.0) / (etch_height_ * 1.0);
  float dx = maxx - minx;
  float dy = maxy - miny;
  if ((dx / aspect_ratio) > dy) {
    // The object is wider than it is tall.
    scale_ = etch_width_ / dx;
    offset_x_ = 0.0;
    offset_y_ = (etch_height_ - (scale_ * dy)) / 2.0;
  } else {
    // The object is taller than it is wide.
    scale_ = etch_height_ / dy;
    offset_x_ = (etch_width_ - (scale_ * dx)) / 2.0;
    offset_y_ = 0.0;
  }
  Serial.printf("scaleToFit: setting scale_ %f offset_x %f offset_y %f\n", scale_, offset_x_, offset_y_);
}

// Convert the raw_ points to stepper coordinates in pending_.
void EscherStepper::commit() {
  Serial.printf("commit: zoom_ %f offsetLeft_ %d offsetBottom_ %d\n", zoom_, offsetLeft_, offsetBottom_);
  offset_x_ = 0;
  offset_y_ = 0;
  scale_ = 1.0;
  if (scaleToFit_) {
    scaleToFit();
  }
  while (!raw_.empty()) {
    std::pair<float, float> front = raw_.front();
    raw_.erase(raw_.begin());
    float x = front.first;
    float y = front.second;
    long tx, ty;
    tx = (zoom_ * scale_ * x) + offsetLeft_ + offset_x_;
    ty = (zoom_ * scale_ * y) + offsetBottom_ + offset_y_;
    std::pair<float, float> coord(tx, ty);
    pending_.push_back(coord);
  }
}