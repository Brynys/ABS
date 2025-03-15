#ifndef FARM_HUB_WEBSERVER_H
#define FARM_HUB_WEBSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <time.h>

// Načteme WiFi (pro AP_SSID, AP_PASS, a status WiFi)
#include "FarmHubWiFi.h"

// Načteme Config (pro homeSsid, homePass, saveUserConfig() atd.)
#include "FarmHubConfig.h"

// Načteme Data (pro dataBuffer, SensorReading atd.)
#include "FarmHubData.h"

// Globální proměnné (pokud je máte jinde, odstraňte zde):
extern bool autoWatering;
extern float moistureThreshold;
extern int waterAmountML;
extern std::vector<SensorReading> dataBuffer;

// Vytvoříme statický server na portu 80
static AsyncWebServer server(80);

// ------------------------------------------------------------
// 1) Uložíme velké řetězce (HTML/CSS/JS) do PROGMEM
// ------------------------------------------------------------

static const char PAGE_HEAD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>%TITLE%</title>
  <style>
    body{font-family:sans-serif;margin:0;padding:0;background:#f0f0f0;}
    .navbar{position:fixed;top:0;left:0;width:100%;background:#4CAF50;padding:10px 20px;box-sizing:border-box;z-index:999;}
    .navbar a{color:#fff;text-decoration:none;margin-right:20px;font-weight:bold;padding:6px 12px;border-radius:4px;transition:background 0.3s;}
    .navbar a:hover{background:#45a049;}
    .container{max-width:1000px;margin:80px auto 20px auto;padding:20px;background:#fff;border-radius:8px;box-shadow:0 0 6px rgba(0,0,0,0.1);}
    h1{margin-top:0; margin-bottom:0.5em;}
    h3{margin-top:1.2em; margin-bottom:0.5em;}
    p{margin:0.8em 0; line-height:1.4;}
    hr{margin:1.5em 0; border:none; border-top:1px solid #ddd;}
    .btn{background:#4CAF50;color:#fff;padding:6px 12px;border:none;cursor:pointer;border-radius:4px;}
    .btn:hover{background:#45a049;}
    .table-container{width:100%;overflow-x:auto;margin-top:10px;}
    table{border-collapse:collapse;min-width:600px;width:100%;}
    th,td{border:1px solid #ccc;padding:6px;text-align:left;}
    th{background:#f9f9f9;}
    .form-group{margin-bottom:1em;}
    label{font-weight:bold;display:block;margin-bottom:0.3em;}
    input[type='text'],input[type='password'],input[type='number'],select{
      width:100%;max-width:300px;padding:6px;margin-bottom:0.5em;box-sizing:border-box;border:1px solid #ccc;border-radius:4px;
    }
    .chart-container{
      width:100%;
      max-width:600px;
      height:300px;
      margin-bottom:30px;
    }
  </style>
</head>
<body>
  <nav class="navbar">
    <a href="/">Přehled</a>
    <a href="/charts">Grafy</a>
    <a href="/history">Log</a>
    <a href="/wifi">Nastavení Wi-Fi</a>
    <a href="/watering">Zalévání</a>
  </nav>
  <div class="container">
  <h1>
)rawliteral";

/**
 * Po TITLE následuje uzavření <h1> a pak zbytek HTML těla.
 */
static const char PAGE_HEAD_END[] PROGMEM = R"rawliteral(
</h1>
)rawliteral";

/**
 * Základní HTML patička
 */
static const char PAGE_FOOTER[] PROGMEM = R"rawliteral(
  </div>
</body>
</html>
)rawliteral";

/**
 * Volitelný JavaScript pro Chart.js. (Můžete dát i do externího .js)
 */
static const char CHART_JS[] PROGMEM = R"rawliteral(
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script>
function drawSoilChart(labels, dataSoil) {
  const ctxSoil = document.getElementById('chartSoil').getContext('2d');
  new Chart(ctxSoil, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Soil Moisture (%)',
        data: dataSoil,
        borderColor: 'rgba(75, 192, 192, 1)',
        backgroundColor: 'rgba(75, 192, 192, 0.2)',
        tension: 0.1
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { title: { display: true, text: 'Unix time (s)'} },
        y: { suggestedMin: 0, suggestedMax: 100 }
      }
    }
  });
}
function drawTempChart(labels, dataTemp) {
  const ctxTemp = document.getElementById('chartTemp').getContext('2d');
  new Chart(ctxTemp, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Temperature (°C)',
        data: dataTemp,
        borderColor: 'rgba(255, 99, 132, 1)',
        backgroundColor: 'rgba(255, 99, 132, 0.2)',
        tension: 0.1
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { title: { display: true, text: 'Unix time (s)'} }
      }
    }
  });
}
function drawHumChart(labels, dataHum) {
  const ctxHum = document.getElementById('chartHum').getContext('2d');
  new Chart(ctxHum, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Humidity (%)',
        data: dataHum,
        borderColor: 'rgba(54, 162, 235, 1)',
        backgroundColor: 'rgba(54, 162, 235, 0.2)',
        tension: 0.1
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { title: { display: true, text: 'Unix time (s)'} },
        y: { suggestedMin: 0, suggestedMax: 100 }
      }
    }
  });
}
function drawLightChart(labels, dataLight) {
  const ctxLight = document.getElementById('chartLight').getContext('2d');
  new Chart(ctxLight, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Light',
        data: dataLight,
        borderColor: 'rgba(255, 206, 86, 1)',
        backgroundColor: 'rgba(255, 206, 86, 0.2)',
        tension: 0.1
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { title: { display: true, text: 'Unix time (s)'} }
      }
    }
  });
}
</script>
)rawliteral";

