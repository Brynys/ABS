#ifndef FARM_HUB_WEBSERVER_H
#define FARM_HUB_WEBSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Načteme WiFi (pro AP_SSID, AP_PASS, a status WiFi)
#include "FarmHubWiFi.h"

// Načteme Config (pro homeSsid, homePass, saveUserConfig() atd.)
#include "FarmHubConfig.h"

// Načteme Data (pro dataBuffer)
#include "FarmHubData.h"

// Vytvoříme statický server na portu 80
static AsyncWebServer server(80);

/**
 * Pomocná funkce, která vytvoří HTML hlavičku se základním 
 * stylem a navigačním menu.
 */
String makeHtmlHeader(const String &title) {
    String html;
    html += "<!DOCTYPE html><html><head><meta charset='utf-8'/>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1.0'/>";
    html += "<title>" + title + "</title>";

    // Základní styl pro celou stránku
    html += "<style>"
            "body{font-family:sans-serif;margin:0;padding:0;background:#f0f0f0;}"
            ".navbar{position:fixed;top:0;left:0;width:100%;background:#4CAF50;padding:10px 20px;box-sizing:border-box;z-index:999;}"
            ".navbar a{color:#fff;text-decoration:none;margin-right:20px;font-weight:bold;padding:6px 12px;border-radius:4px;transition:background 0.3s;}"
            ".navbar a:hover{background:#45a049;}"
            ".container{max-width:1000px;margin:80px auto 20px auto;padding:20px;background:#fff;border-radius:8px;box-shadow:0 0 6px rgba(0,0,0,0.1);}"
            "h1{margin-top:0;}"
            ".btn{background:#4CAF50;color:#fff;padding:6px 12px;border:none;cursor:pointer;}"
            ".btn:hover{background:#45a049;}"
            ".table-container{width:100%;overflow-x:auto;margin-top:10px;}"
            "table{border-collapse:collapse;min-width:600px;width:100%;}"
            "th,td{border:1px solid #ccc;padding:6px;text-align:left;}"
            "th{background:#f9f9f9;}"
            "</style>";

    html += "</head><body>";

    // Vložíme horní lištu (navbar)
    html += "<nav class='navbar'>";
    html += "<a href='/'>Přehled</a>";
    html += "<a href='/wifi'>Nastavení Wi-Fi</a>";
    html += "<a href='/watering'>Zalévání</a>";
    html += "</nav>";

    // Vložíme kontejner na obsah
    html += "<div class='container'>";
    html += "<h1>" + title + "</h1>";

    return html;
}


/**
 * Pomocná funkce, která uzavře HTML.
 */
String makeHtmlFooter() {
    return "</div></body></html>";
}

/**
 * Spuštění asynchronního webového serveru
 */
