#ifndef FARM_HUB_CONFIG_H
#define FARM_HUB_CONFIG_H

#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

// ----- Nastavení Wi-Fi -----
static String homeSsid = "";
static String homePass = "";

// ----- Nastavení prahu pro zalévání -----
static float moistureThreshold = 40.0;
static int   waterAmountML     = 100;

// ----- Nové proměnné pro řízení zavlažování -----
static float litersPerDay          = 10.0;  // kolik litrů se má za den využít
static float wateringIntervalHours = 6.0;   // jak často (po kolika hodinách) se zalévá

/**
 * Inicializace souborového systému (SPIFFS)
 */
static inline bool initFileSystem() {
  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount Failed!");
    return false;
  }
  return true;
}

/**
 * Uložení configu do /config.json
 */
static inline void saveUserConfig() {
  StaticJsonDocument<256> doc;
  doc["homeSsid"]             = homeSsid;
  doc["homePass"]             = homePass;
  doc["moistureThreshold"]    = moistureThreshold;
  doc["waterAmountML"]        = waterAmountML;

  // Uložíme i nové proměnné:
  doc["litersPerDay"]         = litersPerDay;
  doc["wateringIntervalHours"] = wateringIntervalHours;

  File file = SPIFFS.open("/config.json", "w");
  if(!file) {
    Serial.println("Failed to open config.json for writing");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Config saved.");
}

/**
 * Načtení configu z /config.json
 */
static inline void loadUserConfig() {
  if(!SPIFFS.exists("/config.json")) {
    Serial.println("No config.json found, using defaults.");
    return;
  }
  File file = SPIFFS.open("/config.json", "r");
  if(!file) {
    Serial.println("Failed to open config.json");
    return;
  }
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if(err) {
    Serial.println("Failed to parse config.json");
    return;
  }

  homeSsid          = doc["homeSsid"]          | "";
  homePass          = doc["homePass"]          | "";
  moistureThreshold = doc["moistureThreshold"] | 40.0;
  waterAmountML     = doc["waterAmountML"]     | 100;

  // Načteme i nové proměnné:
  litersPerDay          = doc["litersPerDay"]         | 10.0;
  wateringIntervalHours = doc["wateringIntervalHours"] | 6.0;

  Serial.println("Config loaded.");
}

#endif // FARM_HUB_CONFIG_H
