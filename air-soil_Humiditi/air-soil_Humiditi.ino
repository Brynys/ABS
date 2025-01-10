/*
   Script pro odesílání dat o půdní vlhkosti (A0) a teploty/vlhkosti vzduchu (DHT11)
   přes WebSocket na ESP8266.

   Odesílá JSON ve tvaru například:
   {
     "sensorID": "soilDHTsensor",
     "soil": <analogValue 0..1023>,
     "temp": <float>,
     "hum": <float>
   }
*/

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// -- WIFI nastavení (podle vašich údajů) --
const char* WIFI_SSID = "FarmHub-AP";
const char* WIFI_PASS = "farmhub123";

// -- WebSocket nastavení (podle vašeho serveru) --
const char* WS_HOST = "192.168.4.1";
const int   WS_PORT = 80;
const char* WS_PATH = "/ws";

// -- Piny pro čidla --
#define DHT_PIN  D2
#define DHTTYPE  DHT11

DHT dht(DHT_PIN, DHTTYPE);
WebSocketsClient webSocket;

// -- Časování pro periodické odesílání dat --
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 15000; // 5 sekund

// -------------------------------------------------------------------
// Připojení k WiFi
// -------------------------------------------------------------------
void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
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

// -------------------------------------------------------------------
// Obsluha WebSocket událostí
// -------------------------------------------------------------------
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected!");
      break;

    case WStype_CONNECTED:
      Serial.println("WebSocket connected!");
      // Po připojení můžeme poslat jednorázovou identifikaci
      webSocket.sendTXT("{\"sensorID\":\"soilDHTsensor\"}");
      break;

    case WStype_TEXT: {
      // Pokud chceme zpracovávat příchozí zprávy (třeba příkazy), můžeme tady
      payload[length] = 0; // ukončení řetězce
      String msg = (char*)payload;
      Serial.print("WS message: ");
      Serial.println(msg);
      // Případné zpracování JSON, příkazy apod...
      }
      break;

    default:
      // Nezpracované typy
      break;
  }
}

// -------------------------------------------------------------------
// setup()
// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Inicializace DHT senzoru
  dht.begin();

  // Připojení k WiFi
  connectToWifi();

  // Inicializace WebSocketu
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
}

// -------------------------------------------------------------------
// loop()
// -------------------------------------------------------------------
void loop() {
  // WebSocket klient musí být ve smyčce
  webSocket.loop();

  // Každých SEND_INTERVAL ms přečteme data a odešleme
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();

    // 1) Přečíst půdní vlhkost z A0 (analog)
    //    Hodnota je 0..1023
    int soilVal = analogRead(A0);

    // 2) Přečíst teplotu a vlhkost z DHT11
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();

    // Ověřit, zda DHT vrátil validní data
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Chyba čtení z DHT senzoru!");
      return; 
    }

    // 3) Vytvořit JSON s daty
    StaticJsonDocument<256> doc;
    doc["sensorID"] = "soilDHTsensor";
    doc["soil"]     = soilVal;      // 0..1023
    doc["temp"]     = temperature;  // °C
    doc["hum"]      = humidity;     // %

    // 4) Serializovat do stringu
    String out;
    serializeJson(doc, out);

    // 5) Odeslat přes WebSocket
    webSocket.sendTXT(out);

    // -- Ladicí výpis do Serialu --
    Serial.print("Odesílám: ");
    Serial.println(out);
  }

  // Krátká pauza, aby smyčka neběžela příliš rychle
  delay(20);
}
