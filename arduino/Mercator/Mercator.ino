
#include <Adafruit_DotStar.h>
#include <SPI.h>

#include "earth.h"
#define IMAGE IMAGE_earth
#define IMAGE_COLUMNS IMAGE_COLUMNS_earth
#define IMAGE_ROWS IMAGE_ROWS_earth

//#include "hollowknight.h"
//#define IMAGE IMAGE_hollowknight
//#define IMAGE_COLUMNS IMAGE_COLUMNS_hollowknight
//#define IMAGE_ROWS IMAGE_ROWS_hollowknight

//#include "hline.h"
//#define IMAGE IMAGE_hline
//#define IMAGE_COLUMNS IMAGE_COLUMNS_hline
//#define IMAGE_ROWS IMAGE_ROWS_hline

#define NUMPIXELS 72 // Number of LEDs in strip
#define DATAPIN    14
#define CLOCKPIN   32
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

#define HALLPIN  15
#define PWMPIN A0
#define POTPIN A1
#define SIGNALPIN 33

#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_13_BIT 13
#define LEDC_BASE_FREQ 10000

#define BRIGHTNESS 20
#define NUM_COLUMNS IMAGE_COLUMNS

#define IDLE_TIME 1000000
#define ALPHA 0.5
//#define FRONT_STRIP_ONLY
//#define WARN_IF_TOO_FAST
#define X_SHIFT 0
#define Y_SHIFT 0
#define BACK_OFFSET 3
#define ROTATE_INTERVAL 20000

int cur_hall = 0;
int cur_step = 0;
int cur_x_offset = 0;
uint32_t last_hall_time = 0;
uint32_t last_rotate_time = 0;
uint32_t cycle_time = 0;
uint32_t per_column_time = 0;
uint32_t next_column_time = 0;

void setAll(uint32_t color) {
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}


// Interpolate between color1 and color2. mix represents the amount of color2.
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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t color_wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

void setup() {
  pinMode(HALLPIN, INPUT_PULLUP);
  pinMode(PWMPIN, OUTPUT);
  pinMode(POTPIN, INPUT);
  pinMode(SIGNALPIN, OUTPUT);
  last_hall_time = micros();
  next_column_time = micros();

  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(PWMPIN, LEDC_CHANNEL_0);
  ledcAnalogWrite(LEDC_CHANNEL_0, 100);
  
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  setAll(0xff0000);
  delay(200);
  setAll(0x00ff00);
  delay(200);
  setAll(0x0000ff);
  delay(200);
  setAll(0x0);
  strip.show();
}

uint32_t get_color(int x, int y) {
  if (x < 0 || x > IMAGE_COLUMNS-1) return 0x0;
  if (y < 0 || y > IMAGE_ROWS-1) return 0x0;
  int index = (y * IMAGE_COLUMNS * 3) + (x * 3);
  uint8_t* p = (uint8_t*)&IMAGE[index];
  uint32_t* pv = (uint32_t*)p;
  uint32_t pixel = *pv;
  return pixel;
}

void doPaint(uint32_t cur_time) {
  if (cur_time < next_column_time) {
    return;
  }
  
  int x = (cur_step - X_SHIFT + cur_x_offset) % NUM_COLUMNS;
  int frontx = x;
  int backx = (x + (NUM_COLUMNS/2)) % NUM_COLUMNS;

  // Front strip.
  for (int i = 0; i < NUMPIXELS/2; i++) {
    int y = (NUMPIXELS/2) - i;   // Swap y-axis.
    strip.setPixelColor(i, get_color(frontx, y));
  }

  // Back strip.
  for (int i = NUMPIXELS/2; i < NUMPIXELS; i++) {
    int y = i - (NUMPIXELS/2) + BACK_OFFSET;
#ifdef FRONT_STRIP_ONLY
    strip.setPixelColor(i, 0x0);
#else
    strip.setPixelColor(i, get_color(backx, y));
#endif
  }

  strip.show();
  cur_step++;
  next_column_time = cur_time + per_column_time;
}

void loop() {
  int potVal = analogRead(POTPIN);
  ledcAnalogWrite(LEDC_CHANNEL_0, potVal >> 4);

  uint32_t cur_time = micros();
  int hall = digitalRead(HALLPIN);

  if (hall == LOW && cur_hall == 0) {
    if (cur_time - last_hall_time > 10000) {  // Debounce.
#ifdef HALLDEBUG
    setAll(0xff0000);
#endif      
      cur_hall = 1;

#ifdef WARN_IF_TOO_FAST
      if (cur_step < NUM_COLUMNS-2) {
        setAll(0xff0000);
      }
#endif

      cur_step = 0; // Reset cycle.
      cycle_time = (ALPHA * cycle_time) + ((1.0 - ALPHA) * (cur_time - last_hall_time));
      per_column_time = (cycle_time / NUM_COLUMNS);
      next_column_time = cur_time - per_column_time;
      last_hall_time = cur_time;

#ifdef ROTATE_INTERVAL
      if (cur_time - last_rotate_time > ROTATE_INTERVAL) {
        cur_x_offset++;
        last_rotate_time = cur_time;
      }
#endif

    }
  } else if (hall == HIGH && cur_hall == 1) {
    cur_hall = 0;
#ifdef HALLDEBUG
    setAll(0x0);
#endif
  }
#ifndef HALLDEBUG
  if (cur_time - last_hall_time < IDLE_TIME) {
    doPaint(cur_time);
  } else {
    setAll(0x0);
  }
#endif

}
