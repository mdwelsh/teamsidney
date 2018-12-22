#include "Blinky.h"
#include "BlinkyModes.h"

#define NUM_CHRISTMAS_COLORS 10
const uint32_t CHRISTMAS_COLORS[] = {
  0x0,
  0xff0000,
  0x0,
  0x00ff00,
  0x0,
  0x0000ff,
  0x0,
  0xff00ff,
  0x0,
  0xffac00,
};

BlinkyMode* BlinkyMode::Create(const deviceConfig_t *config) {
  if (!strcmp(config->mode, "") || !strcmp(config->mode, "off") || !strcmp(config->mode, "none")) {
    Serial.println("Creating NoneMode");
    return new NoneMode();
  }
  if (!strcmp(config->mode, "wipe")) {
    Serial.println("Creating WipeMode");
    return new WipeMode(config);
  }
  if (!strcmp(config->mode, "test")) {
    Serial.println("Creating TestMode");
    return new TestMode();
  }
  if (!strcmp(config->mode, "single")) {
    return new SingleColorMapper(config);
  }
  if (!strcmp(config->mode, "double")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    return new MultiColorMapper(config, colors, 2);
  }
  if (!strcmp(config->mode, "random")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    return new RandomColorMapper(config, colors, 2);
  }
  if (!strcmp(config->mode, "rainbow")) {
    return new Rainbow(config);
  }
  if (!strcmp(config->mode, "rain")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    PixelMapper *m = new RandomColorMapper(config, colors, 2);
    return new Rain(config, m, strip->numPixels(), 1.0, 1.0, 0.0, 0.05, 0.05, 1.0, false, false);
  }
  if (!strcmp(config->mode, "snow")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    PixelMapper *m = new RandomColorMapper(config, colors, 2);
    return new Rain(config, m, strip->numPixels()*2, 0.02, 1.0, 0.0, 0.1, 0.02, 0.5, false, false);
  }
  if (!strcmp(config->mode, "sparkle")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    PixelMapper *m = new RandomColorMapper(config, colors, 2);
    return new Rain(config, m, strip->numPixels(), 1.0, 1.0, 0.0, 0, 0.4, 1.0, false, false);
  }
  if (!strcmp(config->mode, "shimmer")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    PixelMapper *m = new RandomColorMapper(config, colors, 2);
    return new Rain(config, m, 10, 0.1, 1.0, 0.0, 0.2, 0.05, 1.0, true, false);
  }
  if (!strcmp(config->mode, "twinkle")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    PixelMapper *m = new RandomColorMapper(config, colors, 2);
    return new Rain(config, m, strip->numPixels(), 0.7, 0.8, 0.6, 0.1, 0.1, 0.1, false, true);
  }
  if (!strcmp(config->mode, "fade")) {
    uint32_t *colors = (uint32_t *)malloc(2 * sizeof(uint32_t));
    colors[0] = config->color1;
    colors[1] = config->color2;
    PixelMapper *m = new RandomColorMapper(config, colors, 2);
    return new Rain(config, m, strip->numPixels(), 0.1, 1.0, 0.1, 0.1, 0.01, 0.1, false, true);
  }
  // Kind of a hack to allow Google Assistant to set the value to the capitalized version.
  if (!strcmp(config->mode, "christmas") || !strcmp(config->mode, "Christmas")) {
    PixelMapper *m = new MultiColorMapper(config, CHRISTMAS_COLORS, NUM_CHRISTMAS_COLORS);
    return new Rain(config, m, strip->numPixels(), 0.7, 0.8, 0.6, 0.1, 0.1, 0.1, false, true);
  }
  
  printf("Warning: Create got unknown mode %s\n", config->mode);
  return new NoneMode();
}

void ColorChangingMode::run() {
  strip->setBrightness(_brightness);
  if (_colorChange > 0) {
    _wheel1 += _colorChange;
    _wheel1 = _wheel1 % 255;
    _color1 = Wheel(_wheel1);
    if (_color2 != 0) {
      _wheel2 += _colorChange;
      _wheel2 = _wheel2 % 255;
      _color2 = Wheel(_wheel2);
    }
  }
}

void WipeMode::run() {
  Serial.println("WipeMode::run called");
  colorWipe(_color1, _speed);
  ColorChangingMode::run();
}

void TestMode::run() {
  strip->setBrightness(50);
  colorWipe(0xff0000, 5);
  colorWipe(0x00ff00, 5);
  colorWipe(0x0000ff, 5);
  colorWipe(0, 5);
}

void PixelMapper::run() {
  for (int i = 0; i < strip->numPixels(); i++) {
    uint32_t c = PixelColor(i);
    setPixel(i, c);
  }
  strip->show();
  ColorChangingMode::run();
  delay(_speed);
}

uint32_t Twinkler::PixelColor(int index) {
  uint32_t color = _mapper->PixelColor(index);
  float b = _brightness[index];
  float step = random(0, _stepRange) / 100.0;
  if (random(0, 10) < 5) {
    b += step;
  } else {
    b -= step;
  }
  if (b < _minBrightness) b = _minBrightness;
  if (b > _maxBrightness) b = _maxBrightness;
  _brightness[index] = b;
  color = interpolate(0, color, b);
  return color;
}

uint32_t Rain::PixelColor(int index) {
  // See if we need to fire up more pixels.
  if (index == 0) {
    int numActive = 0;
    for (int i = 0; i < strip->numPixels(); i++) {
      if (_state[i].growing) {
        numActive++;
      }
    }
    if (_multi) {
      if (numActive == 0) {
        // New pixels start all at once
        for (int i = 0; i < _maxDrops; i++) {
          int p = random(0, strip->numPixels());
          if (_state[p].value <= _minValue) {
            _state[p].color = _mapper->PixelColor(p);
            if (_randInit) {
              _state[p].value = random(_minValue * 100, _maxValue * 100) / 100.0;
              _state[p].growing = (random(0, 100) < 50) ? true : false;
            } else {
              _state[p].value = _initValue;
              _state[p].growing = true;
            }
          }
        }
      }
    } else {
      // Randomly start new pixels.
      if (random(0, strip->numPixels()) <= _maxDrops) {
        int p = random(0, strip->numPixels());
        if (_state[p].value <= _minValue) {
          _state[p].color = _mapper->PixelColor(p);
          if (_randInit) {
            _state[p].value = random(_minValue * 100, _maxValue * 100) / 100.0;
            _state[p].growing = (random(0, 100) < 50) ? true : false;
          } else {
            _state[p].value = _initValue;
            _state[p].growing = true;
          }
        }
      }
    }
  } // Only for cycle init.

  float val = _state[index].value;
  // Skip pixels that have not been initialized.
  if (val == 0.0) {
    return 0;
  }
  
  if (val <= _minValue) {
    val = _minValue;
  }
  if (val >= _maxValue) {
    val = _maxValue;
  }

  uint32_t tc = interpolate(0, _state[index].color, val);
    
  if (_state[index].value >= _maxValue) {
    _state[index].growing = false;
  }
  // Grow or fade.
  if (_state[index].value >= _minValue && random(0, 100) <= (int)(100.0*_fadeProb)) {
    if (_state[index].growing) {
      _state[index].value += _growSpeed;
    } else {
      _state[index].value -= _fadeSpeed;
    }
  }
  return tc;
  
}
