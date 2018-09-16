#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEOPIXEL_PIN 14
#define POT1_PIN A0
#define POT2_PIN A1
#define BUTTON1_PIN A2
#define MIC_PIN A3

uint32_t fade(uint32_t color, uint8_t fade);
uint32_t Wheel(byte WheelPos);

int brightness = 50;

class Firework {
  public:
    Firework(Adafruit_NeoPixel *strip, uint16_t pos, uint32_t color, uint8_t width) :
      strip(strip), pos(pos), color(color), width(width), step(0) {
    }
    Firework() {}

    void update() {
      step += 1;
      if (step <= width) {
        for (int i = 0; i < step; i++) {
          strip->setPixelColor(pos + i, fade(color, 100 - (15 * i)));
          strip->setPixelColor(pos - i, fade(color, 100 - (15 * i)));
        }
      } else {
        for (int i = 0; i < step; i++) {
          strip->setPixelColor(pos + i, 0);
          strip->setPixelColor(pos - i, 0);
        }
      }
      strip->show();

      if (step > width) {
        step = 0;
        pos = random(10, strip->numPixels() - 10);
        color = Wheel(random(0, 255));
        width = random(2, 5);
      }
    }

  protected:
    uint16_t pos;
    uint32_t color;
    uint8_t step;
    uint8_t width;
    Adafruit_NeoPixel* strip;


};


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

Firework f;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

 // pinMode(POT1_PIN, INPUT);
 // pinMode(POT2_PIN, INPUT);
 // pinMode(BUTTON1_PIN, INPUT);
 // pinMode(MIC_PIN, INPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(brightness);

  f = Firework(&strip, 20, 0xa00080, 10);
  //colorWipe(0xff0000, 10);


}

uint32_t fade(uint32_t color, uint8_t fade) {
  uint8_t r = (color & 0xff0000) >> 16;
  uint8_t g = (color & 0xff00) >> 8;
  uint8_t b = (color & 0xff);
  r = (r * fade) >> 8;
  g = (g * fade) >> 8;
  b = (b * fade) >> 8;
  return strip.Color(r, g, b);
}

void pellet(uint32_t color, uint16_t pos, uint16_t width) {
  for (uint16_t offset = 0; offset < width; offset++) {
    strip.setPixelColor(pos + offset, fade(color, 0.2));
    strip.setPixelColor(pos - offset, fade(color, 0.4));
  }
  strip.setPixelColor(pos + width, 0);
  strip.setPixelColor(pos - width, 0);

  strip.show();
}

void bounce(uint32_t color, uint16_t width, uint8_t wait) {
  for (uint16_t i = width / 2; i < strip.numPixels() - width / 2; i += 1) {
    pellet(color, i, width);
    delay(wait);

  }
  for (uint16_t i = strip.numPixels() - width / 2; i > width / 2; i -= 1) {
    pellet(color, i, width);
    delay(wait);
  }
}

#define NUM_FIREWORKS 4
Firework fireworks[NUM_FIREWORKS];
int num_fireworks = 0;

void loop() {
   return;
   /*
  //int pot1 = analogRead(POT1_PIN);
  int pot1 = analogRead(MIC_PIN);
  int pot2 = analogRead(POT2_PIN);
  //int button1 = analogRead(BUTTON1_PIN);
  
  int numLeds = (pot1 / 20.0) * strip.numPixels();
  //int db = (pot1+83.2073) / 11.003;
  //int numLeds = db / 2;
  //uint32_t color = Wheel((pot1 / 1024.0) * 255);
  uint32_t color = 0x0000b0;

  //if (button1 > 255) {
  //  color = 0xff0000;
  //}

  int i;
  for (i = 0; i < numLeds; i++) {
    strip.setPixelColor(i, color);
  }
  for (; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
   */

   

  if (num_fireworks < NUM_FIREWORKS && random(100) < 10) {
    fireworks[num_fireworks] = Firework(&strip, random(10, strip.numPixels() - 10), Wheel(random(0, 255)), random(2, 5));
    num_fireworks++;
  }
  for (int i = 0; i < num_fireworks; i++) {
    fireworks[i].update();
  }
  delay(100);

}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
