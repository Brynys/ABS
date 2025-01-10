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
            "h1{margin-top:0; margin-bottom:0.5em;}"
            "h3{margin-top:1.2em; margin-bottom:0.5em;}"
            "p{margin:0.8em 0; line-height:1.4;}"
            "hr{margin:1.5em 0; border:none; border-top:1px solid #ddd;}"
            ".btn{background:#4CAF50;color:#fff;padding:6px 12px;border:none;cursor:pointer;border-radius:4px;}"
            ".btn:hover{background:#45a049;}"
            ".table-container{width:100%;overflow-x:auto;margin-top:10px;}"
            "table{border-collapse:collapse;min-width:600px;width:100%;}"
            "th,td{border:1px solid #ccc;padding:6px;text-align:left;}"
            "th{background:#f9f9f9;}"
            ".form-group{margin-bottom:1em;}"
            "label{font-weight:bold;display:block;margin-bottom:0.3em;}"
            "input[type='text'],input[type='password'],input[type='number'],select{"
            "  width:100%;max-width:300px;padding:6px;margin-bottom:0.5em;box-sizing:border-box;"
            "  border:1px solid #ccc;border-radius:4px;"
            "}"
            /* Přidáme menší styl pro "plátno" grafů, aby nebyly zbytečně roztažené */
            ".chart-container{"
            "  width:100%;"
            "  max-width:600px;"   /* Můžete upravit dle potřeby */
            "  height:300px;"      /* Můžete upravit dle potřeby */
            "  margin-bottom:30px;"
            "}"
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
   *    - pokud je Wi-Fi připojeno, načte se chart.js a vykreslí se grafy
   */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html;
    html += makeHtmlHeader("FarmHub - Přehled");
    
    // Zobrazení stavu AP
    html += "<p>AP SSID: <strong>" + String(AP_SSID) + "</strong>, "
            "heslo: <strong>" + String(AP_PASS) + "</strong></p>";
    html += "<p>AP IP: <strong>" + WiFi.softAPIP().toString() + "</strong></p>";

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

    // Pokud jsme online, zobrazíme 4 grafy
    if(WiFi.status() == WL_CONNECTED) {
      html += "<hr><h3>Grafy (Chart.js)</h3>";

      // 1) Přidáme 4 plochy pro 4 grafy
      html += "<div class='chart-container'><canvas id='chartSoil'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartTemp'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartHum'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartLight'></canvas></div>";

      // 2) Vložíme script pro Chart.js (CDN)
      html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";

      // 3) Připravíme data pro X-osu = timestamps
      html += "<script>";
      html += "const labels = [";
      for(int i = (count > 0 ? 0 : -1); i < count; i++) {
        if(i >= 0) {
          html += "'" + String(dataBuffer[i].timestamp) + "'";
          if(i < count - 1) html += ",";
        }
      }
      html += "];";

      // Data pro soilMoisture
      html += "const soilData = [";
      for(int i = 0; i < count; i++) {
        html += String(dataBuffer[i].soilMoisture);
        if(i < count - 1) html += ",";
      }
      html += "];";

      // Data pro temperature
      html += "const tempData = [";
      for(int i = 0; i < count; i++) {
        html += String(dataBuffer[i].temperature);
        if(i < count - 1) html += ",";
      }
      html += "];";

      // Data pro humidity
      html += "const humData = [";
      for(int i = 0; i < count; i++) {
        html += String(dataBuffer[i].humidity);
        if(i < count - 1) html += ",";
      }
      html += "];";

      // Data pro light
      html += "const lightData = [";
      for(int i = 0; i < count; i++) {
        html += String(dataBuffer[i].lightLevel);
        if(i < count - 1) html += ",";
      }
      html += "];";

      // 4) Vytvoříme 4 samostatné grafy (line)
      // -- Soil Moisture --
      html += "const ctxSoil = document.getElementById('chartSoil').getContext('2d');"
              "new Chart(ctxSoil, {"
              "  type: 'line',"
              "  data: {"
              "    labels: labels,"
              "    datasets: [{"
              "      label: 'Soil Moisture (%)',"
              "      data: soilData,"
              "      borderColor: 'rgba(75, 192, 192, 1)',"
              "      backgroundColor: 'rgba(75, 192, 192, 0.2)',"
              "      tension: 0.1"
              "    }]"
              "  },"
              "  options: {"
              "    responsive: true,"
              "    maintainAspectRatio: false"
              "  }"
              "});";

      // -- Temperature --
      html += "const ctxTemp = document.getElementById('chartTemp').getContext('2d');"
              "new Chart(ctxTemp, {"
              "  type: 'line',"
              "  data: {"
              "    labels: labels,"
              "    datasets: [{"
              "      label: 'Temperature (°C)',"
              "      data: tempData,"
              "      borderColor: 'rgba(255, 99, 132, 1)',"
              "      backgroundColor: 'rgba(255, 99, 132, 0.2)',"
              "      tension: 0.1"
              "    }]"
              "  },"
              "  options: {"
              "    responsive: true,"
              "    maintainAspectRatio: false"
              "  }"
              "});";

      // -- Humidity --
      html += "const ctxHum = document.getElementById('chartHum').getContext('2d');"
              "new Chart(ctxHum, {"
              "  type: 'line',"
              "  data: {"
              "    labels: labels,"
              "    datasets: [{"
              "      label: 'Humidity (%)',"
              "      data: humData,"
              "      borderColor: 'rgba(54, 162, 235, 1)',"
              "      backgroundColor: 'rgba(54, 162, 235, 0.2)',"
              "      tension: 0.1"
              "    }]"
              "  },"
              "  options: {"
              "    responsive: true,"
              "    maintainAspectRatio: false"
              "  }"
              "});";

      // -- Light --
      html += "const ctxLight = document.getElementById('chartLight').getContext('2d');"
              "new Chart(ctxLight, {"
              "  type: 'line',"
              "  data: {"
              "    labels: labels,"
              "    datasets: [{"
              "      label: 'Light',"
              "      data: lightData,"
              "      borderColor: 'rgba(255, 206, 86, 1)',"
              "      backgroundColor: 'rgba(255, 206, 86, 0.2)',"
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
        html += "<div class='form-group'>";
        html += "<label>Vyberte síť:</label>";
        html += "<select name='ssid'>";
        for (int i = 0; i < g_foundNetworks; i++) {
          String ssidFound = g_scannedSSIDs[i];
          html += "<option value='" + ssidFound + "'>";
          html += ssidFound; 
          html += "</option>";
        }
        html += "</select>";
        html += "</div>";

        // Heslo
        html += "<div class='form-group'>";
        html += "<label>Heslo:</label>";
        html += "<input type='password' name='pass'>";
        html += "</div>";

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
    html += "<div class='form-group'>";
    html += "<label>SSID:</label>";
    html += "<input type='text' name='ssid'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Heslo:</label>";
    html += "<input type='password' name='pass'>";
    html += "</div>";

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

    html += "<div class='form-group'>";
    html += "<label>Litry za den:</label>";
    html += "<input type='number' step='0.1' name='litersPerDay' value='" + String(litersPerDay) + "'>";
    html += "</div>";

    html += "<div class='form-group'>";
    html += "<label>Interval (hodiny):</label>";
    html += "<input type='number' step='1' name='wateringIntervalHours' value='" + String(wateringIntervalHours) + "'>";
    html += "</div>";

    html += "<input type='submit' class='btn' value='Uložit Zalévání'>";
    html += "</form>";

    // Formulář pro jednorázové zalití
    html += "<hr><h3>Jednorázové zalití</h3>";
    html += "<form method='POST' action='/runoneshot'>";

    html += "<div class='form-group'>";
    html += "<label>Litry:</label>";
    html += "<input type='number' step='0.1' name='oneTimeLiters' value='1.0'>";
    html += "</div>";

    html += "<input type='submit' class='btn' value='Zalít'>";
    html += "</form>";

    html += makeHtmlFooter();
    request->send(200, "text/html", html);
  });

  // ---- PŮVODNÍ POST ENDPOINTY ----

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
