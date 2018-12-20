/* Blinky - Team Sidney Enterprises
 * Author: Matt Welsh <mdw@mdw.la>
 * 
 * This sketch controls a Feather Huzzah32 board with an attached Neopixel or Dotstar LED strip.
 * It periodically checks in by writing a record to a Firebase database, and reads a config from
 * the database to control the LED pattern.
 */

#ifndef _BlinkyModes_h
#define _BlinkyModes_h

#include "Blinky.h"

class BlinkyMode {
public:
  // Run the mode.
  virtual void run() = 0;
  // Create the appropriate BlinkyMode object for the given config.
  static BlinkyMode* Create(const deviceConfig_t*);
};

class NoneMode : public BlinkyMode {
public:
  void run() { /* Do nothing. */ }
};

class WipeMode : public BlinkyMode {
public:
  WipeMode(const deviceConfig_t* config) : _color(config->color1), _speed(config->speed) {}
  void run();
private:
  uint32_t _color; 
  int _speed;
};

class TestMode : public BlinkyMode {
public:
  void run();
};

#if 0
class PixelMapper {
public:
  virtual uint32_t PixelColor(int index) = 0;
  void run();
};

class SingleColorMapper : public PixelMapper {
public:
  SingleColorMapper(uint32_t c) : _color(c) {}
  uint32_t PixelColor(int index) { return _color; }

private:
  uint32_t _color;
};

class MultiColorMapper : public PixelMapper {
public:
  MultiColorMapper(const uint32_t *colors, int numcolors)
    : _colors(colors), _numcolors(numcolors) {}
  uint32_t PixelColor(int index) { return _colors[index % _numcolors]; }
protected:
  const uint32_t *_colors;
  int _numcolors;
};

class RandomColorMapper : public MultiColorMapper {
public:
  RandomColorMapper(const uint32_t *colors, int numcolors)
    : MultiColorMapper(colors, numcolors) {}
  uint32_t PixelColor(int index) { return _colors[random(0, _numcolors-1)]; }
};

class Twinkler : public PixelMapper {
public:
  Twinkler(PixelMapper *mapper, int stepRange, float minBrightness,
      float maxBrightness)
    : _mapper(mapper), _stepRange(stepRange), _minBrightness(minBrightness),
      _maxBrightness(maxBrightness) {
    for (int i = 0; i < MAX_PIXELS; i++) {
      _brightness[i] = minBrightness + ((maxBrightness-minBrightness)/2.0);
    }
  }
  uint32_t PixelColor(int index);

private:
  PixelMapper *_mapper;
  int _stepRange;
  float _minBrightness;
  float _maxBrightness;
  float _brightness[MAX_PIXELS];
};
#endif // 0

#endif
