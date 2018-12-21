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
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "Blinky.h"
#include "BlinkyModes.h"

// We use some magic strings in this constant to ensure that we can easily strip it out of the binary.
const char BUILD_VERSION[] = ("__Bl!nky__ " __DATE__ " " __TIME__ " " DEVICE_TYPE " " DEVICE_FORMFACTOR " ___");

WiFiMulti wifiMulti;
HTTPClient http;

StaticJsonDocument<1024> curConfigDocument;

deviceConfig_t curConfig = (deviceConfig_t) {
  "test",
  true,
  NUMPIXELS,
  DEFAULT_DATA_PIN,
  DEFAULT_CLOCK_PIN,
  0,   // Color change.
  100, // Brightness.
  100, // Speed.
  0,   // Color1.
  0,   // Color2.
  ""   // Firmware.
};
deviceConfig_t nextConfig;

#define TEST_CONFIG // Define for local testing.
#ifdef TEST_CONFIG
// Only used for testing.
deviceConfig_t testConfig = (deviceConfig_t) {
  "christmas",
  true,
  NUMPIXELS,
  DEFAULT_DATA_PIN,
  DEFAULT_CLOCK_PIN,
  5,   // Color change.
  20, // Brightness.
  100, // Speed.
  0xffff00,   // Color1.
  0xffb000,   // Color2.
  ""   // Firmware.
};
#endif

// We maintain the strip as a global variable mainly because it can be different types.
#ifdef USE_NEOPIXEL
Adafruit_NeoPixel *strip;
#else
Adafruit_DotStar *strip;
#endif

BlinkyMode *curMode = NULL;

// Used to protect access to curConfig / nextConfig.
SemaphoreHandle_t configMutex = NULL;

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

// Concurrent tasks.
void TaskCheckin(void *);
void TaskRunConfig(void *);

void makeNewStrip(int numPixels, int dataPin, int clockPin) {
  Serial.printf("Making new strip with %d pixels, data %d clock %d\n", numPixels, dataPin, clockPin);
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
  Serial.begin(115200);
  Serial.printf("Starting: %s\n", BUILD_VERSION);
  
  pinMode(LED_BUILTIN, OUTPUT);

  configMutex = xSemaphoreCreateMutex();
  memcpy(&nextConfig, &curConfig, sizeof(nextConfig));
  printConfig(&curConfig);
  printConfig(&nextConfig);

  makeNewStrip(NUMPIXELS, DEFAULT_DATA_PIN, DEFAULT_CLOCK_PIN);
  curMode = BlinkyMode::Create(&curConfig);

#if 0 // MDW: I don't think this is necessary.
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
#endif
  wifiMulti.addAP("theonet_EXT", "juneaudog");
  
  xTaskCreate(TaskCheckin, (const char *)"Checkin", 1024*40, NULL, 2, NULL);
  xTaskCreate(TaskRunConfig, (const char *)"Run config", 1024*40, NULL, 8, NULL);
  Serial.println("Done with setup()");
}

