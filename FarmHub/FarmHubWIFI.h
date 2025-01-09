#ifndef FARM_HUB_WIFI_H
#define FARM_HUB_WIFI_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <vector>

// Vše "static", abychom se vyhnuli multiple definition
static const char* AP_SSID = "FarmHub-AP";
static const char* AP_PASS = "farmhub123";

/** 
 * Globální proměnné pro asynchronní skenování 
 */
static bool g_isScanning      = false;  // Běží teď sken?
static bool g_scanComplete    = false;  // Je hotovo?
static int  g_foundNetworks   = 0;      // Kolik sítí jsme našli?
static std::vector<String> g_scannedSSIDs;

/**
 * Vytvoří AP + STA mód, aby se k němu mohly
 * připojit senzory (AP), a zároveň se to mohlo
 * připojit k domácí Wi-Fi (STA).
 */
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

/**
 * Pokus o připojení k domácí Wi-Fi (SSID + heslo).
 * Vrací true při úspěchu, false při neúspěchu.
 */
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

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to home WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Failed to connect to home WiFi!");
    return false;
  }
}

/**
 * Získání lokální IP (buď STA IP, nebo AP IP).
 */
static inline IPAddress getLocalIP() {
  return WiFi.localIP();
}

/**
 * Spustí asynchronní sken sítí.
 * - Pokud už sken běží, neudělá nic.
 */
static inline void startAsyncScan() {
  if (!g_isScanning) {
    Serial.println("Spouštím asynchronní skenování Wi-Fi...");
    g_isScanning   = true;
    g_scanComplete = false;
    g_foundNetworks = -1;

    // Zahájí asynchronní sken na pozadí
    WiFi.scanNetworks(true);
  }
}

/**
 * Kontroluje stav asynchronního skenu, 
 * pokud je hotovo, uloží výsledky do g_scannedSSIDs,
 * a znovu zapne AP (WiFi.mode(WIFI_AP_STA)).
 *
 * Tuto funkci je třeba volat pravidelně (např. v loop())
 * nebo alespoň pokaždé, než zobrazíte stránku /wifi.
 */
static inline void checkAsyncScan() {
  if (!g_isScanning) {
    return; // nic neděláme
  }

  int result = WiFi.scanComplete();
  if (result == WIFI_SCAN_RUNNING) {
    // stále probíhá
    return;
  }
  else if (result == WIFI_SCAN_FAILED) {
    // někdy se stává, že scan selže - zkusíme znovu
    Serial.println("Sken selhal, opakuji...");
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
  }
  else {
    // Hotovo, result >= 0 = počet sítí
    g_isScanning    = false;
    g_scanComplete  = true;
    g_foundNetworks = result;

    g_scannedSSIDs.clear();
    for (int i = 0; i < result; i++) {
      g_scannedSSIDs.push_back(WiFi.SSID(i));
    }
    WiFi.scanDelete();

    // Obnovíme AP, kdyby se stihlo vypnout
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);

    Serial.printf("Asynchronní sken dokončen, nalezeno %d sítí.\n", result);
  }
}

#endif // FARM_HUB_WIFI_H
