#ifndef FARM_HUB_ESP32CAM_H
#define FARM_HUB_ESP32CAM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "FarmHubData.h"
#include "FarmHubConfig.h"

// Globální proměnná pro uložení poslední obdržené fotografie ve formátu base64
static String latestImageBase64 = "";

//-----------------------------------------------------------
// Funkce pro získání URL aktuální fotografie z ESP32-CAM
// Vrací data URL, kterou lze přímo vložit do tagu <img>
//-----------------------------------------------------------
inline String getCurrentImageURL() {
  if (latestImageBase64.length() == 0) {
    Serial.print("Není obsah latestImageBase64");
    return "";
  }
  Serial.print("Vytvářím string s img do webu");
  return "data:image/jpeg;base64," + latestImageBase64;
}

//-----------------------------------------------------------
// Funkce pro předání dat senzorů do ESP32-CAM
// Hub nemůže kameru kontaktovat, proto tato funkce jen vypíše upozornění.
//-----------------------------------------------------------
inline void forwardDataToESP32CAM(const SensorReading &sr) {
  Serial.println("forwardDataToESP32CAM: Hub nemůže kontaktovat kameru přímo.");
  // Pokud chcete data odeslat, implementujte alternativní řešení (např. HTTP POST apod.)
}

//-----------------------------------------------------------
// Funkce pro načtení archivovaných fotografií z ESP32-CAM
//-----------------------------------------------------------
inline String getArchivedPhotos() {
  return "";
}

//-----------------------------------------------------------
// Funkce pro získání senzorových dat (stub)
//-----------------------------------------------------------
static inline String getSensorData() {
  return "{\"readings\":[]}";
}

//-----------------------------------------------------------
// Funkce pro registraci HTTP endpointu pro přijetí fotografie z kamery
//-----------------------------------------------------------
static inline void registerImageUploadEndpoint(AsyncWebServer &server) {
  // Tento endpoint přijímá POST požadavky s JSON tělem, které obsahuje pole "image"
  Serial.print("R.I.U.E.");
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Odpověď se odešle po zpracování těla požadavku
      request->send(200, "application/json", "{\"status\":\"received\"}");
    },
    NULL,
    // onBody handler – akumulace a zpracování těla požadavku
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Pro jednoduchost zde používáme statickou proměnnou – 
      // tato implementace předpokládá, že současně nebude zpracováván více požadavků
      static String bodyBuffer;
      //if (index == 0) {
      //  bodyBuffer = "";
      //}
      // Přidáme aktuální chunk do akumulačního bufferu
      bodyBuffer += String((char*)data).substring(0, len);
      
      // Pokud ještě nejsou přijata všechna data, ukončíme zpracování
      if (index + len < total) {
        return;
      }
      
      // Všechny data byly obdrženy, nyní zkusíme JSON parsovat
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, bodyBuffer);
      if (!error) {
        if (doc.containsKey("image")) {

          //*/String prefix = "image:";
          //if (latestImageBase64.startsWith(prefix)) {
          //    latestImageBase64 = latestImageBase64.substring(prefix.length());
          //}
          String latestImageBase64 = doc["image"].as<String>();

          Serial.println("Přijata nová fotografie přes HTTP POST.");
        }
      } else {
        Serial.println("Chyba při parsování JSONu v /upload");
      }
    }
  );
  Serial.print("R.I.U.E.");
}

#endif // FARM_HUB_ESP32CAM_H
