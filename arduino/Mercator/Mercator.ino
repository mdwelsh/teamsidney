
#include <Adafruit_DotStar.h>
#include <SPI.h>
#include "image.h"

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

#define BRIGHTNESS 40
#define NUM_COLUMNS 40

int cur_hall = 0;
int leds_on = false;
int hall_count = 0;
int cur_step = 0;
uint32_t last_hall_time = 0;
uint32_t cycle_time = 0;
uint32_t per_column_time = 0;
uint32_t next_column_time = 0;


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
  ledcAnalogWrite(LEDC_CHANNEL_0, 100); // This seems to work pretty well with a 1 KHz PWM clock.
  
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0xff0000);
    strip.show();  
  }
  delay(200);
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0x00ff00);
    strip.show();  
  }
  delay(200);
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0x0000ff);
    strip.show();  
  }
  delay(200);
  for (int i = 0; i < NUMPIXELS; i++) {
      strip.setPixelColor(i, 0x00000);
  }
  strip.show();

}

void do_testpattern() {
  int color;
  if (leds_on) {
    color = 0xffffff;
    leds_on = false;
  } else {
    color = 0xff0000;
    leds_on = true;
  }
  for (int i = 0; i < NUMPIXELS/2; i++) {
    strip.setPixelColor(i, color);
  }
  for (int i = NUMPIXELS/2; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0x0);
  }
  strip.show();
  //digitalWrite(SIGNALPIN, leds_on);  // To measure.
}

uint32_t get_color(int x, int y) {

  x -= 16;
  y -= 16;
  if (x < 0 || x > IMAGE_COLUMNS-1) return 0x0;
  if (y < 0 || y > IMAGE_ROWS-1) return 0x0;
 
  uint32_t pixel = IMAGE[(y * IMAGE_COLUMNS) + x];
  if (pixel != 0) {
    pixel = interpolate(0xff0000, 0x0000ff, (y * 1.0)/IMAGE_ROWS);
  }
  return pixel;

//     return interpolate(0xff0000, 0x0000ff, (y * 1.0)/(NUMPIXELS/2.0));
  
//   if (x == 0) {
//     digitalWrite(SIGNALPIN, 0);
//     return 0xff0000;
//   } else if (x == NUM_COLUMNS/2) {
//     digitalWrite(SIGNALPIN, 1);
//     return 0x0000ff;
//   } else {
//     return 0x0;
//   }
}

void doPaint(uint32_t cur_time) {
  if (cur_time < next_column_time) {
    return;
  }

  for (int i = 0; i < NUMPIXELS/2; i++) {
    int y = (NUMPIXELS/2) - i;   // Swap y-axis.
    strip.setPixelColor(i, get_color(cur_step, y));
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
    if (cur_time - last_hall_time > 1000) {  // Debounce.
      cur_hall = 1;
      cur_step = 0; // Reset cycle.
      cycle_time = cur_time - last_hall_time;
      per_column_time = cycle_time / NUM_COLUMNS;
      next_column_time = cur_time + per_column_time;
      last_hall_time = cur_time;
    }
  } else if (hall == HIGH && cur_hall == 1) {
    cur_hall = 0;
  }
  doPaint(cur_time);

#if 0
  if (hall == LOW && cur_hall == 0) {
    // Hall effect trigger on pin going low.
    cur_hall = 1;
    hall_count++;

    if (hall_count == HALL_COUNT) {
      hall_count = 0;
      int color;
      if (leds_on) {
        color = 0x000000;
        leds_on = false;
      } else {
        color = 0xff0000;
        leds_on = true;
      }  
      for (int i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, color);
      }
      strip.show();
    }
    
  } else if (hall == HIGH && cur_hall == 1) {
    cur_hall = 0;
  }
#endif

}
