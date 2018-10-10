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

#ifdef USE_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#else
#include <Adafruit_DotStar.h>
#endif

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define USE_SERIAL Serial

const char BUILD_VERSION[] = (__DATE__ " " __TIME__);

// Start with this many pixels, but can be reconfigured.
#define NUMPIXELS 120
// Maximum number, for the sake of maintaining state.
#define MAX_PIXELS 200
#define NEOPIXEL_DATA_PIN 14
#define DOTSTAR_DATA_PIN 14
#define DOTSTAR_CLOCK_PIN 32

#ifdef USE_NEOPIXEL
Adafruit_NeoPixel *strip = NULL;
#else
Adafruit_DotStar *strip = NULL;
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
int configColorChange = 0;
int configBrightness = 100;
int configSpeed = 100;
int configRed = 100;
int configBlue = 100;
int configGreen = 0;
String configFirmwareUrl = "";

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

void makeNewStrip(int numPixels) {
  USE_SERIAL.printf("Making new strip with %d pixels\n", numPixels);
  if (strip != NULL) {
    black();
    delete strip;
  }
#ifdef USE_NEOPIXEL
  strip = new Adafruit_NeoPixel(numPixels, NEOPIXEL_DATA_PIN, NEO_GRB + NEO_KHZ800);
#else
  strip = new Adafruit_DotStar(numPixels, DOTSTAR_DATA_PIN, DOTSTAR_CLOCK_PIN, DOTSTAR_BGR);
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
  pinMode(LED_BUILTIN, OUTPUT);

  makeNewStrip(NUMPIXELS);
  
  USE_SERIAL.begin(115200);
  USE_SERIAL.printf("Starting: %s\n", BUILD_VERSION);

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  wifiMulti.addAP("theonet", "juneaudog");

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

float rainState[MAX_PIXELS];
void rain(uint32_t color, int maxdrops, int wait) {
  USE_SERIAL.printf("Rain: Called with color %x drops %d wait %d\n", color, maxdrops, wait);
  
  for (int i = 0; i < MAX_PIXELS; i++) {
    rainState[i] = 0;
  }
  for (int cycle = 0; cycle < maxdrops * 4; cycle++) {
    if (random(0, strip->numPixels()) <= maxdrops) {
      int p = random(0, strip->numPixels());
      rainState[p] = 1.0;
    }
    for (int i = 0; i < strip->numPixels(); i++) {
      if (rainState[i] > 0.0) {
        uint32_t tc = interpolate(0, color, rainState[i]);
        rainState[i] -= 0.05;
        strip->setPixelColor(i, tc);
      } else {
        strip->setPixelColor(i, 0);
      }
    }
    strip->show();
    delay(wait);
  }
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
  strip->show();
}

void fire(int cycles, int wait) {
  colorWipe(0, 0); // Set to black.

  int start = random(0, strip->numPixels());
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
      float fade = 1.0 - ((offset * 1.0) / (tail * 1.0)); // TODO(mdw): Make nonlinear.
      uint32_t tc = interpolate(0, color, fade);
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

// Run the current config.
void runConfig() {
  USE_SERIAL.println("runConfig mode: " + configMode);

  String cMode;
  uint32_t cColor;
  int cColorChange, cBrightness, cSpeed;
  bool cEnabled;

  // Read local copy of config to avoid holding mutex for too long.
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    cEnabled = configEnabled;
    cMode = configMode;
    cColor = strip->Color(configRed, configGreen, configBlue);
    cColorChange = configColorChange;
    cBrightness = configBrightness;
    cSpeed = configSpeed;
    xSemaphoreGive(configMutex);
  } else {
    // Can't get mutex to read config, just bail.
    USE_SERIAL.println("Warning - runConfig() unable to get config mutex.");
  }

  if (cMode == "random") {
    cMode = randomMode();
  }

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
    rain(cColor, NUMPIXELS , cSpeed);

  } else if (cMode == "comet") {
    comet(cColor, 8, cSpeed);

  } else if (cMode == "test") {
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
    configMode = (const String &)cc["mode"];
    configEnabled = (cc["enabled"] == true);
    configSpeed = cc["speed"];
    configBrightness = cc["brightness"];
    configColorChange = cc["colorChange"];
    configRed = cc["red"];
    configGreen = cc["green"];
    configBlue = cc["blue"];

    // If we have a new config for the number of pixels, reset the strip.
    if (configNumPixels != strip->numPixels()) {
      makeNewStrip(configNumPixels);
    }
    
    xSemaphoreGive(configMutex);
  } else {
    USE_SERIAL.println("Warning - readConfig() unable to get config mutex");
  }

  http.end();
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

void loop() {
  delay(1000);
}
