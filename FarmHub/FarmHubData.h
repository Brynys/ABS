#ifndef FARM_HUB_DATA_H
#define FARM_HUB_DATA_H

#include <Arduino.h>
#include <vector>
#include <FS.h>
#include <ArduinoJson.h>

/**
 * Pin pro čerpadlo. 
 * (Pokud hub už fyzicky čerpadlo nespíná, můžeš ho odstranit.)
 */
static const int PIN_PUMP = 5; 

/**
 * Struktura pro naměřené hodnoty
 */
struct SensorReading {
  String sensorID;
  float soilMoisture;
  float temperature;
  float humidity;
  float lightLevel;
  unsigned long timestamp;
};

// Buffer pro data v RAM
static std::vector<SensorReading> dataBuffer;

/**
 * Ukládáme data do RAM a CSV
 */
static inline void storeSensorData(const SensorReading &sr) {
  // Uložíme do RAM
  dataBuffer.push_back(sr);
  if(dataBuffer.size() > 300) {
    // Ořežeme staré
    dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + 100);
  }
  // Uložíme do souboru (CSV)
  File file = SPIFFS.open("/datalog.csv", "a");
  if(file) {
    file.printf("%s,%lu,%.2f,%.2f,%.2f,%.2f\n",
                sr.sensorID.c_str(),
                sr.timestamp,
                sr.soilMoisture,
                sr.temperature,
                sr.humidity,
                sr.lightLevel);
    file.close();
  }
}

/**
 * Čte z datalog.csv a vrací JSON s readings ve vybraném časovém období.
 */
static inline String getDataForPeriod(unsigned long fromMs, unsigned long toMs) {
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.createNestedArray("readings");

  File file = SPIFFS.open("/datalog.csv", "r");
  if(!file) {
    Serial.println("datalog.csv not found");
    return "";
  }
  while(file.available()) {
    String line = file.readStringUntil('\n');
    if(line.length() < 5) continue;

    // CSV: sensorID,timestamp,soil,temperature,humidity,light
    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    int idx3 = line.indexOf(',', idx2 + 1);
    int idx4 = line.indexOf(',', idx3 + 1);
    int idx5 = line.indexOf(',', idx4 + 1);
    if(idx5 < 0) continue;

    String sID = line.substring(0, idx1);
    unsigned long ts = line.substring(idx1 + 1, idx2).toInt();
    float sm = line.substring(idx2 + 1, idx3).toFloat();
    float te = line.substring(idx3 + 1, idx4).toFloat();
    float hu = line.substring(idx4 + 1, idx5).toFloat();
    float li = line.substring(idx5 + 1).toFloat();

    if(ts >= fromMs && ts <= toMs) {
      JsonObject obj = arr.createNestedObject();
      obj["sensorID"]     = sID;
      obj["timestamp"]    = ts;
      obj["soilMoisture"] = sm;
      obj["temperature"]  = te;
      obj["humidity"]     = hu;
      obj["lightLevel"]   = li;
    }
  }
  file.close();

  String output;
  serializeJson(doc, output);
  return output;
}

/**
 * Funkce z WebSocketu, kterou použijeme pro vyslání příkazu
 * “Zapni čerpadlo na X sekund” zavlažovacím klientům.
 */
extern void broadcastRunPump(int durationSec);

/**
 * V configu máme extern float moistureThreshold; 
 * extern int waterAmountML; (možná už nepotřebné)
 * 
 * NOVÉ:
 * extern float litersPerDay;           // kolik litrů za den
 * extern float wateringIntervalHours;  // co X hodin
 *
 * Pro time-based logiku potřebujeme pamatovat si, 
 * kdy proběhlo poslední zalití:
 */
static unsigned long lastWateringTime = 0;

/**
 * checkAndIrrigate:
 * - zkontroluje, zda uplynul nastavený interval
 * - pokud ano a vlhkost je menší než threshold, spočítá kolik litrů na session 
 *   a kolik sekund čerpadlo poběží, a vyšle broadcastRunPump(sec).
 */
static inline void checkAndIrrigate() {
  if(dataBuffer.empty()) return;

  // externy z FarmHubConfig.h
  extern float moistureThreshold;
  extern float litersPerDay;
  extern float wateringIntervalHours;

  auto &last = dataBuffer.back();

  // 1) Časová kontrola: Uplynulo aspoň wateringIntervalHours od posledního zalití?
  unsigned long now = millis();
  unsigned long intervalMs = (unsigned long)(wateringIntervalHours * 3600000UL);
  if((now - lastWateringTime) < intervalMs) {
    // Ještě není čas na další zalití
    return;
  }

  // 2) Kontrola vlhkosti půdy
  if(last.soilMoisture < moistureThreshold) {
    // Vypočítáme, kolik zaléváme při každém intervalu
    // timesPerDay = 24 / wateringIntervalHours
    float timesPerDay = 24.0f / wateringIntervalHours;
    float litersPerSession = litersPerDay / timesPerDay;

    // Čerpadlo 120 l/h -> 1 litr za 30 sekund
    // => sessionSeconds = litersPerSession * 30
    float sessionSeconds = litersPerSession * 30.0f;
    int secInt = (int)ceil(sessionSeconds);

    Serial.printf("[Hub] Soil=%.1f%% < %.1f%% => instructing pump for %d s\n",
                  last.soilMoisture, moistureThreshold, secInt);

    // Pošleme zavlažovacímu klientovi příkaz RUN_PUMP
    broadcastRunPump(secInt);

    // Zaznamenáme čas zalití
    lastWateringTime = now;
  }
}

#endif // FARM_HUB_DATA_H
