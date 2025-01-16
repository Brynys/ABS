#ifndef FARM_HUB_WIFI_H
#define FARM_HUB_WIFI_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <vector>

// Výchozí AP (Access Point) údaje
static const char* AP_SSID = "FarmHub-AP";
static const char* AP_PASS = "farmhub123";

// Proměnné pro asynchronní skenování
static bool g_isScanning    = false;
static bool g_scanComplete  = false;
static int  g_foundNetworks = 0;
static std::vector<String> g_scannedSSIDs;

// Spuštění AP + STA módu
static inline void setupWifiAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println("=== AP MODE ===");
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP PASS: ");
  Serial.println(AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

// Připojení k domácí Wi-Fi
static inline bool connectToHomeWiFi(const String &ssid, const String &pass) {
  if (ssid.isEmpty() || pass.isEmpty()) {
    Serial.println("Home WiFi credentials empty, skipping...");
    return false;
  }
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to home WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Failed to connect to home WiFi!");
    return false;
  }
}

// Získání lokální IP
static inline IPAddress getLocalIP() {
  return WiFi.localIP();
}

// Spuštění asynchronního skenu
static inline void startAsyncScan() {
  if (!g_isScanning) {
    Serial.println("Starting async Wi-Fi scan...");
    g_isScanning   = true;
    g_scanComplete = false;
    g_foundNetworks = -1;
    WiFi.scanNetworks(true);
  }
}

// Kontrola, zda sken doběhl a uložení výsledků
static inline void checkAsyncScan() {
  if (!g_isScanning) return;

  int result = WiFi.scanComplete();
  if (result == WIFI_SCAN_RUNNING) {
    return; // stále probíhá
  }
  else if (result == WIFI_SCAN_FAILED) {
    Serial.println("Scan failed, retrying...");
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
  }
  else {
    g_isScanning    = false;
    g_scanComplete  = true;
    g_foundNetworks = result;

    g_scannedSSIDs.clear();
    for (int i = 0; i < result; i++) {
      g_scannedSSIDs.push_back(WiFi.SSID(i));
    }
    WiFi.scanDelete();

    // Obnovíme AP pro jistotu
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);

    Serial.printf("Scan complete, found %d networks.\n", result);
  }
}

#endif // FARM_HUB_WIFI_H
