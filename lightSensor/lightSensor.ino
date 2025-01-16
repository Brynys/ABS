// Senzor BH1750 (intenzita světla) + WebSocket na ESP8266.
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <BH1750.h>

// Wi-Fi údaje
const char* WIFI_SSID = "FarmHub-AP";
const char* WIFI_PASS = "farmhub123";

// WebSocket údaje
const char* WS_HOST = "192.168.4.1";
const int   WS_PORT = 80;
const char* WS_PATH = "/ws";

BH1750 lightMeter;
WebSocketsClient webSocket;

// Interval pro odeslání dat
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 90000; // 90 s

// Připojení k Wi-Fi
void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi NOT connected (timeout).");
  }
}

// WebSocket události
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket connected!");
      webSocket.sendTXT("{\"sensorID\":\"bh1750Sensor\"}");
      break;
    case WStype_TEXT:
      payload[length] = 0;
      Serial.print("WS message: ");
      Serial.println((char*)payload);
      break;
    default:
      break;
  }
}

// setup()
void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 init error!");
    while (1) { delay(10); }
  }
  Serial.println("BH1750 ready.");

  connectToWifi();
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
}

// loop()
void loop() {
  webSocket.loop();

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();

    float luxValue = lightMeter.readLightLevel();

    StaticJsonDocument<256> doc;
    doc["sensorID"] = "lightsensor";
    doc["light"]    = luxValue;

    String out;
    serializeJson(doc, out);
    webSocket.sendTXT(out);

    Serial.print("Odesílám: ");
    Serial.println(out);
  }

  delay(10);
}
