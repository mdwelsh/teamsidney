/* Blinky - Team Sidney Enterprises
 * Author: Matt Welsh <mdw@mdw.la>
 * 
 * This sketch controls a Feather Huzzah32 board with an attached Neopixel or Dotstar LED strip.
 * It periodically checks in by writing a record to a Firebase database, and reads a config from
 * the database to control the LED pattern.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/task.h>
#include <Update.h>

//#define USE_NEOPIXEL
#define DOTSTAR_MATRIX

#ifdef USE_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#else
#include <Adafruit_DotStar.h>
#endif

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define USE_SERIAL Serial

#ifdef USE_NEOPIXEL
#define DEVICE_TYPE "Neopixel"
#else
#define DEVICE_TYPE "Dotstar"
#endif

#ifdef DOTSTAR_MATRIX
#define DEVICE_FORMFACTOR "matrix"
#else
#define DEVICE_FORMFACTOR "strip"
#endif

// We use some magic strings in this constant to ensure that we can easily strip it out of the binary.
const char BUILD_VERSION[] = ("__Bl!nky__ " __DATE__ " " __TIME__ " " DEVICE_TYPE " " DEVICE_FORMFACTOR " ___");

// Start with this many pixels, but can be reconfigured.
#define NUMPIXELS 72
// Maximum number, for the sake of maintaining state.
#define MAX_PIXELS 200
#define NEOPIXEL_DATA_PIN 14

#ifdef DOTSTAR_MATRIX
// Settings for matrix board.
#define DOTSTAR_DATA_PIN 27
#define DOTSTAR_CLOCK_PIN 13
#else
// Settings for strip with Team Sidney connector PCB.
#define DOTSTAR_DATA_PIN 14
#define DOTSTAR_CLOCK_PIN 32
#endif

#ifdef USE_NEOPIXEL
Adafruit_NeoPixel *strip = NULL;
#define DEFAULT_DATA_PIN NEOPIXEL_DATA_PIN
#define DEFAULT_CLOCK_PIN 0 // Unused.
#else
Adafruit_DotStar *strip = NULL;
#define DEFAULT_DATA_PIN DOTSTAR_DATA_PIN
#define DEFAULT_CLOCK_PIN DOTSTAR_CLOCK_PIN
#endif

WiFiMulti wifiMulti;
HTTPClient http;

StaticJsonDocument<512> curConfigDocument;
// Maximum length of configMode string.
#define MAX_MODE_LEN 16

// Copy of the current configuration.
String configMode = "test";
bool configEnabled = true;
int configNumPixels = NUMPIXELS;
int configDataPin = DEFAULT_DATA_PIN;
int configClockPin = DEFAULT_CLOCK_PIN;
int configColorChange = 0;
int configBrightness = 100;
int configSpeed = 100;
int configRed = 100;
int configBlue = 100;
int configGreen = 0;
String configFirmwareVersion = "";

// Next firmware URL and hash, for OTA update.
StaticJsonDocument<512> firmwareVersionDocument;
String firmwareUrl = "";
String firmwareHash = "";

// Set of modes to select from in "random" mode.
#define NUM_RANDOM_MODES 7
const String RANDOM_MODES[] = {
  "wipe",
  "theatre",
  "rainbow",
  "bounce",
  "strobe",
  "rain",
  "comet",
};

SemaphoreHandle_t configMutex = NULL;
void TaskFlash(void *);
void TaskCheckin(void *);
void TaskRunConfig(void *);

void makeNewStrip(int numPixels, int dataPin, int clockPin) {
  USE_SERIAL.printf("Making new strip with %d pixels\n", numPixels);
  if (strip != NULL) {
    black();
    delete strip;
  }
#ifdef USE_NEOPIXEL
  strip = new Adafruit_NeoPixel(numPixels, dataPin, NEO_GRB + NEO_KHZ800);
#else
  strip = new Adafruit_DotStar(numPixels, dataPin, clockPin, DOTSTAR_BGR);
#endif
  strip->begin();
  strip->setBrightness(20);
  strip->show();
  black();
  colorWipe(0xFFFF00, 1);
  black();
}

void setup() {
  configMode.reserve(32);
  configFirmwareVersion.reserve(128);
  pinMode(LED_BUILTIN, OUTPUT);

  makeNewStrip(NUMPIXELS, DEFAULT_DATA_PIN, DEFAULT_CLOCK_PIN);
  initPhantoms();
  
  USE_SERIAL.begin(115200);
  USE_SERIAL.printf("Starting: %s\n", BUILD_VERSION);

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  wifiMulti.addAP("theonet_EXT", "juneaudog");

  configMutex = xSemaphoreCreateMutex();
  xTaskCreate(TaskCheckin, (const char *)"Checkin", 1024*40, NULL, 2, NULL);
  xTaskCreate(TaskRunConfig, (const char *)"Run config", 1024*40, NULL, 8, NULL);
}

void flashLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

void setPixel(int index, uint32_t c) {
 if (index >= 0 && index <= strip->numPixels()-1) {
    strip->setPixelColor(index, c);
 }
}

void setAll(uint32_t c) {
  for (int i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
  }
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
    if (wait > 0) {
      strip->show();
      delay(wait);
    }
  }
  if (wait == 0) {
    strip->show();
  }
}

// Set all pixels to black. I am not sure why the extra delays are needed, but this seems
// to prevent spurious pixels from being shown.
void black() {
  delay(5);
  for(int i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, 0);
    strip->show();
    delay(5);
  }
  strip->show();
}

uint32_t interpolate(uint32_t color1, uint32_t color2, float mix) {
  uint32_t r1 = (color1 & 0xff0000) >> 16;
  uint32_t r2 = (color2 & 0xff0000) >> 16;
  uint32_t r3 = (uint32_t)(r1 * (1.0-mix)) + (uint32_t)(r2 * mix) << 16;

  uint32_t g1 = (color1 & 0x00ff00) >> 8;
  uint32_t g2 = (color2 & 0x00ff00) >> 8;
  uint32_t g3 = (uint32_t)(g1 * (1.0-mix)) + (uint32_t)(g2 * mix) << 8;

  uint32_t b1 = (color1 & 0x0000ff);
  uint32_t b2 = (color2 & 0x0000ff);
  uint32_t b3 = (uint32_t)(b1 * (1.0-mix)) + (uint32_t)(b2 * mix);

  return r3 | g3 | b3;
}

void mixBetween(uint32_t color1, uint32_t color2, int numsteps, uint8_t wait) {
  setAll(color1);
  strip->show();
  delay(wait);
  float mix = 0.0;
  for (mix = 0.0; mix < 1.0; mix += (1.0 / numsteps)) {
    uint32_t tc = interpolate(color1, color2, mix);
    setAll(tc);
    strip->show();
    delay(wait);
  }
  setAll(color2);
  strip->show();
  delay(wait);
}

String randomMode() {
  int index = random(0, NUM_RANDOM_MODES-1);
  USE_SERIAL.printf("Selecting random mode %d: %s\n", index, RANDOM_MODES[index]);
  return RANDOM_MODES[index];
}

void strobe(uint32_t c, int numsteps, uint8_t wait) {
  mixBetween(0, c, numsteps/2, wait);
  mixBetween(c, 0, numsteps/2, wait);
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip->numPixels(); i++) {
      strip->setPixelColor(i, Wheel((i+j) & 255));
    }
    strip->show();
    delay(wait);
  }
}

struct {
  float value;
  bool growing;
} rainState[MAX_PIXELS];

void rain(uint32_t color, int maxdrops, int wait, float initValue, float growSpeed, float fadeSpeed, bool expand) {  
  for (int i = 0; i < MAX_PIXELS; i++) {
    rainState[i].value = 0.0;
    rainState[i].growing = false;
  }
  for (int cycle = 0; cycle < maxdrops * 20; cycle++) {
    if (random(0, strip->numPixels()) <= maxdrops) {
      int p = random(0, strip->numPixels());
      if (rainState[p].value <= 0.0) {
        rainState[p].value = initValue;
        rainState[p].growing = true;
      }
    }
    for (int i = 0; i < strip->numPixels(); i++) {
      float val = rainState[i].value;
      if (val < 0.0) {
        val = 0.0;
      }
      if (val > 1.0) {
        val = 1.0;
      }
      uint32_t tc = interpolate(0, color, val);
      strip->setPixelColor(i, tc);
      
      if (rainState[i].value >= 1.0) {
        rainState[i].growing = false;
        if (expand) {
          int p = i-1;
          if (p >= 0 && rainState[p].value <= 0.0) {
            rainState[p].value = initValue;
            rainState[p].growing = true;
          }
          p = i+1;
          if (p < strip->numPixels() && rainState[p].value <= 0.0) {
            rainState[p].value = initValue;
            rainState[p].growing = true;
          }
        }
      }
      
      if (rainState[i].value > 0.0) {
        if (rainState[i].growing) {
          rainState[i].value += growSpeed;
        } else {
          rainState[i].value -= fadeSpeed;
        }
      }
    }
    strip->show();
    delay(wait);
  }
}

#define MAX_SPLATS 1
#define MAX_SPLAT_STEPS 20
struct {
  int index;
  int step;
} splatState[MAX_SPLATS];

void splat(uint32_t color, int wait) {
  for (int i = 0; i < MAX_SPLATS; i++) {
    splatState[i].index = -1;
  }
  int curSplat = 0;
  for (int c = 0; c < 100; c++) {
    if (splatState[curSplat].index == -1) {
      // Start a new splat.
      splatState[curSplat].index = random(0, strip->numPixels());
      splatState[curSplat].step = 0;
      USE_SERIAL.printf("Adding splat %d at index %d\n", curSplat, splatState[curSplat].index);
      curSplat++;
      curSplat = curSplat % MAX_SPLATS; 
    }

    for (int s = 0; s < MAX_SPLATS; s++) {
      int index = splatState[s].index;
      if (index == -1) continue;
      int st = splatState[s].step;
      float baseVal;
      int spread;

      if (st < MAX_SPLAT_STEPS) {
        baseVal = st / (MAX_SPLAT_STEPS * 1.0);
      } else if (st < 2*MAX_SPLAT_STEPS) {
        baseVal = 1.0 - ((st-(2*MAX_SPLAT_STEPS)) / (MAX_SPLAT_STEPS*1.0));
      } else {
        baseVal = 0.0;
      }
      
      spread = st;
      for (int i = 0; i < spread; i++) {
        //float val = baseVal - (baseVal / spread);
        float val = baseVal;
        uint32_t tc = interpolate(0, color, val);
        setPixel(index + i, tc);
        setPixel(index - i, tc);
      } 
      splatState[s].step++;
    }
    strip->show();
    delay(wait);
  }
}

void halloween(int wait) {
  strobe(0x9b009b, 20, wait);
  strobe(0xff5500, 10, wait);
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip->numPixels(); i++) {
      strip->setPixelColor(i, Wheel(((i * 256 / strip->numPixels()) + j) & 255));
    }
    strip->show();
    delay(wait);
  }
}

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<1000; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip->numPixels(); i=i+3) {
        strip->setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip->show();

      delay(wait);

      for (uint16_t i=0; i < strip->numPixels(); i=i+3) {
        strip->setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip->numPixels(); i=i+3) {
        strip->setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip->show();

      delay(wait);

      for (uint16_t i=0; i < strip->numPixels(); i=i+3) {
        strip->setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

void spackle(uint8_t cycles, uint8_t maxSet, uint8_t wait) {
  colorWipe(0, 0); // Set to black.

  int setPixels[maxSet];
  for (int i = 0; i < maxSet; i++) {
    setPixels[i] = -1;
  }
  
  for (uint16_t i = 0; i < cycles; i++) {
    if (setPixels[i % maxSet] != -1) {
      strip->setPixelColor(setPixels[i % maxSet], 0);
    }
    int index = random(0, strip->numPixels());
    uint32_t col = Wheel(random(0, 255));
    setPixels[i % maxSet] = index;
    strip->setPixelColor(index, col);
    strip->show();
    delay(wait);
  }
}

void incrementFire(int index) {
  if (index < 0) {
    return;
  }
  if (index >= strip->numPixels()) {
    return;
  }
  uint32_t c = strip->getPixelColor(index);
  if (c >= 0xf00000) {
    return;
  }
  c += 0x100400;
  strip->setPixelColor(index, c);
}

void fire(int cycles, int wait) {
  colorWipe(0, 0); // Set to black.

  //int start = random(0, strip->numPixels());
  int start = 32+8;
  strip->setPixelColor(start, 0x100400);
  strip->show();
  delay(wait);

  // XXX(mdw) - This is buggy.
  for (int cycle = 0; cycle < cycles; cycle++) {
    for (int i = 0; i < strip->numPixels(); i++) {
      uint32_t cur = strip->getPixelColor(i);
      if (cur == 0 || cur >= 0xf00000) {
        continue;
      }
      incrementFire(i);
      incrementFire(i+1);
      incrementFire(i-1);
      strip->show();
      delay(wait);
      break;
    }
  }
}

void candle(int wait) {
  for (int cycle = 0; cycle < 1000; cycle++) {
    int red = random(0xc0, 0xff);
    int green = (red * 0.8);
    int blue = 0;
    uint32_t curColor = strip->Color(red, green, blue);
    colorWipe(curColor, 0);
    int w = random(wait/2, wait*2);
    delay(w);
  }
}

void flicker(uint32_t color, int brightness, int wait) {
  int curBright = brightness;
  for (int cycle = 0; cycle < 100; cycle++) {
    int step = random(0, 10);
    if (random(0, 10) < 5) {
      curBright += step;
    } else {
      curBright -= step;
    }
    if (curBright < 40) {
      curBright = 40;
    }
    if (curBright > brightness) {
      curBright = brightness;
    }
    strip->setBrightness(curBright);
    colorWipe(color, 0);
    int w = random(wait/2, wait*2);
    delay(w);
  }
}

void skewBrightness() {
  int b = strip->getBrightness();
  int bc = random(0, 20);
  if (random(0, 100) < 50) {
    b -= bc;
  } else {
    b += bc;
  }
  if (b < 10) {
    b = 10;
  }
  if (b > 120) {
    b = 80;
  }
  strip->setBrightness(b);
}

#define MAX_PHANTOMS 5
struct {
  int index;
  uint32_t color;
  int dir;
} phantomState[MAX_PHANTOMS];

void initPhantoms() {
  for (int p = 0; p < MAX_PHANTOMS; p++) {
    phantomState[p].index = -1;
  }
}

void movePhantom(int p) {
  int pi = phantomState[p].index;
  int r = random(0, 100);
  if (random(0, 100) < 10) {
    phantomState[p].dir *= -1;
  }
  int dir = phantomState[p].dir;
  if (random(0, 100) > 10) {
    return;
  }
  pi += dir;
  if (pi < 1) {
    pi = 1;
  }
  if (pi > strip->numPixels()-2) {
    pi = strip->numPixels()-2;
  }
  phantomState[p].index = pi;
}

void drawPhantom(int p, int tail) {
  int pi = phantomState[p].index;
  uint32_t color = phantomState[p].color;
  int dir = phantomState[p].dir * -1;
  for (int j = 0; j < tail; j++) {
    int index = pi + (j * dir);
    float fade = (j*1.0) / (tail * 1.0);
    if (fade > 1.0) {
      fade = 1.0;
    }
    uint32_t tc = interpolate(color, 0, fade);
    setPixel(index, tc);
  }
  for (int j = 0; j < tail/2; j++) {
    int index = pi - (j * dir);
    float fade = (j*1.0) / (tail * 1.0);
    if (fade > 1.0) {
      fade = 1.0;
    }
    uint32_t tc = interpolate(color, 0, fade);
    setPixel(index, tc);
  }
}

void phantom(uint32_t color, int numPhantoms, int tail, int wait) {
  for (int p = 0; p < numPhantoms; p++) {
    phantomState[p].index = random(1, strip->numPixels()-1);
    phantomState[p].color = color;
    if (random(0, 100) < 50) {
      phantomState[p].dir = 1;
    } else {
      phantomState[p].dir = -1;
    }
  }
  for (int p = numPhantoms; p < MAX_PHANTOMS; p++) {
    phantomState[p].index = -1;
  }
  
  for (int i = 0; i < 100; i++) {
    if (random(0, 100) < 20) {
      skewBrightness();
    }
    setAll(0);
    for (int p = 0; p < numPhantoms; p++) {
      movePhantom(p);
      drawPhantom(p, tail);
    }
    strip->show();
    delay(wait);
  }
}

void bounce(uint32_t color, int wait) {
  colorWipe(0, 0); // Set to black.

  int curSize = 1;
  int maxSize = 100;
  int dir = 1;
  int curIndex = 0;
  while (curSize < maxSize) {
    for (int i = 0; i < curSize; i++) {
      strip->setPixelColor(curIndex + i, color);
    }
    if (dir > 0 && curIndex-1 > 0) {
      strip->setPixelColor(curIndex-1, 0);
    }
    if (dir < 0 && curIndex+curSize < strip->numPixels()) {
      strip->setPixelColor(curIndex+curSize, 0);
    }
    strip->show();
    delay(wait);
    curIndex += dir;
    if (dir > 0 && curIndex+curSize >= strip->numPixels()) {
      dir = -1;
      curSize += 2;
    } else if (dir < 0 && curIndex == 0) {
      dir = 1;
      curSize += 2;
    }
  }
}

void comet(uint32_t color, int tail, int wait) {
  int dir = 1;
  int curIndex = 0;
  int numBounces = 0;

  while (numBounces < 2) {
    int p;
    for (int offset = 0; offset < tail; offset++) {
      float fade = (offset*offset*1.0) / (tail * 1.0);
      if (fade > 1.0) {
        fade = 1.0;
      }
      uint32_t tc = interpolate(color, 0, fade);
      p = curIndex - (offset * dir);
      if (p >= 0 && p < strip->numPixels()) {
        strip->setPixelColor(p, tc);
      } 
    }
    // Set the end of the comet to black, regardless.
    if (p >= 0 && p < strip->numPixels()) {
      strip->setPixelColor(p, 0);
    }
    strip->show();
    delay(wait);

    curIndex += dir;
    if (curIndex >= strip->numPixels()) {
      curIndex = strip->numPixels()-1;
      dir = -1;
      numBounces++;
    } else if (curIndex < 0) {
      curIndex = 0;
      dir = 1;
      numBounces++;
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip->Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip->Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

int wheelPos = 0; // Current color changing wheel position.

// Run the current config.
void runConfig() {
  USE_SERIAL.println("runConfig mode: " + configMode);

  String cMode;
  uint32_t cColor;
  int cColorChange, cBrightness, cSpeed, cNumPixels, cDataPin, cClockPin;
  bool cEnabled;

  // Read local copy of config to avoid holding mutex for too long.
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    cEnabled = configEnabled;
    cMode = configMode;
    cColor = strip->Color(configRed, configGreen, configBlue);
    cColorChange = configColorChange;
    cBrightness = configBrightness;
    cSpeed = configSpeed;
    cNumPixels = configNumPixels;
    cDataPin = configDataPin;
    cClockPin = configClockPin;
    xSemaphoreGive(configMutex);
  } else {
    // Can't get mutex to read config, just bail.
    USE_SERIAL.println("Warning - runConfig() unable to get config mutex.");
    return;
  }

  // If we have a new config for the number of pixels, reset the strip.
  if (cNumPixels != strip->numPixels()) {
    makeNewStrip(configNumPixels, cDataPin, cClockPin);
  }

  if (cMode == "random") {
    cMode = randomMode();
  }

  if (cColorChange > 0) {
    wheelPos += cColorChange;
    wheelPos = wheelPos % 255;
    cColor = Wheel(wheelPos);
  }

  // XXX MDW HACKING
  splat(0xff0000, 100);
  return;

  USE_SERIAL.println("Running config: " + cMode + " enabled " + cEnabled);

  if (cMode == "none" || cMode == "off" || !cEnabled) {
    black();
    delay(1000);

  } else if (cMode == "wipe") {
    strip->setBrightness(cBrightness);
    colorWipe(cColor, cSpeed);
    colorWipe(0, cSpeed);
    
  } else if (cMode == "theater") {
    strip->setBrightness(cBrightness);
    theaterChase(cColor, cSpeed);
    
  } else if (cMode == "rainbow") {
    strip->setBrightness(cBrightness);
    rainbow(cSpeed);
    
  } else if (cMode == "rainbowCycle") {
    strip->setBrightness(cBrightness);
    rainbowCycle(cSpeed);
    
  } else if (cMode == "spackle") {
    strip->setBrightness(cBrightness);
    spackle(10000, 50, cSpeed);
    
  } else if (cMode == "fire") {
    strip->setBrightness(cBrightness);
    fire(1000, cSpeed);
    
  } else if (cMode == "bounce") {
    strip->setBrightness(cBrightness);
    bounce(cColor, cSpeed);
    
  } else if (cMode == "strobe") {
    strip->setBrightness(cBrightness);
    strobe(cColor, 10, cSpeed);

  } else if (cMode == "rain") {
    strip->setBrightness(cBrightness);
    rain(cColor, NUMPIXELS, cSpeed, 1.0, 0.05, 0.05, false);

  } else if (cMode == "snow") {
    rain(cColor, NUMPIXELS, cSpeed, 0.02, 0.01, 0.2, false);

  } else if (cMode == "sparkle") {
    rain(cColor, NUMPIXELS, cSpeed, 1.0, 0, 0.4, false);

  } else if (cMode == "comet") {
    strip->setBrightness(cBrightness);
    comet(cColor, 100, cSpeed);

  } else if (cMode == "candle") {
    strip->setBrightness(cBrightness);
    candle(cSpeed);

  } else if (cMode == "flicker") {
    flicker(cColor, cBrightness, cSpeed);

  } else if (cMode == "phantom") {
    strip->setBrightness(cBrightness);
    phantom(cColor, 5, 10, cSpeed);

  } else if (cMode == "halloween") {
    halloween(cSpeed);

  } else if (cMode == "test") {
    strip->setBrightness(50);
    colorWipe(0xff0000, 5);
    colorWipe(0x00ff00, 5);
    colorWipe(0x0000ff, 5);
    colorWipe(0, 5);
    
  } else {
    USE_SERIAL.println("Unknown mode: " + cMode);
    black();
    delay(1000);
  }
}

void checkin() {
  USE_SERIAL.print("MAC address ");
  USE_SERIAL.println(WiFi.macAddress());
  USE_SERIAL.print("IP address is ");
  USE_SERIAL.println(WiFi.localIP().toString());

  String url = "https://team-sidney.firebaseio.com/checkin/" + WiFi.macAddress() + ".json";
  http.setTimeout(1000);
  http.begin(url);

  String curModeJson;
  StaticJsonDocument<512> checkinDoc;
  JsonObject checkinPayload = checkinDoc.to<JsonObject>();

  // This is Firebase magic to cause a server variable to be set with the current server timestamp on receipt.
  JsonObject ts = checkinPayload.createNestedObject("timestamp");
  ts[".sv"] = "timestamp";
  checkinPayload["version"] = BUILD_VERSION;
  checkinPayload["mac"] = WiFi.macAddress();
  checkinPayload["ip"] = WiFi.localIP().toString();
  checkinPayload["rssi"] = WiFi.RSSI();

  String payload;
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    JsonObject curConfig = curConfigDocument.as<JsonObject>(); 
    if (!curConfig.isNull()) {
     JsonObject checkinConfig = checkinPayload.createNestedObject("config");
     checkinConfig.copyFrom(curConfig);
    }
    serializeJson(checkinPayload, payload);
    xSemaphoreGive(configMutex);
  } else {
    // Can't get lock to serialize current config, just bail out.
    USE_SERIAL.println("Warning - checkin() unable to get config mutex.");
    return;
  }
  
  USE_SERIAL.print("[HTTP] PUT " + url + "\n");
  USE_SERIAL.print(payload + "\n");
  
  int httpCode = http.PUT(payload);
  if (httpCode > 0) {
    USE_SERIAL.printf("[HTTP] Response code: %d\n", httpCode);
    String payload = http.getString();
    USE_SERIAL.println(payload);
  } else {
    USE_SERIAL.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void readConfig() {
  USE_SERIAL.println("readConfig called");
  
  String url = "https://team-sidney.firebaseio.com/strips/" + WiFi.macAddress() + ".json";
  http.setTimeout(1000);
  http.begin(url);

  USE_SERIAL.print("[HTTP] GET " + url + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    USE_SERIAL.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }
  
  String payload = http.getString();
  USE_SERIAL.printf("[HTTP] Response code: %d\n", httpCode);
  USE_SERIAL.println(payload);

  bool needsFirmwareUpdate = false;

  // Parse JSON config.
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    DeserializationError err = deserializeJson(curConfigDocument, payload);
    USE_SERIAL.print("Deserialize returned: ");
    USE_SERIAL.println(err.c_str());

    JsonObject cc = curConfigDocument.as<JsonObject>();
    configNumPixels = cc["numPixels"];
    if (configNumPixels == 0) {
      configNumPixels = NUMPIXELS;
    }
    configDataPin = cc["dataPin"];
    if (configDataPin == 0) {
      configDataPin = DEFAULT_DATA_PIN;
    }
    configClockPin = cc["clockPin"];
    if (configClockPin == 0) {
      configClockPin = DEFAULT_CLOCK_PIN;
    }
    configMode = (const String &)cc["mode"];
    configEnabled = (cc["enabled"] == true);
    configSpeed = cc["speed"];
    configBrightness = cc["brightness"];
    configColorChange = cc["colorChange"];
    configRed = cc["red"];
    configGreen = cc["green"];
    configBlue = cc["blue"];
    configFirmwareVersion = (const String &)cc["version"];

    // If the firmware version needs to be updated, kick off the update.
    if (configFirmwareVersion != BUILD_VERSION &&
        configFirmwareVersion != "none" &&
        configFirmwareVersion != "" &&
        configFirmwareVersion != "current") {
      needsFirmwareUpdate = true;
    }
    
    xSemaphoreGive(configMutex);
  } else {
    USE_SERIAL.println("Warning - readConfig() unable to get config mutex");
  }
  http.end();

  if (needsFirmwareUpdate) {
    updateFirmware();
  }

}

/* Task to periodically checkin and read new config. */
void TaskCheckin(void *pvParameters) {
  for (;;) {
    if ((wifiMulti.run() == WL_CONNECTED)) {
      for (int i = 0; i < 5; i++) {
        flashLed();
      }
      checkin();
      readConfig();
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

/* Task to run current LED config. */
void TaskRunConfig(void *pvParameters) {
  for (;;) {
    runConfig();
  }
}

void readFirmwareMetadata(String firmwareVersion) {
  USE_SERIAL.println("readFirmwareMetadata called");
  
  String url = "https://team-sidney.firebaseio.com/firmware/" + firmwareVersion + ".json";
  http.setTimeout(1000);
  http.begin(url);

  USE_SERIAL.print("[HTTP] GET " + url + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    USE_SERIAL.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }
  
  String payload = http.getString();
  USE_SERIAL.printf("[HTTP] Response code: %d\n", httpCode);
  USE_SERIAL.println(payload);

  // Parse JSON config.
  DeserializationError err = deserializeJson(firmwareVersionDocument, payload);
  USE_SERIAL.print("Deserialize returned: ");
  USE_SERIAL.println(err.c_str());

  JsonObject fwdoc = firmwareVersionDocument.as<JsonObject>();
  firmwareUrl = (const String &)fwdoc["url"];
  firmwareHash = (const String &)fwdoc["hash"];

  http.end();
}

void updateFirmware() {
  String newVersion;
  
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    newVersion = configFirmwareVersion;
    xSemaphoreGive(configMutex);
  } else {
    USE_SERIAL.println("Warning - updateFirmware() unable to get config mutex");
    return;
  }
  
  USE_SERIAL.printf("Current firmware version: %s\n", BUILD_VERSION);
  USE_SERIAL.println("Desired firmware version: " + newVersion);

  readFirmwareMetadata(newVersion);
  USE_SERIAL.println("Read firmware metadata, URL is " + firmwareUrl);
  USE_SERIAL.println("Hash " + firmwareHash);

  USE_SERIAL.println("Starting OTA...");
  http.begin(firmwareUrl);

  USE_SERIAL.print("[HTTP] GET " + firmwareUrl + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    USE_SERIAL.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }

  int contentLen = http.getSize();
  USE_SERIAL.printf("Content-Length: %d\n", contentLen);
  bool canBegin = Update.begin(contentLen);
  if (canBegin) {
    USE_SERIAL.println("OK to start OTA.");
  } else {
    USE_SERIAL.println("Not enough space to begin OTA");
    return;
  }

  WiFiClient* client = http.getStreamPtr();
  size_t written = Update.writeStream(*client);
  USE_SERIAL.printf("OTA: %d/%d bytes written.\n", written, contentLen);
  if (written != contentLen) {
    USE_SERIAL.println("Wrote partial binary. Giving up.");
    return;
  }

  if (Update.end()) {
    USE_SERIAL.println("OTA done!");
  } else {
    USE_SERIAL.println("Error from Update.end(): " + String(Update.getError()));
    return;
  }
  
  String md5 = Update.md5String();
  USE_SERIAL.println("MD5: " + md5);
  
  if (Update.isFinished()) {
    USE_SERIAL.println("Update successfully completed. Rebooting.");
    ESP.restart();
  } else {
    USE_SERIAL.println("Error from Update.isFinished(): " + String(Update.getError()));
    return;
  }
}

void loop() {
  delay(60000);
}
