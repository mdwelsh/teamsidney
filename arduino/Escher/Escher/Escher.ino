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
#include "EscherStepper.h"

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

// We use some magic strings in this constant to ensure that we can easily strip it out of the binary.
const char BUILD_VERSION[] = ("__E5ch3r__ " __DATE__ " " __TIME__ " ___");

WiFiMulti wifiMulti;
HTTPClient http;
WiFiServer server(80);

void flashLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

// Tasks.
void TaskCheckin(void *);
//void TaskHTTPServer(void *);
//void TaskEtch(void *);

void setup() {  
  Serial.begin(115200);
  Serial.printf("Starting: %s\n", BUILD_VERSION);
  pinMode(LED_BUILTIN, OUTPUT);

  stepper1.setMaxSpeed(MAX_SPEED);
  stepper2.setMaxSpeed(MAX_SPEED);
  mstepper.addStepper(stepper1);
  mstepper.addStepper(stepper2);
  AFMS.begin(); // Start the bottom shield

  wifiMulti.addAP("theonet_EXT", "juneaudog");

  xTaskCreate(TaskCheckin, (const char *)"Checkin", 1024*40, NULL, 2, NULL);
  xTaskCreate(TaskHTTPServer, (const char *)"WiFi server", 1024*40, NULL, 4, NULL);
  //xTaskCreate(TaskEtch, (const char *)"Etch", 1024*40, NULL, 8, NULL);
  Serial.println("Done with setup()");
}

void loop() {
  // Do nothing.
  delay(60000);
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

/* Task to periodically checkin and read new config. */
void TaskCheckin(void *pvParameters) {
  for (;;) {
    if ((wifiMulti.run() == WL_CONNECTED)) {
      for (int i = 0; i < 5; i++) {
        flashLed();
      }
      checkin();
    } else {
      Serial.println("TaskCheckin: Not connected to WiFi");
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void handleHttpRequest(WiFiClient client) {
  Serial.println("HTTP connection from " + client.remoteIP().toString());

  String currentLine = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);
      if (c == '\n') {
        if (currentLine.length() == 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type: text/html");
          client.println();
          client.println("<html><body>You're connected to Escher, isn't this cool?</body></html>");
          client.println();
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  // close the connection:
  client.stop();
}

void TaskHTTPServer(void *pvParameters) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
  }
  server.begin();
  Serial.println("Started HTTP server on http://" + WiFi.localIP().toString() + ":80/");
 
  for (;;) {
    WiFiClient client = server.available();
    if (client) {
      handleHttpRequest(client);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
