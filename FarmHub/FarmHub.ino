#include <Arduino.h>

// Načteme všechny .h moduly
#include "FarmHubWiFi.h"
#include "FarmHubDisplay.h"
#include "FarmHubConfig.h"
#include "FarmHubData.h"
#include "FarmHubWebServer.h"
#include "FarmHubWebSocket.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  // SPIFFS init
  initFileSystem(); // z FarmHubConfig.h

  // Načteme uloženou konfiguraci
  loadUserConfig(); // homeSsid, homePass, moistureThreshold...

  // OLED displej
  initDisplay();
  displayInfo("Starting...", "");

  // Vytvoříme AP (Access Point) pro konfiguraci v případě potíží
  setupWifiAP();

  // Zkusit se připojit k domácí Wi-Fi
  bool wifiOK = connectToHomeWiFi(homeSsid, homePass);
  if (wifiOK) {
    displayInfo("WiFi OK", WiFi.localIP().toString());

    // >>> ZDE zavoláme synchronizaci času přes NTP <<<
    setupTimeFromNTP();
    
  } else {
    displayInfo("WiFi fail", "AP only");
  }

  // Spustíme asynchronní webserver
  startAsyncWebServer();
  startWebSocket();
}

void loop() {
  // Aktualizace displeje (pokud máte měnící se údaje, např. čas, stav…)
  updateDisplayLoop();

  // Kontrola zalévání (automatika/manual)
  checkAndIrrigate();

  delay(50);

}
