#include <Arduino.h>

#include "FarmHubWiFi.h"
#include "FarmHubDisplay.h"
#include "FarmHubConfig.h"
#include "FarmHubData.h"
#include "FarmHubWebServer.h"
#include "FarmHubWebSocket.h"

/**
 * setup() nastavuje a inicializuje
 * všechny potřebné části projektu (SPIFFS, Wi-Fi, displej, webserver, atd.).
 */
void setup() {
  Serial.begin(115200);
  delay(200);

  // 1) Inicializace souborového systému (SPIFFS)
  initFileSystem(); // z FarmHubConfig.h

  // 2) Načteme uloženou konfiguraci (homeSsid, homePass, moistureThreshold, ...)
  loadUserConfig();

  // 3) Inicializujeme OLED displej
  initDisplay();
  displayInfo("Starting...", "");

  // 4) Spustíme AP (Access Point) pro případné nastavení, když selže připojení
  setupWifiAP();

  // 5) Zkusíme se připojit k domácí Wi-Fi
  bool wifiOK = connectToHomeWiFi(homeSsid, homePass);
  if (wifiOK) {
    displayInfo("WiFi OK", WiFi.localIP().toString());
    // Synchronizace času přes NTP (pokud je Wi-Fi OK)
    setupTimeFromNTP();
  } else {
    displayInfo("WiFi fail", "AP only");
  }

  // 6) Spuštění asynchronního webového serveru a WebSocketu
  startAsyncWebServer();
  startWebSocket();
}

void loop() {
  // 1) Aktualizace OLED displeje - pokud zobrazujete proměnlivá data
  updateDisplayLoop();

  // 2) Kontrola prahu vlhkosti a automatické/manualní zalévání
  checkAndIrrigate();

  // Krátká prodleva, aby smyčka neběžela příliš rychle
  delay(50);
}
