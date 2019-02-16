// Escher - CNC Etch-a-Sketch
// Matt Welsh <mdw@mdw.la>
// https://www.teamsidney.com

#define DEBUG_ESP_HTTP_SERVER

#include <vector>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "EscherStepper.h"
#include "EscherParser.h"

// Format flash filesystem if it can't be mounted, e.g., due to being the first run.
#define FORMAT_SPIFFS_IF_FAILED true

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
#define BACKLASH_X 10
#define BACKLASH_Y 15
#define MAX_SPEED 100.0

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
EscherStepper escher(mstepper, BACKLASH_X, BACKLASH_Y);
EscherParser parser(escher);

// We use some magic strings in this constant to ensure that we can easily strip it out of the binary.
const char BUILD_VERSION[] = ("__E5ch3r__ " __DATE__ " " __TIME__ " ___");

WiFiMulti wifiMulti;
HTTPClient http;
WebServer server(80);
File fsUploadFile;

void flashLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

enum EtchState { STATE_INITIALIZING, STATE_IDLE, STATE_READY, STATE_ETCHING, STATE_PAUSED };
EtchState etchState = STATE_INITIALIZING;

void setup() {  
  Serial.begin(115200);
  Serial.printf("Starting: %s\n", BUILD_VERSION);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize SPIFFS.
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("Warning - SPIFFS Mount Failed");
  }
  showFilesystemContents();

  stepper1.setMaxSpeed(MAX_SPEED);
  stepper2.setMaxSpeed(MAX_SPEED);
  mstepper.addStepper(stepper1);
  mstepper.addStepper(stepper2);
  AFMS.begin(); // Start the bottom shield

  // Bring up WiFi.
  wifiMulti.addAP("theonet_EXT", "juneaudog");

  // Configure HTTP server.
  server.on("/", handleRoot);
  server.on("/upload", HTTP_OPTIONS, handleOptions);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUpload);
  server.on("/etch", handleEtch);
  server.on("/pause", handlePause);
  server.onNotFound(handleNotFound);

  Serial.println("Done with setup()");
}

// Checkin to Firebase.
void checkin() {
  Serial.print("MAC address ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address is ");
  Serial.println(WiFi.localIP().toString());

  //String url = "https://firestore.googleapis.com/v1/projects/team-sidney/databases/(default)/documents/escher/root/devices/" + WiFi.macAddress();
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
  String docName = "projects/team-sidney/databases/(default)/documents/escher/root/devices/" + WiFi.macAddress();
  update["name"] = docName;
  JsonObject fields = update.createNestedObject("fields");
  JsonObject buildversion = fields.createNestedObject("version");
  buildversion["stringValue"] = BUILD_VERSION;
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

  Serial.print("[HTTP] POST " + url + "\n");
  Serial.print(payload + "\n");

  int httpCode = http.sendRequest("POST", payload);
  if (httpCode == 200) {
    Serial.printf("[HTTP] Checkin response code: %d\n", httpCode);
    //String payload = http.getString();
    //Serial.println(payload);
  } else {
    Serial.printf("[HTTP] failed, status code %d: %s\n",
      httpCode, http.errorToString(httpCode).c_str());
  }
  http.end();
}

void showFilesystemContents() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    size_t fileSize = file.size();
    Serial.printf("FS file %s, size: %d\n", fileName.c_str(), fileSize);
    file = root.openNextFile();
  }
}

// HTTP request handlers.

void handleRoot() {
  Serial.println("Server: handleRoot called");
  server.send(200, "text/html", "<html><body>You're talking to Escher!</body></html>");
}

void handleNotFound() {
  Serial.println("Server: handleNotFound called for " + server.uri());
  Serial.printf("Method: ");
  switch (server.method()) {
    case HTTP_GET: Serial.println("GET"); break;
    case HTTP_POST: Serial.println("POST"); break;
    default: Serial.println("other"); break;
  }
  Serial.printf("Number of args: %d\n", server.args());
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.printf("  arg[%d] = %s\n", i, server.argName(i).c_str());
    Serial.println(server.arg(i).c_str());
  }
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Permit CORS.
  server.send(404, "text/plain", "Not found");
}

void handleUpload() {
  Serial.println("handleUpload called");
  if (etchState == STATE_ETCHING) {
    Serial.println("Warning - cannot accept upload while etching.");
    return;
  }

  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.printf("handleUpload filename: %s\n", filename.c_str());
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("handleUpload received %d bytes\n", upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    Serial.printf("handleUpload completed write of %d bytes\n", upload.totalSize);
  }
}

void handleUploadDone() {
  Serial.println("handleUploadDone called");
  showFilesystemContents();
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Permit CORS.

  if (etchState == STATE_ETCHING) {
    server.send(500, "text/plain", "Upload rejected - already etching");
  } else {
    etchState = STATE_READY;
    server.send(200, "text/plain", "Thanks for the file.");
  }
}

void handleOptions() {
  Serial.println("handleOptions called");
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Permit CORS.
  server.send(200, "text/plain", "OK");
}

void handleEtch() {
  Serial.println("handleEtch called");
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Permit CORS.

  // First check that we are ready.
  if (!(etchState == STATE_READY || etchState == STATE_PAUSED)) {
    server.send(500, "text/plain", "State must be ready or paused to start etching");
  }

  if (parser.Open("/cmddata.txt")) {
    server.send(200, "text/plain", "Etching started.");
    etchState = STATE_ETCHING;
  } else {
    server.send(500, "text/plain", "No cmddata.txt found -- use /upload first");
    etchState = STATE_IDLE;
  }
}

void handlePause() {
  Serial.println("handlePause called");
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Permit CORS.

  // First check that we are etching.
  if (etchState != STATE_ETCHING) {
    server.send(500, "text/plain", "State must be etching to pause");
  } else {
    server.send(200, "text/plain", "Pausing");
    etchState = STATE_PAUSED;
  }
}

bool runEtcher() {
  // First see if the Escher controller is ready for more.
  if (escher.run()) {
    return true;
  }

  // Feed more commands to Escher. Returns false when file is complete.
  return parser.Feed();
}

// Main loop.
void loop() {
  unsigned long lastCheckin = 0;

  if (etchState == STATE_INITIALIZING) {
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Waiting for WiFi connection...");
      delay(1000);
      return;
    } else {
      server.begin();
      Serial.println("Started HTTP server on http://" + WiFi.localIP().toString() + ":80/");
      etchState = STATE_IDLE;
    }
    
  } else if (etchState == STATE_IDLE || etchState == STATE_READY || etchState == STATE_PAUSED) {
    // Do periodic checkins.
    if (millis() - lastCheckin >= 10000) {
      lastCheckin = millis();
      checkin();
    }
    server.handleClient();   
     
  } else if (etchState == STATE_ETCHING) {
    // Avoid doing checkins; run etcher but poll for HTTP pause commands.
    if (!runEtcher()) {
      Serial.println("Etcher completed.");
      etchState = STATE_IDLE;
    }
    server.handleClient();
  }
}
