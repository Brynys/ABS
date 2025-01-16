#ifndef FARM_HUB_DISPLAY_H
#define FARM_HUB_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 128x64 OLED displej
static Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Inicializace displeje
static inline void initDisplay() {
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    return;
  }
  display.clearDisplay();
  display.display();
}

// Rychlá funkce pro zobrazení dvou řádků (ponechána)
static inline void displayInfo(const String &line1, const String &line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 16);
  display.println(line2);
  display.display();
}

/**
 * @brief Zobrazí SSID, IP, režim (Auto/Manual) a poslední data senzorů.
 *        Půdní vlhkost a světlo na jednom řádku, teplota a vlhkost vzduchu na dalším.
 */
static inline void displayStatus(const String &wifiName,
                                 const String &wifiIP,
                                 bool autoMode,
                                 float soilVal,
                                 float lightVal,
                                 float tempVal,
                                 float humVal)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // 1. řádek: WiFi SSID
  display.setCursor(0, 0);
  display.print("WiFi: ");
  display.println(wifiName);

  // 2. řádek: IP adresa
  display.setCursor(0, 10);
  display.print("IP:   ");
  display.println(wifiIP);

  // 3. řádek: Režim zalévání
  display.setCursor(0, 20);
  display.print("Mode: ");
  display.println(autoMode ? "Auto" : "Manual");

  // 4. řádek: Půdní vlhkost a světlo (např. "Soil=35%  Light=52lx")
  display.setCursor(0, 30);
  display.print("Soil=");
  display.print(soilVal, 1);
  display.print("%  Light=");
  display.print(lightVal, 1);
  display.println("lx");

  // 5. řádek: Teplota a vlhkost vzduchu (např. "Temp=21.5C Hum=40%")
  display.setCursor(0, 40);
  display.print("Temp=");
  display.print(tempVal, 1);
  display.print("C  Hum=");
  display.print(humVal, 1);
  display.println("%");

  display.display();
}

// Pokud potřebujete něco obsluhovat v loopu (animace apod.)
static inline void updateDisplayLoop() {
  // Prázdné nebo vlastní logika
}

#endif // FARM_HUB_DISPLAY_H
