#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define NEOPIXEL_DATA_PIN 14

#define USE_SERIAL Serial

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, NEOPIXEL_DATA_PIN, NEO_GRB + NEO_KHZ800);
WiFiMulti wifiMulti;
HTTPClient http;
String curMode;

void setup() {
  strip.begin();
  strip.setBrightness(20);
  strip.show(); // Initialize all pixels to 'off'
  
  curMode = String("none");
  pinMode(LED_BUILTIN, OUTPUT);

  USE_SERIAL.begin(115200);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();
  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  wifiMulti.addAP("theonet", "juneaudog");
}

void flashLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

void checkin() {
  USE_SERIAL.print("MDW: MAC address is ");
  USE_SERIAL.print(WiFi.macAddress());
  USE_SERIAL.print("\nMDW: IP address is ");
  USE_SERIAL.print(WiFi.localIP().toString());
  USE_SERIAL.print("\n");

  String url = "https://team-sidney.firebaseio.com/checkin/" + WiFi.macAddress() + ".json";
  http.begin(url);

  // {".sv": "timestamp"} writes the server's timestamp value when the payload is received.
  // Firebase is so cool.
  String payload = "{\"timestamp\": {\".sv\": \"timestamp\"}, \"mac\": \"" + WiFi.macAddress() + "\", \"ip\": \"" + WiFi.localIP().toString() + "\", \"mode\": \"" + curMode +"\"}";

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

void readMode() {
  String url = "https://team-sidney.firebaseio.com/strips/" + WiFi.macAddress() + ".json";
  http.begin(url);

  USE_SERIAL.print("[HTTP] GET " + url + "\n");
  int httpCode = http.GET();

  if (httpCode > 0) {
    USE_SERIAL.printf("[HTTP] Response code: %d\n", httpCode);
    curMode = String(http.getString());
    USE_SERIAL.println(curMode);
  } else {
    USE_SERIAL.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
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

void runMode() {
  //curMode = "wipe";
  USE_SERIAL.println("Running mode: \"" + curMode + "\"");
  if (curMode == "\"none\"" || curMode == "\"off\"") {
    colorWipe(strip.Color(0, 0, 0), 10);
  } else if (curMode.equals("\"wipered\"")) {
    colorWipe(strip.Color(255, 0, 0), 10);
  } else if (curMode.equals("\"wipegreen\"")) {
    colorWipe(strip.Color(0, 255, 0), 10);
  } else if (curMode.equals("\"wipeblue\"")) {
    colorWipe(strip.Color(0, 0, 255), 10);
  } else if (curMode.equals("\"rainbow\"")) {
    rainbow(10);
  } else if (curMode.equals("\"rainbowcycle\"")) {
    rainbowCycle(10);
  } else {
    USE_SERIAL.println("Unknown mode: " + curMode);
  }
  strip.show();
}

void loop() {
  if ((wifiMulti.run() != WL_CONNECTED)) {
    delay(10000);
    return;
  }
  flashLed();
  checkin();
  readMode();
  USE_SERIAL.println("Setting mode: " + curMode + "\n");
  runMode();
  delay(10 * 1000);
}
