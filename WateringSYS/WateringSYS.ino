/*
  Script pro ovládání čerpadla přes WebSocket.
  - Připojuje se na WiFi
  - Připojuje se k WebSocket serveru
  - Přijímá JSON s "cmd":"RUN_PUMP" a "duration"
  - Zapne čerpadlo nastavením D2 do OUTPUT HIGH
  - Vypne čerpadlo nastavením D2 do INPUT
*/

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "FarmHub-AP";
const char* WIFI_PASS = "farmhub123";

const char* WS_HOST  = "192.168.4.1";
const int   WS_PORT  = 80;
const char* WS_PATH  = "/ws";

#define PUMP_PIN 4

WebSocketsClient webSocket;

bool pumpRunning = false;
unsigned long pumpEndTime = 0;

void pumpOn() {
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);
}

void pumpOff() {
  pinMode(PUMP_PIN, INPUT);
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(200);
  }
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    webSocket.sendTXT("{\"sensorID\":\"pumpClient\"}");
  }
  else if (type == WStype_TEXT) {
    payload[length] = 0;
    String msg = (char*)payload;
    StaticJsonDocument<200> doc;
    if (!deserializeJson(doc, msg)) {
      String cmd = doc["cmd"]      | "";
      int durationSec = doc["duration"] | 0;
      if (cmd.equals("RUN_PUMP")) {
        if (durationSec > 60) durationSec = 60;
        pumpOn();
        pumpRunning = true;
        pumpEndTime = millis() + (unsigned long)durationSec * 1000UL;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pumpOff();
  connectToWifi();
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  if (pumpRunning && millis() >= pumpEndTime) {
    pumpOff();
    pumpRunning = false;
  }
  delay(20);
}
