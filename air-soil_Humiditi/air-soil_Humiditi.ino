// Senzor pro půdní vlhkost (A0) a teplotu/vlhkost (DHT11) + WebSocket na ESP8266.
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Wi-Fi údaje
const char* WIFI_SSID = "FarmHub-AP";
const char* WIFI_PASS = "farmhub123";

// WebSocket údaje
const char* WS_HOST = "192.168.4.1";
const int   WS_PORT = 80;
const char* WS_PATH = "/ws";

// Pin DHT11 a definice typu
#define DHT_PIN  D2
#define DHTTYPE  DHT11

DHT dht(DHT_PIN, DHTTYPE);
WebSocketsClient webSocket;

// Interval pro odeslání dat (ms)
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
      webSocket.sendTXT("{\"sensorID\":\"soilDHTsensor\"}");
      break;
    case WStype_TEXT: {
      payload[length] = 0;
      String msg = (char*)payload;
      Serial.print("WS message: ");
      Serial.println(msg);
    } break;
    default:
      break;
  }
}

// setup()
void setup() {
  Serial.begin(115200);
  dht.begin();
  connectToWifi();
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
}

// loop()
void loop() {
  webSocket.loop();

  // Odesílání dat v intervalu
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();

    float soilVal = analogRead(A0);  
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Chyba čtení z DHT senzoru!");
      return;
    }

    // Přepočet 0..1023 -> odhad vlhkosti v %
    StaticJsonDocument<256> doc;
    doc["sensorID"] = "soilDHTsensor";
    doc["soil"]     = (100.0f - (soilVal / 10.23f)); 
    doc["temp"]     = temperature;
    doc["hum"]      = humidity;

    String out;
    serializeJson(doc, out);
    webSocket.sendTXT(out);

    Serial.print("Odesílám: ");
    Serial.println(out);
  }

  delay(20);
}