// Vytvoření HTML hlavičky
String makeHtmlHeader(const String &title) {
    String html;
    html.reserve(2048);

    // 1) Načteme z PROGMEM úvod HTML s <head> a CSS
    html = FPSTR(PAGE_HEAD);

    // 2) Nahradíme %TITLE% v textu skutečným `title`
    html.replace("%TITLE%", title);

    // 3) Pak doplníme text do <h1> (část je už v PAGE_HEAD)
    html += title;

    // 4) Vložíme pokračování (uzavření <h1>)
    html += FPSTR(PAGE_HEAD_END);

    return html;
}

/**
 * Funkce, která vrátí základní HTML patičku (uloženou v PROGMEM).
 */
String makeHtmlFooter() {
    return FPSTR(PAGE_FOOTER);
}

// ------------------------------------------------------------
// 2) Samotné spuštění asynchronního webserveru
// ------------------------------------------------------------
static inline void startAsyncWebServer() {

    // Hlavní stránka "/"
    // ==================================================
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = makeHtmlHeader("FarmHub - Přehled");

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

      // Najdeme poslední data senzorů (soilDHT a light)
      SensorReading lastSoilDHT;
      bool foundSoilDHT = false;
      SensorReading lastLight;
      bool foundLight   = false;

      for (int i = dataBuffer.size() - 1; i >= 0; i--) {
        const auto &d = dataBuffer[i];
        if (!foundSoilDHT && d.sensorID == "soilDHTsensor") {
          lastSoilDHT = d;
          foundSoilDHT = true;
        }
        if (!foundLight && d.sensorID == "lightsensor") {
          lastLight = d;
          foundLight = true;
        }
        if (foundSoilDHT && foundLight) break;
      }

      html += "<hr><h3>Aktuální hodnoty</h3>";
      html += "<div class='table-container'><table>";
      html += "<tr><th>Půdní vlhkost (%)</th><th>Teplota (°C)</th><th>Vlhkost (%)</th><th>Světlo</th><th>Naposledy přijato</th></tr>";
      html += "<tr>";

      float soilVal = (foundSoilDHT ? lastSoilDHT.soilMoisture : 0.0);
      float tempVal = (foundSoilDHT ? lastSoilDHT.temperature : 0.0);
      float humVal  = (foundSoilDHT ? lastSoilDHT.humidity : 0.0);
      float lightVal= (foundLight   ? lastLight.lightLevel : 0.0);

      // Čas, který zobrazíme
      unsigned long latestTs = 0;
      if (foundSoilDHT) latestTs = lastSoilDHT.timestamp;
      if (foundLight && lastLight.timestamp > latestTs) {
        latestTs = lastLight.timestamp;
      }

      html += "<td>" + String(soilVal)  + "</td>";
      html += "<td>" + String(tempVal)  + "</td>";
      html += "<td>" + String(humVal)   + "</td>";
      html += "<td>" + String(lightVal) + "</td>";
      if (latestTs > 0) {
        html += "<td>" + formatEpochTime(latestTs) + "</td>";
      } else {
        html += "<td>N/A</td>";
      }
      html += "</tr></table></div>";

      // Zobrazíme volnou RAM (pro kontrolu)
      html += "<p>Volné místo v RAM: " + String(ESP.getFreeHeap()) + " B</p>";

      // Dokončíme stránku
      html += makeHtmlFooter();
      request->send(200, "text/html", html);
    });

    // Stránka "/charts" - pouze grafy
    // ==================================================
    server.on("/charts", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = makeHtmlHeader("FarmHub - Grafy");

      if(WiFi.status() != WL_CONNECTED){
        // Pokud není Wi-Fi, jen upozorníme, že grafy budou prázdné
        html += "<p><strong>Není připojení k Wi-Fi, grafy se nemusí vykreslit.</strong></p>";
      }

      // Vložíme 4 plátna pro 4 grafy
      html += "<hr><h3>Grafy senzorů</h3>";
      html += "<div class='chart-container'><canvas id='chartSoil'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartTemp'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartHum'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartLight'></canvas></div>";

      // Vložíme kód pro Chart.js
      html += FPSTR(CHART_JS);

      // Připravíme data pro JavaScript
      std::vector<float> soilTimes;
      std::vector<float> soilValues;
      std::vector<float> tempValues;
      std::vector<float> humValues;
      std::vector<float> lightTimes;
      std::vector<float> lightValues;

      // Projdeme celý dataBuffer a rozdělíme data podle sensorID
      for (auto &d : dataBuffer) {
        if(d.soilMoisture == 0 && d.temperature == 0 &&
           d.humidity == 0 && d.lightLevel == 0) {
          continue;
        }
        if(d.sensorID == "soilDHTsensor") {
          soilTimes.push_back((float)d.timestamp);
          soilValues.push_back(d.soilMoisture);
          tempValues.push_back(d.temperature);
          humValues.push_back(d.humidity);
        }
        else if(d.sensorID == "lightsensor") {
          lightTimes.push_back((float)d.timestamp);
          lightValues.push_back(d.lightLevel);
        }
      }

      // Omezení počtu bodů
      auto keepLastN = [&](std::vector<float> &times,
                           std::vector<float> &v1,
                           std::vector<float> &v2,
                           std::vector<float> &v3)
      {
        const size_t CHART_MAX_POINTS = 6; 
        if(times.size() > CHART_MAX_POINTS) {
          size_t startIndex = times.size() - CHART_MAX_POINTS;
          times.erase(times.begin(), times.begin() + startIndex);
          v1.erase(v1.begin(), v1.begin() + startIndex);
          if(!v2.empty()) v2.erase(v2.begin(), v2.begin() + startIndex);
          if(!v3.empty()) v3.erase(v3.begin(), v3.begin() + startIndex);
        }
      };
      keepLastN(soilTimes, soilValues, tempValues, humValues);
      {
        std::vector<float> dummy2, dummy3;
        keepLastN(lightTimes, lightValues, dummy2, dummy3);
      }

      // Seřadíme podle času (vzestupně)
      auto sortByTime = [](std::vector<float> &times,
                           std::vector<float> &v1,
                           std::vector<float> &v2,
                           std::vector<float> &v3){
        for(size_t i=0; i < times.size(); i++){
          for(size_t j=0; j < times.size()-1; j++){
            if(times[j] > times[j+1]) {
              std::swap(times[j], times[j+1]);
              std::swap(v1[j], v1[j+1]);
              if(!v2.empty()) std::swap(v2[j], v2[j+1]);
              if(!v3.empty()) std::swap(v3[j], v3[j+1]);
            }
          }
        }
      };
      if(!soilTimes.empty()) {
        sortByTime(soilTimes, soilValues, tempValues, humValues);
      }
      {
        std::vector<float> dummy2, dummy3;
        sortByTime(lightTimes, lightValues, dummy2, dummy3);
      }

      // Vygenerujeme JavaScript s poli dat
      html += "<script>";
      // soilTimes
      html += "const soilLabels = [";
      for(size_t i=0; i < soilTimes.size(); i++){
        html += String(soilTimes[i]);
        if(i < soilTimes.size() - 1) html += ",";
      }
      html += "];";
      // soilValues
      html += "const soilData = [";
      for(size_t i=0; i < soilValues.size(); i++){
        html += String(soilValues[i]);
        if(i < soilValues.size() - 1) html += ",";
      }
      html += "];";
      // tempValues
      html += "const tempData = [";
      for(size_t i=0; i < tempValues.size(); i++){
        html += String(tempValues[i]);
        if(i < tempValues.size() - 1) html += ",";
      }
      html += "];";
      // humValues
      html += "const humData = [";
      for(size_t i=0; i < humValues.size(); i++){
        html += String(humValues[i]);
        if(i < humValues.size() - 1) html += ",";
      }
      html += "];";
      // lightTimes
      html += "const lightLabels = [";
      for(size_t i=0; i < lightTimes.size(); i++){
        html += String(lightTimes[i]);
        if(i < lightTimes.size() - 1) html += ",";
      }
      html += "];";
      // lightValues
      html += "const lightData = [";
      for(size_t i=0; i < lightValues.size(); i++){
        html += String(lightValues[i]);
        if(i < lightValues.size() - 1) html += ",";
      }
      html += "];";

      // Zavoláme funkce pro vykreslení
      html += R"(
        drawSoilChart(soilLabels, soilData);
        drawTempChart(soilLabels, tempData);
        drawHumChart(soilLabels, humData);
        drawLightChart(lightLabels, lightData);
      )";
      html += "</script>";

      // Dokončení stránky
      html += makeHtmlFooter();
      request->send(200, "text/html", html);
    });

    // Stránka "/history" - poslední záznamy s volitelným limitem
    // ==================================================
    server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
      // Zkusíme načíst GET parametr "limit"
      int limit = 10; 
      if (request->hasParam("limit")) {
        limit = request->getParam("limit")->value().toInt();
        if (limit <= 0) limit = 10; // ochrana proti nesmyslu
      }

      String html = makeHtmlHeader("Poslední data");

      html += "<hr><h3>Poslední záznamy senzorů</h3>";
      // Form pro nastavení počtu řádků
      html += "<form method='GET' action='/history'>";
      html += "  <label>Počet záznamů k zobrazení:</label>";
      html += "  <input type='number' name='limit' value='" + String(limit) + "'>";
      html += "  <input type='submit' class='btn' value='Zobrazit'>";
      html += "</form>";

      // Tabulka
      html += "<div class='table-container'><table>";
      html += "<tr><th>SensorID</th><th>Soil(%)</th><th>Temp(°C)</th><th>Hum(%)</th><th>Light</th><th>Time</th></tr>";

      int count = dataBuffer.size();
      int rowsPrinted = 0;
      for(int i = count - 1; i >= 0 && rowsPrinted < limit; i--) {
        const auto &d = dataBuffer[i];
        if(d.soilMoisture == 0 &&
           d.temperature  == 0 &&
           d.humidity     == 0 &&
           d.lightLevel   == 0)
        {
          continue;
        }
        html += "<tr>";
        html += "<td>" + d.sensorID             + "</td>";
        html += "<td>" + String(d.soilMoisture) + "</td>";
        html += "<td>" + String(d.temperature)  + "</td>";
        html += "<td>" + String(d.humidity)     + "</td>";
        html += "<td>" + String(d.lightLevel)   + "</td>";
        html += "<td>" + formatEpochTime(d.timestamp) + "</td>";
        html += "</tr>";
        rowsPrinted++;
      }
      html += "</table></div>";

      // Dokončení
      html += makeHtmlFooter();
      request->send(200, "text/html", html);
    });

    // Stránka "/wifi"
    // ==================================================
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
      // Nejdříve zkontrolujeme stav asynchronního skenování
      checkAsyncScan();

      String html = makeHtmlHeader("Nastavení Wi-Fi");
      html += "<p><a class='btn' href='/startscan'>Načíst seznam sítí (async)</a></p>";

      if (g_isScanning) {
        html += "<p><strong>Probíhá skenování sítí...</strong></p>";
        html += "<p>Stránka se obnoví za <span id='timer'>10</span> s.</p>";
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
        if (g_foundNetworks <= 0) {
          html += "<p>Nebyly nalezeny žádné sítě, nebo sken selhal.</p>";
        } else {
          html += "<p>Nalezené sítě v okolí (" + String(g_foundNetworks) + "):</p>";
          html += "<form method='POST' action='/setwifi'>";
          html += "<div class='form-group'><label>Vyberte síť:</label>";
          html += "<select name='ssid'>";
          for (int i = 0; i < g_foundNetworks; i++) {
            String ssidFound = g_scannedSSIDs[i];
            html += "<option value='" + ssidFound + "'>";
            html += ssidFound;
            html += "</option>";
          }
          html += "</select></div>";
          html += "<div class='form-group'><label>Heslo:</label>";
          html += "<input type='password' name='pass'></div>";
          html += "<input type='submit' class='btn' value='Uložit Wi-Fi'>";
          html += "</form>";
        }
      } else {
        html += "<p>Nebylo spuštěno skenování, klikněte výše na tlačítko \"Načíst seznam sítí\".</p>";
      }

      // Formulář pro ruční zadání
      html += "<hr><p>Nebo zadat ručně (skrytou síť):</p>";
      html += "<form method='POST' action='/setwifi'>";
      html += "<div class='form-group'><label>SSID:</label>";
      html += "<input type='text' name='ssid'></div>";
      html += "<div class='form-group'><label>Heslo:</label>";
      html += "<input type='password' name='pass'></div>";
      html += "<input type='submit' class='btn' value='Uložit Wi-Fi'>";
      html += "</form>";

      html += makeHtmlFooter();
      request->send(200, "text/html", html);
    });

    server.on("/startscan", HTTP_GET, [](AsyncWebServerRequest *request){
      startAsyncScan();
      request->redirect("/wifi");
    });

    // Stránka "/watering"
    // ==================================================
    server.on("/watering", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = makeHtmlHeader("Nastavení zalévání");

      html += "<h3>Základní nastavení</h3>";
      html += "<form method='POST' action='/setwatering'>";
      // Režim zalévání
      html += "<div class='form-group'><label>Režim zalévání:</label>";
      html += "<select name='autoWatering'>";
      html += "<option value='false' " + String(!autoWatering ? "selected" : "") + ">Manuální</option>";
      html += "<option value='true' "  + String( autoWatering ? "selected" : "") + ">Automatický</option>";
      html += "</select></div>";
      // Prahová vlhkost
      html += "<div class='form-group'><label>Prahová vlhkost půdy (%):</label>";
      html += "<input type='number' step='1' name='moistureThreshold' value='" + String(moistureThreshold) + "'>";
      html += "</div>";
      // Množství vody
      html += "<div class='form-group'><label>Množství vody na jedno zalití (ml):</label>";
      html += "<input type='number' step='1' name='waterAmountML' value='" + String(waterAmountML) + "'>";
      html += "</div>";
      html += "<input type='submit' class='btn' value='Uložit nastavení'>";
      html += "</form>";

      if(!autoWatering) {
        html += "<hr><h3>Jednorázové zalití (manuální)</h3>";
        html += "<form method='POST' action='/runoneshot'>";
        html += "<div class='form-group'><label>Množství vody (ml):</label>";
        html += "<input type='number' step='1' name='oneTimeML' value='100'>";
        html += "</div>";
        html += "<input type='submit' class='btn' value='Zalít'>";
        html += "</form>";
      }

      html += makeHtmlFooter();
      request->send(200, "text/html", html);
    });

    // ----- POST Endpointy -----
    server.on("/setwifi", HTTP_POST, [](AsyncWebServerRequest *request){
      if(request->hasParam("ssid", true) && request->hasParam("pass", true)) {
        homeSsid = request->getParam("ssid", true)->value();
        homePass = request->getParam("pass", true)->value();

        saveUserConfig();
        request->redirect("/wifi");

        WiFi.disconnect();
        bool ok = connectToHomeWiFi(homeSsid, homePass);
        Serial.printf("connectToHomeWiFi => %s\n", ok ? "OK" : "FAIL");
      } else {
        request->redirect("/wifi");
      }
    });

    server.on("/setwatering", HTTP_POST, [](AsyncWebServerRequest *request){
      if(request->hasParam("autoWatering", true) &&
         request->hasParam("moistureThreshold", true) &&
         request->hasParam("waterAmountML", true))
      {
        autoWatering      = (request->getParam("autoWatering", true)->value() == "true");
        moistureThreshold = request->getParam("moistureThreshold", true)->value().toFloat();
        waterAmountML     = request->getParam("waterAmountML", true)->value().toInt();

        saveUserConfig();
        Serial.printf("Uloženo: autoWatering=%s, threshold=%.1f, waterML=%d\n",
                      autoWatering ? "true" : "false",
                      moistureThreshold,
                      waterAmountML);
      }
      request->redirect("/watering");
    });

    server.on("/runoneshot", HTTP_POST, [](AsyncWebServerRequest *request){
      if(request->hasParam("oneTimeML", true)) {
        double oneTimeML = request->getParam("oneTimeML", true)->value().toFloat();
        double secondsFloat = oneTimeML * 0.03; // 1 ml = 0.03 s (příklad)
        int secondsToPump  = (int)round(secondsFloat);

        Serial.printf("Jednorázové zalití: %.2f ml -> %d sekund.\n", oneTimeML, secondsToPump);
        broadcastRunPump(secondsToPump);
      }
      request->redirect("/watering");
    });

    // Dummy favicon
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "image/x-icon", "");
    });

    // Spustíme server
    server.begin();
    Serial.println("AsyncWebServer started on port 80");
}

#endif // FARM_HUB_WEBSERVER_H
