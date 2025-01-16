#ifndef FARM_HUB_CONFIG_H
#define FARM_HUB_CONFIG_H

#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <time.h>           // Důležité pro configTime(...) a getLocalTime(...)

// ----- Nastavení Wi-Fi -----
static String homeSsid = "";
static String homePass = "";

// ----- Nastavení prahu pro zalévání -----
static float moistureThreshold = 40.0;   
static int   waterAmountML     = 100;    
static bool  autoWatering      = false;  

// ----- Nastavení NTP (čas) -----
static const long  gmtOffset_sec      = 3600;   // pro ČR: UTC+1
static const int   daylightOffset_sec = 0;   // letní čas +1h
static const char* ntpServer          = "pool.ntp.org";

/**
 * Funkce pro inicializaci souborového systému (SPIFFS).
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
  doc["homeSsid"]          = homeSsid;
  doc["homePass"]          = homePass;
  doc["moistureThreshold"] = moistureThreshold;
  doc["waterAmountML"]     = waterAmountML;
  doc["autoWatering"]      = autoWatering;

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
  autoWatering      = doc["autoWatering"]      | false;

  Serial.println("Config loaded.");
}

/**
 * Nastavení času přes NTP. Volat až po připojení k Wi-Fi!
 */
static inline void setupTimeFromNTP() {
  // Zavoláme configTime pro synchronizaci s NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("Připojuji se na NTP server pro synchronizaci času...");

  // Počkáme, až se čas načte
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Čekám na nastavení času...");
    delay(1000);
  }

  Serial.println("Čas úspěšně synchronizován!");
  Serial.print("Lokální čas: ");
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  Serial.println(buf);
}

String formatEpochTime(unsigned long epochSeconds)
{
    time_t rawTime = (time_t)epochSeconds;
    struct tm *ti = localtime(&rawTime);
    if (!ti) {
        return String("N/A"); 
    }
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ti);
    return String(buf);
}

#endif // FARM_HUB_CONFIG_H
