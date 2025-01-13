/*
   Script pro odesílání dat o intenzitě světla (BH1750)
   přes WebSocket na ESP8266.

   Odesílá JSON ve tvaru:
   {
     "sensorID": "bh1750Sensor",
     "light": <float>
   }
*/

// ---------------------- Knihovny ----------------------
#include <ESP8266WiFi.h>         // WiFi
#include <WebSocketsClient.h>    // WebSocket klient
#include <ArduinoJson.h>         // JSON
#include <Wire.h>                // I2C
#include <BH1750.h>              // BH1750 senzor

// -- WIFI nastavení (podle vašich údajů) --
const char* WIFI_SSID = "FarmHub-AP";
const char* WIFI_PASS = "farmhub123";

// -- WebSocket nastavení --
const char* WS_HOST = "192.168.4.1";
const int   WS_PORT = 80;
const char* WS_PATH = "/ws";

// -- Objekt pro BH1750 senzor --
BH1750 lightMeter;

// -- WebSocket klient --
WebSocketsClient webSocket;

// -- Časování pro periodické odesílání dat (ms) --
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 5000; // např. 5 s (5000 ms)

// ------------------------------------------------------
//  Připojení k WiFi
// ------------------------------------------------------
void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  unsigned long start = millis();
  // 10 vteřinové okno pro připojení
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

// ------------------------------------------------------
//  Obsluha WebSocket událostí
// ------------------------------------------------------
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected!");
      break;

    case WStype_CONNECTED:
      Serial.println("WebSocket connected!");
      // Po připojení můžeme poslat jednorázovou identifikaci (volitelně)
      webSocket.sendTXT("{\"sensorID\":\"bh1750Sensor\"}");
      break;

    case WStype_TEXT: {
      // Zpracování příchozích zpráv/JSON příkazů
      payload[length] = 0;
      String msg = (char*)payload;
      Serial.print("WS message: ");
      Serial.println(msg);
      // Můžete si zde implementovat reakci na příkazy...
      }
      break;

    default:
      // Nezpracované typy
      break;
  }
}

// ------------------------------------------------------
//  setup()
// ------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);

  // Inicializace I2C
  Wire.begin();

  // Inicializace BH1750
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 se nepodařilo inicializovat, zkontrolujte zapojení!");
    while (1) { delay(10); }
  }
  Serial.println("BH1750 initializován.");

  // Připojení k WiFi
  connectToWifi();

  // Inicializace WebSocketu
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
}

// ------------------------------------------------------
//  loop()
// ------------------------------------------------------
void loop() {
  // WebSocket klient (nezbytné volat)
  webSocket.loop();

  // Periodické odesílání dat
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();

    // 1) Přečíst intenzitu světla v luxech
    float luxValue = lightMeter.readLightLevel();

    // 2) Vytvořit JSON s daty
    StaticJsonDocument<256> doc;
    doc["sensorID"] = "bh1750Sensor";
    doc["light"]    = luxValue;  // Hodnota v luxech

    // 3) Serializovat do stringu
    String out;
    serializeJson(doc, out);

    // 4) Odeslat přes WebSocket
    webSocket.sendTXT(out);

    // 5) Ladicí výpis do Serialu
    Serial.print("Odesílám: ");
    Serial.println(out);
  }

  // Drobná prodleva
  delay(10);
}