void printConfig(deviceConfig_t *config) {
  Serial.printf("Config [%lx]:\n", config);
  Serial.printf("  Mode: %s\n", config->mode);
  Serial.printf("  Enabled: %s\n", config->enabled ? "true" : "false"); 
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

void strobe(uint32_t c, int numsteps, uint8_t wait) {
  mixBetween(0, c, numsteps/2, wait);
  mixBetween(c, 0, numsteps/2, wait);
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

#if 0
void christmasRainbow(int wait) {
  for(uint16_t i=0; i < strip->numPixels(); i++) {
    uint32_t c;
    if (christmasState[i].wheelPos == 0) {
      christmasState[i].wheelPos = random(1, 255);
    }
    christmasState[i].wheelPos += 1;
    christmasState[i].wheelPos %= 255;
    c = Wheel(christmasState[i].wheelPos);
    strip->setPixelColor(i, c);
  }
  strip->show();
  delay(wait);
}
#endif

// Run the current config.
void runConfig() {
  Serial.printf("runConfig mode: %s\n", curConfig.mode);

  bool configChanged = false;
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    if (memcmp(&curConfig, &nextConfig, sizeof(curConfig))) {
      // Config has changed.
      Serial.printf("Config changed, old mode %s new mode %s\n", curConfig.mode, nextConfig.mode);
      memcpy(&curConfig, &nextConfig, sizeof(nextConfig));
      configChanged = true;
    }
    xSemaphoreGive(configMutex);
  } else {
    // Can't get mutex to read config, just bail.
    Serial.println("Warning - runConfig() unable to get config mutex.");
    return;
  }

  // If we have a new config for the number of pixels, reset the strip.
  if (curConfig.numPixels != strip->numPixels()) {
    Serial.println("Making new strip...");
    makeNewStrip(curConfig.numPixels, curConfig.dataPin, curConfig.clockPin);
  }

  if (configChanged) {
    delete curMode;
    curMode = BlinkyMode::Create(&curConfig);
  }

  curMode->run();
}

#if 0 // XXX XXX XXX MDW - Need to refactor the below:

  if (curConfig.colorChange > 0) {
    wheelPos += curConfig.colorChange;
    wheelPos = wheelPos % 255;
    curConfig.color1 = Wheel(wheelPos);
    if (curConfig.color2 != 0) {
      curConfig.color2 = Wheel((wheelPos + 128) % 255);
    }
  }

  Serial.printf("Running config: %s enabled %s\n",
    curConfig.mode,
    curConfig.enabled ? "true" : "false");

  strip->setBrightness(curConfig.brightness);

  if (!strcmp(curConfig.mode, "none") ||
      !strcmp(curConfig.mode, "off") ||
      !curConfig.enabled) {
    black();
    delay(1000);

  } else if (!strcmp(curConfig.mode, "wipe")) {
    colorWipe(curConfig.color1, curConfig.speed);
    colorWipe(0, curConfig.speed);
    
  } else if (!strcmp(curConfig.mode, "theater")) {
    theaterChase(curConfig.color1, curConfig.speed);
    
  } else if (!strcmp(curConfig.mode, "rainbow")) {
    strip->setBrightness(curConfig.brightness);
    rainbow(curConfig.speed);
    
  } else if (!strcmp(curConfig.mode, "rainbowCycle")) {
    rainbowCycle(curConfig.speed);
    
  } else if (!strcmp(curConfig.mode, "bounce")) {
    bounce(curConfig.color1, curConfig.speed);
    
  } else if (!strcmp(curConfig.mode, "strobe")) {
    strobe(curConfig.color1, 10, curConfig.speed);
    if (curConfig.color2 != 0) {
      strobe(curConfig.color2, 10, curConfig.speed);
    }

  } else if (!strcmp(curConfig.mode, "rain")) {
    rain(curConfig.color1, strip->numPixels(), curConfig.speed, 1.0, 1.0, 0.0, 0.05, 0.05, false, false);

  } else if (!strcmp(curConfig.mode, "snow")) {
    rain(curConfig.color1, strip->numPixels(), curConfig.speed, 0.02, 1.0, 0.0, 0.01, 0.2, false, false);

  } else if (!strcmp(curConfig.mode, "sparkle")) {
    rain(curConfig.color1, strip->numPixels(), curConfig.speed, 1.0, 1.0, 0.0, 0, 0.4, false, false);

  } else if (!strcmp(curConfig.mode, "shimmer")) {
    rain(curConfig.color1, 10, curConfig.speed, 0.1, 1.0, 0.0, 0.2, 0.05, true, false);

  } else if (!strcmp(curConfig.mode, "twinkle")) {
    rain(curConfig.color1, strip->numPixels(), curConfig.speed, 0.2, 0.8, 0.0, 0.1, 0.05, false, true);

  } else if (!strcmp(curConfig.mode, "comet")) {
    comet(curConfig.color1, 100, curConfig.speed);

  } else if (!strcmp(curConfig.mode, "candle")) {
    candle(curConfig.speed);

  } else if (!strcmp(curConfig.mode, "flicker")) {
    flicker(curConfig.color1, curConfig.brightness, curConfig.speed);

  } else if (!strcmp(curConfig.mode, "phantom")) {
    phantom(curConfig.color1, 5, 10, curConfig.speed);

/*
  } else if (!strcmp(curConfig.mode), "christmas")) {
    christmas(curConfig.speed, false, true);

  } else if (!strcmp(curConfig.mode), "christmasBoring")) {
    christmas(curConfig.speed, false, false);

  } else if (!strcmp(curConfig.mode), "christmasRandom")) {
    christmas(curConfig.speed, true, true);

  } else if (!strcmp(curConfig.mode), "christmasRainbow")) {
    christmasRainbow(curConfig.speed);
*/

  } else if (!strcmp(curConfig.mode, "test")) {
    strip->setBrightness(50);
    colorWipe(0xff0000, 5);
    colorWipe(0x00ff00, 5);
    colorWipe(0x0000ff, 5);
    colorWipe(0, 5);
    
  } else {
    Serial.printf("Unknown mode: %s\n", curConfig.mode);
    black();
    delay(1000);
  }
}
#endif // 0

