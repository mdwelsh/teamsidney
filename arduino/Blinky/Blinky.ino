#include <Arduino.h>
#include <ArduinoJson.h>

/* Uncomment for NeoPixel */
// #include <Adafruit_NeoPixel.h>
/* Uncomment for DotStar */
#include <Adafruit_DotStar.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define USE_SERIAL Serial

#define NUMPIXELS 120
#define NEOPIXEL_DATA_PIN 14
#define DOTSTAR_DATA_PIN 14
#define DOTSTAR_CLOCK_PIN 32

//Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_DATA_PIN, DOTSTAR_CLOCK_PIN, DOTSTAR_BGR);

WiFiMulti wifiMulti;
HTTPClient http;
String curMode;
StaticJsonDocument<512> curConfigDocument;

void setup() {
  strip.begin();
  strip.setBrightness(20);
  //strip.show(); // Initialize all pixels to 'off'
  
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


void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    if (wait > 0) {
      strip.show();
      delay(wait);
    }
  }
  if (wait == 0) {
    strip.show();
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

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<1000; j++) {  //do 10 cycles of chasing
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

void spackle(uint8_t cycles, uint8_t maxSet, uint8_t wait) {
  colorWipe(0, 0); // Set to black.

  int setPixels[maxSet];
  for (int i = 0; i < maxSet; i++) {
    setPixels[i] = -1;
  }
  
  for (uint16_t i = 0; i < cycles; i++) {
    if (setPixels[i % maxSet] != -1) {
      strip.setPixelColor(setPixels[i % maxSet], 0);
    }
    int index = random(0, strip.numPixels());
    uint32_t col = Wheel(random(0, 255));
    setPixels[i % maxSet] = index;
    strip.setPixelColor(index, col);
    strip.show();
    delay(wait);
  }
  
}

void incrementFire(int index) {
  if (index < 0) {
    return;
  }
  if (index >= strip.numPixels()) {
    return;
  }
  uint32_t c = strip.getPixelColor(index);
  if (c >= 0xf00000) {
    return;
  }
  c += 0x100400;
  strip.setPixelColor(index, c);
  strip.show();
}

void fire(int cycles, int wait) {
  colorWipe(0, 0); // Set to black.

  int start = random(0, strip.numPixels());
  strip.setPixelColor(start, 0x100400);
  strip.show();
  delay(wait);

  // XXX(mdw) - This is buggy.
  for (int cycle = 0; cycle < cycles; cycle++) {
    for (int i = 0; i < strip.numPixels(); i++) {
      uint32_t cur = strip.getPixelColor(i);
      if (cur == 0 || cur >= 0xf00000) {
        continue;
      }
      incrementFire(i);
      incrementFire(i+1);
      incrementFire(i-1);
      delay(wait);
      break;
    }
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
      strip.setPixelColor(curIndex + i, color);
    }
    if (dir > 0 && curIndex-1 > 0) {
      strip.setPixelColor(curIndex-1, 0);
    }
    if (dir < 0 && curIndex+curSize < strip.numPixels()) {
      strip.setPixelColor(curIndex+curSize, 0);
    }
    strip.show();
    delay(wait);
    curIndex += dir;
    if (dir > 0 && curIndex+curSize >= strip.numPixels()) {
      dir = -1;
      curSize += 2;
    } else if (dir < 0 && curIndex == 0) {
      dir = 1;
      curSize += 2;
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

// Run the current config.
void runConfig() {o
  JsonObject cc = curConfigDocument.to<JsonObject>();
  String mode = cc["mode"];
  int speed = cc["speed"];
  int brightness = cc["brightness"];
  int red = cc["red"];
  int green = cc["green"];
  int blue = cc["blue"];

  strip.setBrightness(brightness);

  USE_SERIAL.println("Running mode: \"" + mode + "\"");
  if (mode == "none" || mode == "off") {
    colorWipe(strip.Color(0, 0, 0), 10);
  } else if (mode == "wipe") {
    colorWipe(strip.Color(red, green, blue), speed);
  } else if (mode == "theater") {
    theaterChase(strip.Color(red, green, blue), speed);
  } else if (mode == "rainbow") {
    rainbow(speed);
  } else if (mode == "rainbowCycle") {
    rainbowCycle(speed);
  } else if (mode == "spackle") {
    spackle(10000, 50, speed);
  } else if (mode == "fire") {
    fire(1000, speed);
  } else if (mode == "bounce") {
    bounce(strip.Color(red, green, blue), speed);
  } else {
    USE_SERIAL.println("Unknown mode: " + mode);
  }
  strip.show();
}

void checkin() {
  USE_SERIAL.print("MDW: MAC address is ");
  USE_SERIAL.print(WiFi.macAddress());
  USE_SERIAL.print("\nMDW: IP address is ");
  USE_SERIAL.print(WiFi.localIP().toString());
  USE_SERIAL.print("\n");

  String url = "https://team-sidney.firebaseio.com/checkin/" + WiFi.macAddress() + ".json";
  http.begin(url);

  String curModeJson;
  StaticJsonDocument<512> checkinDoc;
  JsonObject checkinPayload = checkinDoc.to<JsonObject>();

  // This is Firebase magic to cause a server variable to be set with the current server timestamp on receipt.
  JsonObject ts = checkinPayload.createNestedObject("timestamp");
  ts[".sv"] = "timestamp";
  checkinPayload["mac"] = WiFi.macAddress();
  checkinPayload["ip"] = WiFi.localIP().toString();

  USE_SERIAL.println("\nCurrent config doc at checkin:");
  serializeJson(curConfigDocument, Serial);
  USE_SERIAL.print("\n");
  
  JsonObject curConfig = curConfigDocument.as<JsonObject>();
  USE_SERIAL.println("\nCurrent config at checkin:");
  serializeJson(curConfig, Serial);
  USE_SERIAL.print("\n");
  
 if (!curConfig.isNull()) {
   JsonObject checkinConfig = checkinPayload.createNestedObject("config");
   checkinConfig.copyFrom(curConfig);
  }
  String payload;
  serializeJson(checkinPayload, payload);
  
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
  String url = "https://team-sidney.firebaseio.com/strips/" + WiFi.macAddress() + ".json";
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
  DeserializationError err = deserializeJson(curConfigDocument, payload);
  USE_SERIAL.print("DESERIALIZE error: ");
  USE_SERIAL.println(err.c_str());

  // For testing.
  String output;
  serializeJson(curConfigDocument, Serial);

  // XXX MDW HACKING
  JsonObject obj = curConfigDocument.as<JsonObject>();
  USE_SERIAL.println(obj["hithere"]);  

  http.end();
}


bool initialized = false;
bool connected = false;

void loop() {
  if (!initialized) {
    initialized = true;
    colorWipe(strip.Color(0, 0, 255), 10);
    delay(1000);
  }
  if ((wifiMulti.run() != WL_CONNECTED)) {
    colorWipe(strip.Color(255, 0, 0), 10);
    delay(1000);
    return;
  } else {
    if (!connected) {
      connected = true;
      colorWipe(strip.Color(0, 255, 0), 10);
      delay(1000);
    }
  }
  
  flashLed();
  checkin();
  readConfig();
  //runConfig();
  delay(10 * 1000);
}
