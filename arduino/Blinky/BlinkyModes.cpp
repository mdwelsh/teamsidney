#include "Blinky.h"
#include "BlinkyModes.h"

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
  printf("Warning: Create got unknown mode %s\n", config->mode);
  return new NoneMode();
}

void WipeMode::run() {
  Serial.println("WipeMode::run called");
  colorWipe(_color, _speed);
}

void TestMode::run() {
  Serial.println("TestMode::run() called");
  strip->setBrightness(50);
  Serial.println("Doing colorWipe1...");
  colorWipe(0xff0000, 5);
  Serial.println("Doing colorWipe2...");
  colorWipe(0x00ff00, 5);
  Serial.println("Doing colorWipe3...");
  colorWipe(0x0000ff, 5);
  Serial.println("Doing colorWipe4...");
  colorWipe(0, 5);
}

#if 0
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
#endif // 0
