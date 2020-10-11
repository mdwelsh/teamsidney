/* Blinky - Team Sidney Enterprises
 * Author: Matt Welsh <mdw@mdw.la>
 * 
 */

#ifndef _MercatorModes_h
#define _MercatorModes_h

#include "Mercator.h"

class MercatorMode {
public:
  // Run the mode.
  virtual void run() = 0;
  // Create the appropriate MercatorMode object for the given config.
  static MercatorMode* Create(const deviceConfig_t*);
};

class NoneMode : public MercatorMode {
public:
  void run() { /* Do nothing. */ }
};

class ColorChangingMode : public MercatorMode {
public:
  ColorChangingMode(const deviceConfig_t *config)
    : _brightness(config->brightness),
      _color1(config->color1), _color2(config->color2),
      _speed(config->speed), _colorChange(config->colorChange),
      _wheel1(0), _wheel2(128) {}
  void run();
protected:
  uint32_t _color1, _color2;
  int _speed, _colorChange, _brightness;
private:
  int _wheel1, _wheel2;
};

class WipeMode : public ColorChangingMode {
public:
  WipeMode(const deviceConfig_t* config) : ColorChangingMode(config), _speed(config->speed) {}
  void run();
private:
  int _speed;
};

class TestMode : public MercatorMode {
public:
  void run();
};

class RotatingMode : public MercatorMode {
  public:
  RotatingMode(MercatorMode** modes, int numModes, int rotateTime) 
    : modes_(modes), numModes_(numModes), rotateTime_(rotateTime), lastSwitch_(0), curIndex_(0) {}
  void run();
  private:
    MercatorMode** modes_;
    int numModes_;
    int rotateTime_;
    int curIndex_;
    unsigned long lastSwitch_;
};

class PixelMapper : public ColorChangingMode {
public:
  PixelMapper(const deviceConfig_t* config) : ColorChangingMode(config) {}
  virtual uint32_t PixelColor(int index) = 0;
  void run();
};

class SingleColorMapper : public PixelMapper {
public:
  SingleColorMapper(const deviceConfig_t* config) : PixelMapper(config) {}
  uint32_t PixelColor(int index) { return _color1; }
};

class MultiColorMapper : public PixelMapper {
public:
  MultiColorMapper(const deviceConfig_t* config, const uint32_t *colors, int numcolors)
    : PixelMapper(config), _colors(colors), _numcolors(numcolors) {}
  uint32_t PixelColor(int index) { return _colors[index % _numcolors]; }
protected:
  const uint32_t *_colors;
  int _numcolors;
};

class RandomColorMapper : public MultiColorMapper {
public:
  RandomColorMapper(const deviceConfig_t* config, const uint32_t *colors, int numcolors)
    : MultiColorMapper(config, colors, numcolors) {}
  uint32_t PixelColor(int index) { return _colors[random(0, _numcolors)]; }
};

class Rainbow : public PixelMapper {
public:
  Rainbow(const deviceConfig_t *config) : PixelMapper(config), _wheel(0) {}
  uint32_t PixelColor(int index) {
    if (index == 0) {
      _wheel++;
      _wheel = _wheel % 256;
    }
    return Wheel((index + _wheel) & 255);
  }
private:
  int _wheel;
};

class Twinkler : public PixelMapper {
public:
  Twinkler(const deviceConfig_t* config, PixelMapper *mapper, int stepRange, float minBrightness,
      float maxBrightness)
    : PixelMapper(config), _mapper(mapper), _stepRange(stepRange), _minBrightness(minBrightness),
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

class Rain : public PixelMapper {
public:
  Rain(const deviceConfig_t* config, PixelMapper *mapper, int maxdrops, float initValue, float maxValue, float minValue,
       float growSpeed, float fadeSpeed, float fadeProb, bool multi, bool randInit)
    : PixelMapper(config), _mapper(mapper),
      _numActive(0), _maxDrops(maxdrops), _initValue(initValue), _maxValue(maxValue), _minValue(minValue),
      _growSpeed(growSpeed), _fadeSpeed(fadeSpeed), _fadeProb(fadeProb), _multi(multi), _randInit(randInit) {
    for (int i = 0; i < MAX_PIXELS; i++) {
      _state[i].value = 0.0;
      _state[i].growing = false;
    }
  }
  uint32_t PixelColor(int index);
  
private:
  PixelMapper *_mapper;
  struct {
    uint32_t color;
    float value;
    bool growing;
  } _state[MAX_PIXELS];
  int _maxDrops, _numActive;
  float _initValue, _maxValue, _minValue, _growSpeed, _fadeSpeed, _fadeProb;
  bool _multi, _randInit;
};


class Comet : public MercatorMode {
  public:
  Comet(uint32_t color, int tail, int wait) : color_(color), tail_(tail), wait_(wait) {}
  void run();
  private:
  uint32_t color_;
  int tail_;
  int wait_;
};

class DoubleWipe : public MercatorMode {
  public:
  DoubleWipe(uint32_t color1, uint32_t color2, int wait) : color1_(color1), color2_(color2), wait_(wait) {}
  void run();
  private:
  uint32_t color1_, color2_;
  int wait_;
};

class Pulse : public MercatorMode {
  public:
  Pulse(uint32_t color1, uint32_t color2, int wait) : color1_(color1), color2_(color2), wait_(wait) {}
  void run();
  private:
  uint32_t color1_, color2_;
  int wait_;
};

class LongitudinalMode : public MercatorMode {
public:
  LongitudinalMode(const deviceConfig_t *config)
    : _brightness(config->brightness),
      _color1(config->color1), _color2(config->color2),
      _speed(config->speed) {}
  void run();
protected:
  uint32_t _color1, _color2;
  int _speed, _brightness;
};

#endif
