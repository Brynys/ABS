#ifndef FARM_HUB_DISPLAY_H
#define FARM_HUB_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED displej (např. 128x64)
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

// Zobrazení dvou řádků textu
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

// Pravidelná aktualizace displeje (volitelná)
static inline void updateDisplayLoop() {
  // Lze doplnit vlastní logiku
}

#endif // FARM_HUB_DISPLAY_H
