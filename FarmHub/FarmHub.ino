#include <Arduino.h>
#include "FarmHubWiFi.h"
#include "FarmHubDisplay.h"
#include "FarmHubConfig.h"
#include "FarmHubData.h"
#include "FarmHubWebServer.h"
#include "FarmHubWebSocket.h"
#include "FarmHubESP32CAM.h"

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 30000;

void setup() {
  Serial.begin(115200);
  delay(200);

  initFileSystem();
  loadUserConfig();

  initDisplay();
  displayInfo("Starting...", "");

  // Nastartujeme AP
  setupWifiAP(); 
  bool wifiOK = connectToHomeWiFi(homeSsid, homePass);
  if (wifiOK) {
    displayInfo("WiFi OK", WiFi.localIP().toString());
    setupTimeFromNTP();
  } else {
    displayInfo("WiFi fail", "AP only");
  }

  // Spustíme webserver, websocket
  startAsyncWebServer();
  startWebSocket();

  registerImageUploadEndpoint(server);
  Serial.println("konec Setup");
}

void updateDisplayWithSensorData() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 2000) {
    return; // Aktualizuje jen každé 2s
  }
  lastUpdate = millis();

  // Najdeme poslední měření "soilDHTsensor" a "lightsensor"
  SensorReading lastSoilDHT;
  bool foundSoilDHT = false;
  SensorReading lastLight;
  bool foundLight   = false;

  for (int i = dataBuffer.size() - 1; i >= 0; i--) {
    const auto &d = dataBuffer[i];
    if (!foundSoilDHT && d.sensorID == "soilDHTsensor") {
      lastSoilDHT  = d;
      foundSoilDHT = true;
    }
    if (!foundLight && d.sensorID == "lightsensor") {
      lastLight  = d;
      foundLight = true;
    }
    if (foundSoilDHT && foundLight) break;
  }

  float soilVal  = (foundSoilDHT ? lastSoilDHT.soilMoisture : 0.0f);
  float tempVal  = (foundSoilDHT ? lastSoilDHT.temperature  : 0.0f);
  float humVal   = (foundSoilDHT ? lastSoilDHT.humidity     : 0.0f);
  float lightVal = (foundLight   ? lastLight.lightLevel     : 0.0f);

  // IP domácí Wi-Fi, pokud je připojeno. Pokud ne, "AP only"
  String wifiIP = (WiFi.status() == WL_CONNECTED) 
                    ? WiFi.localIP().toString() 
                    : "AP-only";

  // Vypíšeme na OLED
  displayStatus(
    homeSsid,     // WiFi jméno
    wifiIP,       // IP
    autoWatering, // Režim (Auto/Manual)
    soilVal,
    lightVal,
    tempVal,
    humVal
  );
}

void loop() {
  updateDisplayLoop();
  checkAndIrrigate();
  updateDisplayWithSensorData();

  delay(50);
}
