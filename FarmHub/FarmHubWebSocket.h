#ifndef FARM_HUB_WEBSOCKET_H
#define FARM_HUB_WEBSOCKET_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "FarmHubWebServer.h"
#include "FarmHubData.h"

// Jedna statická instance WebSocketu:
static AsyncWebSocket ws("/ws");

// ----------------------------------------------------------
// Pomocná funkce, která pošle VŠEM připojeným klientům JSON:
//   { "cmd":"RUN_PUMP", "duration": <durationSec> }
// ----------------------------------------------------------
void broadcastRunPump(int durationSec) {
  // Složíme JSON (ručně či přes ArduinoJson)
  // Tady to uděláme “ručně”:
  String msg = "{\"cmd\":\"RUN_PUMP\",\"duration\":";
  msg += durationSec;
  msg += "}";
  
  // Odeslání všem
  ws.textAll(msg);
}

// ----------------------------------------------------------
static inline void onWsEvent(AsyncWebSocket * server, 
                             AsyncWebSocketClient * client, 
                             AwsEventType type, 
                             void * arg, 
                             uint8_t *data, size_t len) 
{
  if(type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected from %s\n", 
                  client->id(), client->remoteIP().toString().c_str());
    client->text("{\"msg\":\"Welcome sensor!\"}");
  }
  else if(type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected.\n", client->id());
  }
  else if(type == WS_EVT_DATA) {
    data[len] = 0; 
    String payload = (char*)data;
    Serial.printf("WS data from #%u: %s\n", client->id(), payload.c_str());

    // Pokus o parse JSON senzorů (soil, temp, atd.)
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if(!err) {
      // Máme validní JSON, např.:
      // { "sensorID":"mySensor","soil":45.6,"temp":21.3,"hum":50,"light":123 }
      SensorReading sr;
      sr.sensorID     = doc["sensorID"]   | String("unknown");
      sr.soilMoisture = doc["soil"]       | 0.0;
      sr.temperature  = doc["temp"]       | 0.0;
      sr.humidity     = doc["hum"]        | 0.0;
      sr.lightLevel   = doc["light"]      | 0.0;
      sr.timestamp    = millis();

      storeSensorData(sr);

      // Odpovíme senzoru
      client->text("{\"status\":\"OK\"}");
    } else {
      // Chyba v JSON
      client->text("{\"status\":\"ERROR\",\"reason\":\"JSON parse\"}");
    }
  }
}

// ----------------------------------------------------------
static inline void startWebSocket() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  Serial.println("WebSocket /ws started.");
}

#endif // FARM_HUB_WEBSOCKET_H
