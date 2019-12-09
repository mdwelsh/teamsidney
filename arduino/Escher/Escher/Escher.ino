// Escher - CNC Etch-a-Sketch
// Matt Welsh <mdw@mdw.la>
// https://www.teamsidney.com

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "EscherStepper.h"
#include "EscherParser.h"

// This file defines DEVICE_SECRET, WIFI_NETWORK, and WIFI_PASSWORD.
#include "EscherDeviceConfig.h"

// Define this to reverse the axes (e.g., if using gears to mate between
// the steppers and the Etch-a-Sketch).
#define REVERSE_AXES
#ifdef REVERSE_AXES
#define FORWARD_STEP BACKWARD
#define BACKWARD_STEP FORWARD
#else
#define FORWARD_STEP FORWARD
#define BACKWARD_STEP BACKWARD
#endif

// These should be calibrated for each device.
#define ETCH_WIDTH 700
#define ETCH_HEIGHT 500
#define BACKLASH_X 10
#define BACKLASH_Y 15
#define MAX_SPEED 50.0

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myStepper1 = AFMS.getStepper(200, 1);
Adafruit_StepperMotor *myStepper2 = AFMS.getStepper(200, 2);
void forwardstep1() {
  myStepper1->onestep(FORWARD_STEP, SINGLE);
}
void backwardstep1() {
  myStepper1->onestep(BACKWARD_STEP, SINGLE);
}
void forwardstep2() {  
  myStepper2->onestep(FORWARD_STEP, DOUBLE);
}
void backwardstep2() {  
  myStepper2->onestep(BACKWARD_STEP, DOUBLE);
}

AccelStepper stepper1(forwardstep1, backwardstep1);
AccelStepper stepper2(forwardstep2, backwardstep2);
MultiStepper mstepper;
EscherStepper escher(mstepper, ETCH_WIDTH, ETCH_HEIGHT, BACKLASH_X, BACKLASH_Y);
EscherParser parser(escher);

// We use some magic strings in this constant to ensure that we can easily strip it out of the binary.
const char BUILD_VERSION[] = ("__E5ch3r__ " __DATE__ " " __TIME__ " ___");

WiFiMulti wifiMulti;
HTTPClient http;
File fsUploadFile;

// Flash the LED.
void flashLed(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
  }
}

enum EtchState { STATE_INITIALIZING = 0, STATE_IDLE, STATE_ETCHING };
EtchState etchState = STATE_INITIALIZING;
String curGcodeUrl = String("");

String etchStateString() {
  switch (etchState) {
    case STATE_INITIALIZING: return "initializing";
    case STATE_IDLE: return "idle";
    case STATE_ETCHING: return "etching";
    default: return "unknown";
  }
}

// Initialization code.
void setup() {  
  Serial.begin(115200);
  Serial.printf("Starting: %s\n", BUILD_VERSION);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize SPIFFS. We always format.
  if (!SPIFFS.format()) {
    Serial.println("Warning - SPIFFS format failed");  
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("Warning - SPIFFS mount failed");
  }
  showFilesystemContents();

  stepper1.setMaxSpeed(MAX_SPEED);
  stepper2.setMaxSpeed(MAX_SPEED);
  mstepper.addStepper(stepper1);
  mstepper.addStepper(stepper2);
  AFMS.begin(); // Start the bottom shield

  // Bring up WiFi.
  wifiMulti.addAP(WIFI_NETWORK, WIFI_PASSWORD);

  Serial.println("Done with setup()");
}

