
#include <Adafruit_DotStar.h>
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET

#define NUMPIXELS 72 // Number of LEDs in strip
#define DATAPIN    14
#define CLOCKPIN   32
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

void setup() {
  strip.begin();
  strip.show();
  strip.setBrightness(30);
}

void loop() {
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0xff0000);
    strip.show();  
    delay(20);
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0x00ff00);
    strip.show();  
    delay(20);
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0x0000ff);
    strip.show();  
    delay(20);
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0x0);
    strip.show();  
    delay(20);
  }
}
