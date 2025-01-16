#ifndef FARM_HUB_CONFIG_H
#define FARM_HUB_CONFIG_H

#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <time.h>

// Globální proměnné
static String homeSsid          = "";
static String homePass          = "";
static float  moistureThreshold = 40.0; 
static int    waterAmountML     = 100;
static bool   autoWatering      = false;

// Nastavení NTP pro ČR
static const long  gmtOffset_sec      = 3600;    
static const int   daylightOffset_sec = 0;       
static const char* ntpServer          = "pool.ntp.org";

// Inicializace SPIFFS
static inline bool initFileSystem() {
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount Failed!");
    return false;
  }
  return true;
}

// Uložení parametrů do /config.json
static inline void saveUserConfig() {
  StaticJsonDocument<256> doc;
  doc["homeSsid"]          = homeSsid;
  doc["homePass"]          = homePass;
  doc["moistureThreshold"] = moistureThreshold;
  doc["waterAmountML"]     = waterAmountML;
  doc["autoWatering"]      = autoWatering;

  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    Serial.println("Failed to open config.json for writing");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Config saved.");
}

// Načtení parametrů z /config.json
static inline void loadUserConfig() {
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("No config.json found, using defaults.");
    return;
  }
  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Failed to open config.json");
    return;
  }
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.println("Failed to parse config.json");
    return;
  }

  homeSsid          = doc["homeSsid"]          | "";
  homePass          = doc["homePass"]          | "";
  moistureThreshold = doc["moistureThreshold"] | 40.0;
  waterAmountML     = doc["waterAmountML"]     | 100;
  autoWatering      = doc["autoWatering"]      | false;

  Serial.println("Config loaded.");
}

// Nastavení času přes NTP
static inline void setupTimeFromNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Connecting to NTP...");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for time...");
    delay(1000);
  }

  Serial.println("Time synced.");
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  Serial.println(buf);
}

// Formátování epochového času
static inline String formatEpochTime(unsigned long epochSeconds) {
  time_t rawTime = (time_t)epochSeconds;
  struct tm* ti  = localtime(&rawTime);
  if (!ti) {
    return "N/A";
  }
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ti);
  return String(buf);
}

#endif // FARM_HUB_CONFIG_H
