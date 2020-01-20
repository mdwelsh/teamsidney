#ifndef _Blinky_h
#define _Blinky_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/task.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

// Define the below to use Neopixels instead of DotStars.
//#define USE_NEOPIXEL
// Define the below to use a Dotstar matrix instead of a strip.
//#define DOTSTAR_MATRIX

#ifdef USE_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#else
#include <Adafruit_DotStar.h>
#endif

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

// Start with this many pixels, but can be reconfigured.
#define NUMPIXELS 144
// Maximum number, for the sake of maintaining state.
#define MAX_PIXELS 350
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
#define DEFAULT_DATA_PIN NEOPIXEL_DATA_PIN
#define DEFAULT_CLOCK_PIN 0 // Unused.
#else
#define DEFAULT_DATA_PIN DOTSTAR_DATA_PIN
#define DEFAULT_CLOCK_PIN DOTSTAR_CLOCK_PIN
#endif

// Maximum length of mode string.
#define MAX_MODE_LEN 32
// Maximum length of firmware version string.
#define MAX_FIRMWARE_LEN 64

// We maintain the strip as a global variable mainly because it can be different types.
#ifdef USE_NEOPIXEL
extern Adafruit_NeoPixel *strip;
#else
extern Adafruit_DotStar *strip;
#endif

// Describes the current device configuration.
typedef struct _deviceConfig {
  char mode[MAX_MODE_LEN];
  bool enabled;
  int numPixels, dataPin, clockPin, colorChange, brightness, speed;
  uint32_t color1, color2;
  char firmwareVersion[MAX_FIRMWARE_LEN];
} deviceConfig_t;

uint32_t interpolate(uint32_t color1, uint32_t color2, float mix);
void setPixel(int index, uint32_t c);
void setAll(uint32_t c);
void colorWipe(uint32_t c, uint8_t wait);
uint32_t Wheel(byte WheelPos);

#endif
