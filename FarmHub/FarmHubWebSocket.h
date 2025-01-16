#ifndef FARM_HUB_WEBSOCKET_H
#define FARM_HUB_WEBSOCKET_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <time.h>
#include "FarmHubConfig.h"
#include "FarmHubWebServer.h"
#include "FarmHubData.h"

// Jedna instance WebSocketu na endpointu /ws
static AsyncWebSocket ws("/ws");

// Odeslání příkazu RUN_PUMP všem klientům (JSON: { "cmd":"RUN_PUMP", "duration":X })
void broadcastRunPump(int durationSec) {
  String msg = "{\"cmd\":\"RUN_PUMP\",\"duration\":";
  msg += durationSec;
  msg += "}";
  ws.textAll(msg);
}

// Obsluha událostí na WebSocketu
static inline void onWsEvent(AsyncWebSocket *server,
                             AsyncWebSocketClient *client,
                             AwsEventType type,
                             void *arg,
                             uint8_t *data,
                             size_t len) 
{
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client #%u connected from %s\n", 
                  client->id(), client->remoteIP().toString().c_str());
    client->text("{\"msg\":\"Welcome sensor!\"}");
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client #%u disconnected.\n", client->id());
  }
  else if (type == WS_EVT_DATA) {
    data[len] = 0;
    String payload = (char*)data;
    Serial.printf("Data from #%u: %s\n", client->id(), payload.c_str());

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      SensorReading sr;
      sr.sensorID     = doc["sensorID"]   | "unknown";
      sr.soilMoisture = doc["soil"]       | 0.0;
      sr.temperature  = doc["temp"]       | 0.0;
      sr.humidity     = doc["hum"]        | 0.0;
      sr.lightLevel   = doc["light"]      | 0.0;
      sr.timestamp    = (unsigned long)time(nullptr);
      storeSensorData(sr);
      client->text("{\"status\":\"OK\"}");
    } else {
      client->text("{\"status\":\"ERROR\",\"reason\":\"JSON parse\"}");
    }
  }
}

// Spuštění WebSocketu a jeho přidání do serveru
static inline void startWebSocket() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  Serial.println("WebSocket /ws started.");
}

#endif // FARM_HUB_WEBSOCKET_H