static inline void startAsyncWebServer() {
  
  /**
   * 1) Hlavní stránka "/"
   *    - ukáže základní info
   *    - zobrazí poslední data v tabulce
   *    - pokud je Wi-Fi připojeno, načte se chart.js a vykreslí se graf
   */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html;
    html += makeHtmlHeader("FarmHub - Přehled");
    
    // Zobrazení stavu AP
    html += "<p>AP SSID: <strong>" + String(AP_SSID) + "</strong>, "
            "heslo: <strong>" + String(AP_PASS) + "</strong></p>";
    html += "<p>AP IP: " + WiFi.softAPIP().toString() + "</p>";

    // Zobrazení stavu STA (domácí Wi-Fi)
    if(WiFi.status() == WL_CONNECTED) {
      html += "<p><strong>Připojeno k domácí síti:</strong> " 
              + homeSsid + " (IP: " + WiFi.localIP().toString() + ")</p>";
    } else {
      html += "<p><strong>Nepřipojeno k domácí Wi-Fi.</strong></p>";
    }

    // Výpis posledních dat - obalený do .table-container
    html += "<hr><h3>Poslední data</h3>";
    html += "<div class='table-container'>";
    html += "<table>";
    html += "<tr><th>SensorID</th><th>Soil(%)</th><th>Temp(°C)</th>"
            "<th>Hum(%)</th><th>Light</th><th>Time(ms)</th></tr>";
    int count = dataBuffer.size();
    // Zobrazíme posledních 10 záznamů
    for(int i = count-1; i >= 0 && i > count-11; i--) {
      html += "<tr>";
      html += "<td>" + dataBuffer[i].sensorID              + "</td>";
      html += "<td>" + String(dataBuffer[i].soilMoisture)  + "</td>";
      html += "<td>" + String(dataBuffer[i].temperature)   + "</td>";
      html += "<td>" + String(dataBuffer[i].humidity)      + "</td>";
      html += "<td>" + String(dataBuffer[i].lightLevel)    + "</td>";
      html += "<td>" + String(dataBuffer[i].timestamp)     + "</td>";
      html += "</tr>";
    }
    html += "</table>";
    html += "</div>"; // konec .table-container

    // Pokud jsme online, zobrazíme graf
    if(WiFi.status() == WL_CONNECTED) {
      html += "<hr><h3>Graf (Chart.js)</h3>";
      html += "<canvas id='myChart' width='100%' height='50'></canvas>";
      
      // 1) Vložíme script pro chart.js (CDN). Použijeme např. cdn.jsdelivr.net
      html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
      
      // 2) Vytvoříme pole s daty (timestamp a soilMoisture jen jako příklad)
      html += "<script>";
      html += "const labels = [";
      for(int i = (count > 0 ? 0 : -1); i < count; i++) {
        // předpokládáme, že dataBuffer jsou chronologicky, raději si to ideálně ověřte
        // Sem dáme reálný timestamp nebo index
        if(i >= 0) {
          html += "'" + String(dataBuffer[i].timestamp) + "'";
          if(i < count - 1) html += ",";
        }
      }
      html += "];";
      
      // Data pro soilMoisture:
      html += "const soilData = [";
      for(int i = 0; i < count; i++) {
        html += String(dataBuffer[i].soilMoisture);
        if(i < count - 1) html += ",";
      }
      html += "];";
      
      // 3) Inicializujeme graf (line)
      html += "const ctx = document.getElementById('myChart').getContext('2d');"
              "new Chart(ctx, {"
              "  type: 'line',"
              "  data: {"
              "    labels: labels,"
              "    datasets: [{"
              "      label: 'Soil Moisture',"
              "      data: soilData,"
              "      borderColor: 'rgba(75, 192, 192, 1)',"
              "      tension: 0.1"
              "    }]"
              "  },"
              "  options: {"
              "    responsive: true,"
              "    maintainAspectRatio: false"
              "  }"
              "});";
      html += "</script>";
    }
    
    html += makeHtmlFooter();
    
    request->send(200, "text/html", html);
  });
  
  /**
   * 2) Stránka "/wifi" - nastavení domácí Wi-Fi
   */
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    // Nejdříve zkontrolujeme, jestli náhodou už sken neskončil:
    checkAsyncScan(); 

    String html;
    html += makeHtmlHeader("Nastavení Wi-Fi");

    // Tlačítko pro zahájení skenu (pokud uživatel chce spustit znova)
    html += "<p><a class='btn' href='/startscan'>Načíst seznam sítí (async)</a></p>";

    // Rozhodnutí podle stavu skenu
    if (g_isScanning) {
      // --- PROBÍHÁ SKEN ---
      html += "<p><strong>Probíhá skenování sítí...</strong></p>";
      // Text + odpočet
      html += "<p>Stránka se obnoví za <span id='timer'>10</span> s.</p>";
      
      // Vložíme JavaScript pro odpočet
      html += "<script>"
              "var countdown = 10;"
              "var x = setInterval(function(){"
              "  countdown--;"
              "  if(countdown <= 0){"
              "    clearInterval(x);"
              "    location.reload();"
              "  }"
              "  document.getElementById('timer').textContent = countdown;"
              "}, 1000);"
              "</script>";

    } else if (g_scanComplete) {
      // --- SKEN JE HOTOV ---
      if (g_foundNetworks <= 0) {
        // Nic nenašel nebo selhalo
        html += "<p>Nebyly nalezeny žádné sítě, nebo sken selhal.</p>";
      } else {
        // Máme nalezené sítě
        html += "<p>Nalezené sítě v okolí (" + String(g_foundNetworks) + "):</p>";

        // Formulář s <select>
        html += "<form method='POST' action='/setwifi'>";
        html += "<label>Vyberte síť:</label><br>";
        html += "<select name='ssid'>";
        for (int i = 0; i < g_foundNetworks; i++) {
          String ssidFound = g_scannedSSIDs[i];
          html += "<option value='" + ssidFound + "'>";
          html += ssidFound; 
          html += "</option>";
        }
        html += "</select><br><br>";

        // Heslo
        html += "<label>Heslo:</label><br>";
        html += "<input type='password' name='pass'><br><br>";
        // Odeslat
        html += "<input type='submit' class='btn' value='Uložit Wi-Fi'>";
        html += "</form>";
      }
    } else {
      // --- NIC SE NEDĚJE (neprobíhá sken a není hotovo – zřejmě ještě nebyl spuštěn)
      html += "<p>Nebylo spuštěno skenování, klikněte výše na tlačítko \"Načíst seznam sítí\".</p>";
    }

    // Zachováme formulář pro ruční zadání
    html += "<hr><p>Nebo zadat ručně (skrytou síť):</p>";
    html += "<form method='POST' action='/setwifi'>";
    html += "<label>SSID:</label><br><input type='text' name='ssid'><br><br>";
    html += "<label>Heslo:</label><br><input type='password' name='pass'><br><br>";
    html += "<input type='submit' class='btn' value='Uložit Wi-Fi'>";
    html += "</form>";

    html += makeHtmlFooter();
    request->send(200, "text/html", html);
  });


  server.on("/startscan", HTTP_GET, [](AsyncWebServerRequest *request){
    startAsyncScan();       // zavolá funkci z FarmHubWiFi.h
    request->redirect("/wifi");
  });


  /**
   * 3) Stránka "/watering" - nastavení zalévání
   */
  server.on("/watering", HTTP_GET, [](AsyncWebServerRequest *request){
    String html;
    html += makeHtmlHeader("Nastavení zalévání");

    // Formulář nastavení periody a objemu zalévání 
    html += "<h3>Zalévání</h3>";
    html += "<form method='POST' action='/setwatering'>";
    html += "<label>Litry za den:</label>";
    html += "<input type='number' step='0.1' name='litersPerDay' value='" + String(litersPerDay) + "'><br>";
    html += "<label>Interval (hodiny):</label>";
    html += "<input type='number' step='1' name='wateringIntervalHours' value='" + String(wateringIntervalHours) + "'><br>";
    html += "<input type='submit' class='btn' value='Uložit Zalévání'>";
    html += "</form>";

    // Formulář pro jednorázové zalití
    html += "<hr><h3>Jednorázové zalití</h3>";
    html += "<form method='POST' action='/runoneshot'>";
    html += "<label>Litry:</label>";
    html += "<input type='number' step='0.1' name='oneTimeLiters' value='1.0'><br>";
    html += "<input type='submit' class='btn' value='Zalít'>";
    html += "</form>";

    html += makeHtmlFooter();
    request->send(200, "text/html", html);
  });

  // ---- PONECHÁME PŮVODNÍ POST ENDPOINTY ----

  // Obsluha pro /setwifi (POST)
  server.on("/setwifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      homeSsid = request->getParam("ssid", true)->value();
      homePass = request->getParam("pass", true)->value();

      // Uložíme do configu
      saveUserConfig();

      request->redirect("/wifi");

      // Odpojíme a zkusíme se připojit
      WiFi.disconnect();
      bool ok = connectToHomeWiFi(homeSsid, homePass);
      Serial.printf("connectToHomeWiFi => %s\n", ok ? "OK" : "FAIL");
    }
    request->redirect("/wifi");  // po uložení vrátíme na stránku Wi-Fi
  });

  // Obsluha pro /setwatering (POST) - ukládá litry/den a interval (hodiny)
  server.on("/setwatering", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("litersPerDay", true) && request->hasParam("wateringIntervalHours", true)) {
      litersPerDay = request->getParam("litersPerDay", true)->value().toFloat();
      wateringIntervalHours = request->getParam("wateringIntervalHours", true)->value().toFloat();

      // Uložíme do configu
      saveUserConfig();
      Serial.printf("Uloženo: litersPerDay=%.1f, interval=%.1f h\n", litersPerDay, wateringIntervalHours);
    }
    request->redirect("/watering");  // po uložení vrátíme na stránku zalévání
  });

  // Jednorázové zalití dle litrů
  server.on("/runoneshot", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("oneTimeLiters", true)) {
      float oneTimeLiters = request->getParam("oneTimeLiters", true)->value().toFloat();
      
      float secondsFloat = oneTimeLiters * 30.0f;  // 1 litr ~ 30s (dle vašeho výpočtu)
      int secondsToPump = (int)round(secondsFloat);

      Serial.printf("Jednorázové zalití: %.2f litrů -> %d sekund.\n", oneTimeLiters, secondsToPump);

      // Spustíme čerpadlo
      broadcastRunPump(secondsToPump);
    }
    request->redirect("/watering");
  });

  // Spustíme server
  server.begin();
  Serial.println("AsyncWebServer started on port 80");
}

#endif // FARM_HUB_WEBSERVER_H
