#ifndef FARM_HUB_DATA_H
#define FARM_HUB_DATA_H

#include <Arduino.h>
#include <vector>
#include <FS.h>
#include <ArduinoJson.h>

static const int PIN_PUMP = 5; // Pin pro čerpadlo

// Struktura pro hodnoty senzorů
struct SensorReading {
  String sensorID;
  float  soilMoisture;
  float  temperature;
  float  humidity;
  float  lightLevel;
  unsigned long timestamp;
};

// Buffer naměřených dat v RAM
static std::vector<SensorReading> dataBuffer;

void forwardDataToESP32CAM(const SensorReading &sr);

// Uložení dat do RAM + CSV
static inline void storeSensorData(const SensorReading &sr) {
  // Uložení do bufferu a CSV
  dataBuffer.push_back(sr);
  if (dataBuffer.size() > 300) {
    dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + 100);
  }
  File file = SPIFFS.open("/datalog.csv", "a");
  if (file) {
    file.printf("%s,%lu,%.2f,%.2f,%.2f,%.2f\n",
                sr.sensorID.c_str(),
                sr.timestamp,
                sr.soilMoisture,
                sr.temperature,
                sr.humidity,
                sr.lightLevel);
    file.close();
  }

  // Pokud chcete data navíc předat do ESP32-CAM, odkomentujte následující řádek:
  // forwardDataToESP32CAM(sr);
}

// Vrací JSON se záznamy z CSV v daném časovém intervalu
static inline String getDataForPeriod(unsigned long fromMs, unsigned long toMs) {
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.createNestedArray("readings");

  File file = SPIFFS.open("/datalog.csv", "r");
  if (!file) {
    Serial.println("datalog.csv not found");
    return "";
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() < 5) continue;

    // CSV: sensorID,timestamp,soil,temperature,humidity,light
    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    int idx3 = line.indexOf(',', idx2 + 1);
    int idx4 = line.indexOf(',', idx3 + 1);
    int idx5 = line.indexOf(',', idx4 + 1);
    if (idx5 < 0) continue;

    String sID = line.substring(0, idx1);
    unsigned long ts = line.substring(idx1 + 1, idx2).toInt();
    float sm = line.substring(idx2 + 1, idx3).toFloat();
    float te = line.substring(idx3 + 1, idx4).toFloat();
    float hu = line.substring(idx4 + 1, idx5).toFloat();
    float li = line.substring(idx5 + 1).toFloat();

    if (ts >= fromMs && ts <= toMs) {
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

// Funkce z WebSocketu pro spuštění čerpadla
extern void broadcastRunPump(int durationSec);

// Kontrola vlhkosti a případné spuštění čerpadla
static inline void checkAndIrrigate() {
  if (dataBuffer.empty()) return;

  extern bool  autoWatering;
  extern float moistureThreshold;
  extern int   waterAmountML;

  if (!autoWatering) return;

  const SensorReading* lastSoilReading = nullptr;
  for (auto it = dataBuffer.rbegin(); it != dataBuffer.rend(); ++it) {
    if (it->sensorID == "soilDHTsensor") {
      lastSoilReading = &(*it);
      break;
    }
  }
  if (!lastSoilReading) return;

  if (lastSoilReading->soilMoisture < moistureThreshold) {
    double secondsFloat = waterAmountML * 0.03; 
    int secInt = (int)round(secondsFloat);
    Serial.printf("[Auto] Soil=%.1f%% < %.1f%% => Pump for %d s\n",
                  lastSoilReading->soilMoisture, moistureThreshold, secInt);
    broadcastRunPump(secInt);
  }
}

#endif // FARM_HUB_DATA_H
