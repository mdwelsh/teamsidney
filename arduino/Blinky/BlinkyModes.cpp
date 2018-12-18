#include "BlinkyModes.h"

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