// Checkin to Firebase.
void checkin() {
  Serial.print("MAC address ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address is ");
  Serial.println(WiFi.localIP().toString());

  String url = "https://firestore.googleapis.com/v1beta1/projects/team-sidney/databases/(default)/documents:commit";
  http.setTimeout(1000);
  http.addHeader("Content-Type", "application/json");
  http.begin(url);

  // With Firestore, in order to get a server-generated timestamp,
  // we need to issue a 'commit' request with two operations: an
  // update and a transform. This results in some gnarly code.

  StaticJsonDocument<1024> checkinDoc;
  JsonObject root = checkinDoc.to<JsonObject>();
  JsonArray writes = root.createNestedArray("writes");

  // First entry - the update operation.
  JsonObject first = writes.createNestedObject();
  JsonObject update = first.createNestedObject("update");

  String docName = "projects/team-sidney/databases/(default)/documents/escher/root/secret/"
                   + DEVICE_SECRET + "/devices/" + WiFi.macAddress();
  update["name"] = docName;

  // Here's the contents of the document we're writing.
  JsonObject fields = update.createNestedObject("fields");
  JsonObject buildversion = fields.createNestedObject("version");
  buildversion["stringValue"] = BUILD_VERSION;
  JsonObject status = fields.createNestedObject("status");
  status["stringValue"] = etchStateString();
  JsonObject mac = fields.createNestedObject("mac");
  mac["stringValue"] = WiFi.macAddress();
  JsonObject ip = fields.createNestedObject("ip");
  ip["stringValue"] = WiFi.localIP().toString();
  JsonObject rssi = fields.createNestedObject("rssi");
  rssi["integerValue"] = String(WiFi.RSSI());
  JsonObject backlash_x = fields.createNestedObject("backlash_x");
  backlash_x["integerValue"] = String(BACKLASH_X);
  JsonObject backlash_y = fields.createNestedObject("backlash_y");
  backlash_y["integerValue"] = String(BACKLASH_Y);

  // Second entry - the transform operation.
  JsonObject second = writes.createNestedObject();
  JsonObject transform = second.createNestedObject("transform");
  transform["document"] = docName;
  JsonArray fieldTransforms = transform.createNestedArray("fieldTransforms");
  JsonObject st = fieldTransforms.createNestedObject();
  st["fieldPath"] = "updateTime";
  // This is the magic key/value to get Firebase to generate a server timestamp.
  st["setToServerValue"] = "REQUEST_TIME";
  JsonObject currentDocument = second.createNestedObject("currentDocument");
  currentDocument["exists"] = true;

  // Now serialize it.
  String payload;
  serializeJson(root, payload);

  Serial.print("[checkin] POST " + url + "\n");
  Serial.print(payload + "\n");

  int httpCode = http.sendRequest("POST", payload);
  if (httpCode == 200) {
    Serial.printf("[checkin] response code: %d\n", httpCode);
  } else {
    Serial.printf("[checkin] failed, status code %d: %s\n",
      httpCode, http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Download the given file from Firebase Storage and save it to the local filesystem.
bool downloadGcode(const char* url) {
  http.setTimeout(10000);
  http.begin(url);
  Serial.printf("[downloadGcode] GET %s\n", url);
  int httpCode = http.GET();
  Serial.printf("[downloadGcode] got response code: %d\n", httpCode);
  if (httpCode <= 0) {
    Serial.printf("[downloadGcode] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return false;
  }
  File outFile = SPIFFS.open("/data.gcd", FILE_WRITE);
  int bytesRead = http.writeToStream(&outFile);
  http.end();
  outFile.close();
  if (bytesRead < 0) {
    Serial.printf("[downloadGcode] Error writing to /data.gcd: %d\n", bytesRead);
    showFilesystemContents();
    return false;
  } else {
    Serial.printf("[downloadGcode] Wrote %d bytes to /data.gcd\n", bytesRead);
    showFilesystemContents();
    return true;
  }
}

// Start etching the currently downloaded file.
bool startEtching() {
  Serial.println("startEtching called");
  if (parser.Open("/data.gcd")) {
    parser.Prepare();
    stepper1.setCurrentPosition(0);
    stepper2.setCurrentPosition(0);
    stepper1.enableOutputs();
    stepper2.enableOutputs();
    Serial.println("startEtching - starting");
    etchState = STATE_ETCHING;
    checkin();
    return true;
  } else {
    Serial.println("startEtching - cannot open /data.gcd");
    checkin();
    return false;
  }
}

void stopEtching() {
  Serial.println("stopEtching - stopping");
  myStepper1->release();
  myStepper2->release();
  stepper1.disableOutputs();
  stepper2.disableOutputs();
}

// Read command from Firebase.
bool readCommand() {
  Serial.println("readCommand called");
  if (etchState != STATE_IDLE && etchState != STATE_ETCHING) {
    Serial.printf("readCommand - unexpected state %s\n", etchStateString());
    return false;
  }

  String url = String("https://firestore.googleapis.com/v1beta1/") +
               String("projects/team-sidney/databases/(default)/documents/escher/root/secret/") +
               DEVICE_SECRET + String("/devices/") + WiFi.macAddress() + String("/commands/etch");
  http.setTimeout(1000);
  http.addHeader("Content-Type", "application/json");
  http.begin(url);
  Serial.print("[readCommand] GET " + url + "\n");
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("[readCommand] failed, error: %s\n", http.errorToString(httpCode).c_str());
    return false;
  }

  String payload = http.getString();
  Serial.printf("[readCommand] response code: %d\n", httpCode);
  Serial.println(payload);
  http.end();

  if (httpCode != 200) {
    Serial.println("[readCommand] got non-200 response code");
    return false;
  }

  // Now, delete the etch document from Firestore, since even if there's an
  // error from this point forward, we don't want to process it again.
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  http.begin(url);
  Serial.print("[readCommand] DELETE " + url + "\n");
  httpCode = http.sendRequest("DELETE");
  if (httpCode <= 0) {
    Serial.printf("[readCommand] delete failed, error: %s\n", http.errorToString(httpCode).c_str());
    return false;
  }
  String deletePayload = http.getString();
  Serial.printf("[readCommand] delete response code: %d\n", httpCode);
  Serial.println(deletePayload);
  http.end();

  // Parse JSON object.
  StaticJsonDocument<1024> commandDoc;
  DeserializationError err = deserializeJson(commandDoc, payload);
  if (err) {
    Serial.println("readCommand - deserialize error:");
    Serial.println(err.c_str());
    return false;
  }

  JsonObject command = commandDoc["fields"].as<JsonObject>();
  if (!command.containsKey("command")) {
    Serial.println("readCommand - command doc missing required key command");
    return false;
  }
  String commandStr = command["command"]["stringValue"];
  Serial.printf("readCommand - commandStr is %s\n", commandStr);

  if (commandStr.equals("etch")) {
    if (!command.containsKey("url")) {
      Serial.println("readCommand - etch command missing required key url");
      return false;
    }
    String gcodeUrl = command["url"]["stringValue"];
    if (gcodeUrl.isEmpty()) {
      // Empty URL means to stop etching.
      if (etchState != STATE_ETCHING) {
        Serial.printf("readCommand - got stop command in state %s\n", etchStateString());
        return false;
      }
      stopEtching();
      return true;
    }

    long offsetLeft = 0;
    long offsetBottom = 0;
    float zoom = 1.0;
    bool scaleToFit = false;
    long etchWidth = ETCH_WIDTH;
    long etchHeight = ETCH_HEIGHT;
    long backlashX = BACKLASH_X;
    long backlashY = BACKLASH_Y;
    if (command.containsKey("offsetLeft")) {
      offsetLeft = command["offsetLeft"]["integerValue"];
    }
    if (command.containsKey("offsetBottom")) {
      offsetBottom = command["offsetBottom"]["integerValue"];
    }
    if (command.containsKey("zoom")) {
      if (command["zoom"].containsKey("doubleValue")) {
        zoom = command["zoom"]["doubleValue"];
      } else {
        zoom = (float)command["zoom"]["integerValue"];
      }
    }
    if (command.containsKey("scaleToFit")) {
      scaleToFit = command["scaleToFit"]["booleanValue"];
    }
    if (command.containsKey("etchWidth")) {
      etchWidth = command["etchWidth"]["integerValue"];
    }
    if (command.containsKey("etchHeight")) {
      etchHeight = command["etchHeight"]["integerValue"];
    }
    if (command.containsKey("backlashX")) {
      backlashX = command["backlashX"]["integerValue"];
    }
    if (command.containsKey("backlashY")) {
      backlashY = command["backlashY"]["integerValue"];
    }
    escher.reset();
    escher.setOffsetLeft(offsetLeft);
    escher.setOffsetBottom(offsetBottom);
    escher.setZoom(zoom);
    escher.setScaleToFit(scaleToFit);
    escher.setEtchWidth(etchWidth);
    escher.setEtchHeight(etchHeight);
    escher.setBacklashX(backlashX);
    escher.setBacklashY(backlashY);

    if (!curGcodeUrl.equals(gcodeUrl)) {
      // New URL to download, go grab it.
      if (!downloadGcode(gcodeUrl.c_str())) {
        Serial.printf("readCommand - got error downloading gCode %s\n", gcodeUrl);
        return false;
      }
      showFilesystemContents();
      curGcodeUrl = gcodeUrl;
      return startEtching();
    } else {
      Serial.println("readCommand - etch with same URL, ignoring");
      return true;
    }
  } else {
    Serial.printf("readCommand - Bad command %s\n", commandStr);
    return false;
  }
  Serial.println("readCommand - Should never get here");
  return false;
}

// Dump debug information on the filesystem contents.
void showFilesystemContents() {
  Serial.printf("SPIFFS: %d/%d bytes used\n", SPIFFS.usedBytes(), SPIFFS.totalBytes());
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    size_t fileSize = file.size();
    Serial.printf("FS file %s, size: %d\n", fileName.c_str(), fileSize);
    file = root.openNextFile();
  }
}

// Run the Etcher.
bool runEtcher() {
  // First see if the Escher controller is ready for more.
  if (escher.run()) {
    return true;
  }

  // Feed more commands to Escher. Returns false when file is complete.
  return parser.Feed();
}

unsigned long lastCheckin = 0;
unsigned long lastBlink = 0;
String gcodeUrl = "";
String gcodeHash = "";

// Main loop.
void loop() {
  if (etchState == STATE_INITIALIZING) {
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Waiting for WiFi connection...");
      delay(1000);
      return;
    } else {
      Serial.println("WiFi initialized, setting state to idle.");
      etchState = STATE_IDLE;
    }
    
  } else if (etchState == STATE_IDLE) {
    if (millis() - lastCheckin >= 10000) {
      lastCheckin = millis();
      checkin();
      if (!readCommand()) {
        etchState = STATE_IDLE;
      }
    }
  } else if (etchState == STATE_ETCHING) {
    if (!runEtcher()) {
      Serial.println("Etcher completed.");
      stopEtching();
      etchState = STATE_IDLE;
      checkin();
    }
  }
}