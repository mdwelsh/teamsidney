#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define USE_SERIAL Serial

WiFiMulti wifiMulti;
HTTPClient http;
String* curMode = NULL;

void setup() {
  curMode = new String("none");
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
  String payload = "{\"timestamp\": {\".sv\": \"timestamp\"}, \"mac\": \"" + WiFi.macAddress() + "\", \"ip\": \"" + WiFi.localIP().toString() + "\", \"mode\": \"" + *curMode +"\"}";

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
    if (curMode != NULL) {
      delete curMode;
    }
    curMode = new String(http.getString());
    USE_SERIAL.println(*curMode);
  } else {
    USE_SERIAL.printf("[HTTP] failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void loop() {
  if ((wifiMulti.run() != WL_CONNECTED)) {
    delay(10000);
    return;
  }
  flashLed();
  checkin();
  readMode();
  USE_SERIAL.println("Current mode: " + *curMode + "\n");
  
  delay(30 * 1000);
}