void checkin() {
  Serial.print("MAC address ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address is ");
  Serial.println(WiFi.localIP().toString());

  String url = "https://team-sidney.firebaseio.com/checkin/" + WiFi.macAddress() + ".json";
  http.setTimeout(1000);
  http.begin(url);

  String curModeJson;
  StaticJsonDocument<1024> checkinDoc;
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
    Serial.println("Warning - checkin() unable to get config mutex.");
    return;
  }
  
  Serial.print("[HTTP] PUT " + url + "\n");
  Serial.print(payload + "\n");
  
  int httpCode = http.PUT(payload);
  if (httpCode > 0) {
    Serial.printf("[HTTP] Checkin response code: %d\n", httpCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void readConfig() {
  Serial.println("readConfig called");

#ifdef TEST_CONFIG
  memcpy(&nextConfig, &testConfig, sizeof(nextConfig));
  return;
#else
  
  String url = "https://team-sidney.firebaseio.com/strips/" + WiFi.macAddress() + ".json";
  http.setTimeout(10000);
  http.begin(url);

  Serial.print("[HTTP] GET " + url + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }
  
  String payload = http.getString();
  Serial.printf("[HTTP] readConfig response code: %d\n", httpCode);
  Serial.println(payload);

  bool needsFirmwareUpdate = false;

  // Parse JSON config.
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    DeserializationError err = deserializeJson(curConfigDocument, payload);
    Serial.print("Deserialize returned: ");
    Serial.println(err.c_str());

    JsonObject cc = curConfigDocument.as<JsonObject>();
    nextConfig.numPixels = cc["numPixels"];
    if (nextConfig.numPixels == 0) {
      nextConfig.numPixels = NUMPIXELS;
    }
    nextConfig.dataPin = cc["dataPin"];
    if (nextConfig.dataPin == 0) {
      nextConfig.dataPin = DEFAULT_DATA_PIN;
    }
    nextConfig.clockPin = cc["clockPin"];
    if (nextConfig.clockPin == 0) {
      nextConfig.clockPin = DEFAULT_CLOCK_PIN;
    }
    memcpy(nextConfig.mode, (const char *)cc["mode"], sizeof(nextConfig.mode));
    nextConfig.enabled = (cc["enabled"] == true);
    nextConfig.speed = cc["speed"];
    nextConfig.brightness = cc["brightness"];
    nextConfig.colorChange = cc["colorChange"];
    nextConfig.color1 = strip->Color(cc["red"], cc["green"], cc["blue"]);
    nextConfig.color2 = strip->Color(cc["red2"], cc["green2"], cc["blue2"]);
    memcpy(nextConfig.firmwareVersion, (const char *)cc["version"], sizeof(nextConfig.firmwareVersion));

    // If the firmware version needs to be updated, kick off the update.
    if (nextConfig.firmwareVersion != BUILD_VERSION &&
        nextConfig.firmwareVersion != "none" &&
        nextConfig.firmwareVersion != "" &&
        nextConfig.firmwareVersion != "current") {
      needsFirmwareUpdate = true;
    }
    
    xSemaphoreGive(configMutex);
  } else {
    Serial.println("Warning - readConfig() unable to get config mutex");
  }
  http.end();

  if (needsFirmwareUpdate) {
    updateFirmware();
  }
#endif // TEST_CONFIG
}

/* Task to periodically checkin and read new config. */
void TaskCheckin(void *pvParameters) {
  for (;;) {
#ifdef TEST_CONFIG
    readConfig();
#else
    if ((wifiMulti.run() == WL_CONNECTED)) {
      for (int i = 0; i < 5; i++) {
        flashLed();
      }
      checkin();
      readConfig();
    }
#endif // TEST_CONFIG
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
  Serial.println("readFirmwareMetadata called");
  
  String url = "https://team-sidney.firebaseio.com/firmware/" + firmwareVersion + ".json";
  http.setTimeout(1000);
  http.begin(url);

  Serial.print("[HTTP] GET " + url + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }
  
  String payload = http.getString();
  Serial.printf("[HTTP] readFirmwareMetadata response code: %d\n", httpCode);
  Serial.println(payload);

  // Parse JSON config.
  DeserializationError err = deserializeJson(firmwareVersionDocument, payload);
  Serial.print("Deserialize returned: ");
  Serial.println(err.c_str());

  JsonObject fwdoc = firmwareVersionDocument.as<JsonObject>();
  firmwareUrl = (const String &)fwdoc["url"];
  firmwareHash = (const String &)fwdoc["hash"];

  http.end();
}

void updateFirmware() {
  String newVersion;
  
  if (xSemaphoreTake(configMutex, (TickType_t )100) == pdTRUE) {
    newVersion = curConfig.firmwareVersion;
    xSemaphoreGive(configMutex);
  } else {
    Serial.println("Warning - updateFirmware() unable to get config mutex");
    return;
  }
  
  Serial.printf("Current firmware version: %s\n", BUILD_VERSION);
  Serial.println("Desired firmware version: " + newVersion);

  readFirmwareMetadata(newVersion);
  Serial.println("Read firmware metadata, URL is " + firmwareUrl);
  Serial.println("Hash " + firmwareHash);

  Serial.println("Starting OTA...");
  http.begin(firmwareUrl);

  Serial.print("[HTTP] GET " + firmwareUrl + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }

  int contentLen = http.getSize();
  Serial.printf("Content-Length: %d\n", contentLen);
  bool canBegin = Update.begin(contentLen);
  if (canBegin) {
    Serial.println("OK to start OTA.");
  } else {
    Serial.println("Not enough space to begin OTA");
    return;
  }

  WiFiClient* client = http.getStreamPtr();
  size_t written = Update.writeStream(*client);
  Serial.printf("OTA: %d/%d bytes written.\n", written, contentLen);
  if (written != contentLen) {
    Serial.println("Wrote partial binary. Giving up.");
    return;
  }

  if (Update.end()) {
    Serial.println("OTA done!");
  } else {
    Serial.println("Error from Update.end(): " + String(Update.getError()));
    return;
  }
  
  String md5 = Update.md5String();
  Serial.println("MD5: " + md5);
  
  if (Update.isFinished()) {
    Serial.println("Update successfully completed. Rebooting.");
    ESP.restart();
  } else {
    Serial.println("Error from Update.isFinished(): " + String(Update.getError()));
    return;
  }
}

void loop() {
  delay(60000);
}
