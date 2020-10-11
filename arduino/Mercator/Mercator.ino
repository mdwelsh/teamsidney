
#include <Adafruit_DotStar.h>
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET

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

#define HALL_COUNT 10

int cur_hall = 0;
int leds_on = false;
int hall_count = 0;

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

  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(PWMPIN, LEDC_CHANNEL_0);
  ledcAnalogWrite(LEDC_CHANNEL_0, 100); // This seems to work pretty well with a 1 KHz PWM clock.
  
  strip.begin();
  strip.show();
  strip.setBrightness(30);
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


void loop() {
  int potVal = analogRead(POTPIN);
  ledcAnalogWrite(LEDC_CHANNEL_0, potVal >> 4);

  int hall = digitalRead(HALLPIN);
  
  if (hall == LOW && cur_hall == 0) {
    // Hall effect trigger on pin going low.
    cur_hall = 1;
    hall_count++;

    if (hall_count == HALL_COUNT) {
      hall_count = 0;
      int color;
      if (leds_on) {
        color = 0x0;
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

}
