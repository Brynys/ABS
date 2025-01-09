#include <Arduino.h>

// Načteme všechny .h moduly
#include "FarmHubWiFi.h"
#include "FarmHubDisplay.h"
#include "FarmHubConfig.h"
#include "FarmHubData.h"
#include "FarmHubWebServer.h"
#include "FarmHubWebSocket.h"
// případně #include "FarmHubWebSocket.h", pokud máš i websockets

void setup() {
  Serial.begin(115200);
  delay(200);

  // SPIFFS init
  initFileSystem(); // z FarmHubConfig.h

  // Načteme uloženou konfiguraci
  loadUserConfig(); // homeSsid, homePass, moistureThreshold...

  // OLED
  initDisplay();
  displayInfo("Starting...", "");

  // Vytvoří AP
  setupWifiAP();

  // Zkusit se připojit k domácí Wi-Fi
  bool wifiOK = connectToHomeWiFi(homeSsid, homePass);
  if(wifiOK) {
    displayInfo("WiFi OK", WiFi.localIP().toString());
  } else {
    displayInfo("WiFi fail", "AP only");
  }

  // Spustíme asynchronní webserver
  startAsyncWebServer();
  startWebSocket();

}

void loop() {
  // Případně update displeje
  updateDisplayLoop();

  // Kontrola zalévání
  checkAndIrrigate();

  delay(50);
}
