#ifndef FARM_HUB_WEBSERVER_H
#define FARM_HUB_WEBSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <ESP8266HTTPClient.h>
#include "FarmHubWiFi.h"
#include "FarmHubConfig.h"
#include "FarmHubESP32CAM.h"
#include "FarmHubWebSocket.h"

// Uložené HTML řetězce, funkce makeHtmlHeader a makeHtmlFooter zůstávají stejné
AsyncWebServer server(80);

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

static const char PAGE_HEAD_END[] PROGMEM = R"rawliteral(
</h1>
)rawliteral";

static const char PAGE_FOOTER[] PROGMEM = R"rawliteral(
  </div>
</body>
</html>
)rawliteral";

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

// Pomocná funkce pro sestavení HTML hlavičky
String makeHtmlHeader(const String &title) {
    String html;
    html.reserve(2048);
    html = FPSTR(PAGE_HEAD);
    html.replace("%TITLE%", title);
    html += title;
    html += FPSTR(PAGE_HEAD_END);
    return html;
}

String makeHtmlFooter() {
    return FPSTR(PAGE_FOOTER);
}

//-----------------------------------------------------------
// Pomocná funkce pro načtení senzorových dat z ESP32-CAM
//-----------------------------------------------------------
String fetchSensorData() {
  return getSensorData(); // Funkce z FarmHubESP32CAM.h
}

//-----------------------------------------------------------
// Endpoint "/" - Přehled
//-----------------------------------------------------------
static inline void setupOverviewPage() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = makeHtmlHeader("FarmHub - Přehled");
      html += "<p>AP SSID: <strong>" + String(AP_SSID) + "</strong>, heslo: <strong>" + String(AP_PASS) + "</strong></p>";
      html += "<p>AP IP: <strong>" + WiFi.softAPIP().toString() + "</strong></p>";
      if(WiFi.status() == WL_CONNECTED) {
        html += "<p><strong>Připojeno k domácí síti:</strong> " + homeSsid + " (IP: " + WiFi.localIP().toString() + ")</p>";
      } else {
        html += "<p><strong>Nepřipojeno k domácí Wi-Fi.</strong></p>";
      }
      
      // Načtení senzorových dat z ESP32-CAM
      String sensorDataJSON = fetchSensorData();
      float soilVal = 0.0, tempVal = 0.0, humVal = 0.0, lightVal = 0.0;
      unsigned long latestTs = 0;
      StaticJsonDocument<2048> doc;
      DeserializationError err = deserializeJson(doc, sensorDataJSON);
      if (!err) {
        JsonArray readings = doc["readings"].as<JsonArray>();
        if (readings.size() > 0) {
          JsonObject lastReading = readings[readings.size()-1];
          soilVal  = lastReading["soilMoisture"] | 0.0;
          tempVal  = lastReading["temperature"]  | 0.0;
          humVal   = lastReading["humidity"]     | 0.0;
          lightVal = lastReading["lightLevel"]   | 0.0;
          latestTs = lastReading["timestamp"]    | 0;
        }
      } else {
        html += "<p>Chyba při načítání dat z ESP32-CAM.</p>";
      }
      
      html += "<hr><h3>Aktuální hodnoty</h3>";
      html += "<div class='table-container'><table>";
      html += "<tr><th>Půdní vlhkost (%)</th><th>Teplota (°C)</th><th>Vlhkost (%)</th><th>Světlo</th><th>Naposledy přijato</th></tr>";
      html += "<tr>";
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
      
      // Zobrazení aktuální fotografie z ESP32-CAM
      html += "<hr><h3>Aktuální fotografie</h3>";
      html += "<img src='" + getCurrentImageURL() + "' alt='Aktuální fotografie' width='320' />";
      
      html += "<p>Volné místo v RAM: " + String(ESP.getFreeHeap()) + " B</p>";
      html += makeHtmlFooter();
      request->send(200, "text/html", html);
  });
}

//-----------------------------------------------------------
// Endpoint "/charts" - Grafy
//-----------------------------------------------------------
static inline void setupChartsPage() {
  server.on("/charts", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = makeHtmlHeader("FarmHub - Grafy");
      if(WiFi.status() != WL_CONNECTED){
        html += "<p><strong>Nepřipojeno k Wi-Fi, grafy se nemusí vykreslit.</strong></p>";
      }
      
      html += "<hr><h3>Grafy senzorů</h3>";
      html += "<div class='chart-container'><canvas id='chartSoil'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartTemp'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartHum'></canvas></div>";
      html += "<div class='chart-container'><canvas id='chartLight'></canvas></div>";
      html += FPSTR(CHART_JS);
      
      // Načtení senzorových dat z ESP32-CAM
      String sensorDataJSON = fetchSensorData();
      StaticJsonDocument<4096> doc;
      DeserializationError err = deserializeJson(doc, sensorDataJSON);
      std::vector<float> soilTimes, soilValues, tempValues, humValues;
      std::vector<float> lightTimes, lightValues;
      
      if (!err) {
        JsonArray readings = doc["readings"].as<JsonArray>();
        for (JsonObject r : readings) {
          unsigned long ts = r["timestamp"] | 0;
          if (r["sensorID"] == "soilDHTsensor") {
            soilTimes.push_back((float)ts);
            soilValues.push_back(r["soilMoisture"] | 0.0);
            tempValues.push_back(r["temperature"] | 0.0);
            humValues.push_back(r["humidity"] | 0.0);
          } else if (r["sensorID"] == "lightsensor") {
            lightTimes.push_back((float)ts);
            lightValues.push_back(r["lightLevel"] | 0.0);
          }
        }
      }
      
      // Omezení počtu bodů a řazení (podobně jako dříve)
      const size_t CHART_MAX_POINTS = 6;
      auto trimData = [&](std::vector<float> &times, std::vector<float> &values) {
        if(times.size() > CHART_MAX_POINTS) {
          size_t startIndex = times.size() - CHART_MAX_POINTS;
          times.erase(times.begin(), times.begin() + startIndex);
          values.erase(values.begin(), values.begin() + startIndex);
        }
      };
      trimData(soilTimes, soilValues);
      trimData(lightTimes, lightValues);
      
      auto sortData = [&](std::vector<float> &times, std::vector<float> &v1, std::vector<float> &v2, std::vector<float> &v3) {
        for (size_t i = 0; i < times.size(); i++) {
          for (size_t j = 0; j < times.size()-1; j++) {
            if(times[j] > times[j+1]) {
              std::swap(times[j], times[j+1]);
              std::swap(v1[j], v1[j+1]);
              std::swap(v2[j], v2[j+1]);
              std::swap(v3[j], v3[j+1]);
            }
          }
        }
      };
      if(!soilTimes.empty()) {
        sortData(soilTimes, soilValues, tempValues, humValues);
      }
      
      html += "<script>";
      html += "const soilLabels = [";
      for(size_t i = 0; i < soilTimes.size(); i++){
        html += String(soilTimes[i]);
        if(i < soilTimes.size() - 1) html += ",";
      }
      html += "];";
      
      html += "const soilData = [";
      for(size_t i = 0; i < soilValues.size(); i++){
        html += String(soilValues[i]);
        if(i < soilValues.size() - 1) html += ",";
      }
      html += "];";
      
      html += "const tempData = [";
      for(size_t i = 0; i < tempValues.size(); i++){
        html += String(tempValues[i]);
        if(i < tempValues.size() - 1) html += ",";
      }
      html += "];";
      
      html += "const humData = [";
      for(size_t i = 0; i < humValues.size(); i++){
        html += String(humValues[i]);
        if(i < humValues.size() - 1) html += ",";
      }
      html += "];";
      
      html += "const lightLabels = [";
      for(size_t i = 0; i < lightTimes.size(); i++){
        html += String(lightTimes[i]);
        if(i < lightTimes.size() - 1) html += ",";
      }
      html += "];";
      
      html += "const lightData = [";
      for(size_t i = 0; i < lightValues.size(); i++){
        html += String(lightValues[i]);
        if(i < lightValues.size() - 1) html += ",";
      }
      html += "];";
      
      html += R"(
        drawSoilChart(soilLabels, soilData);
        drawTempChart(soilLabels, tempData);
        drawHumChart(soilLabels, humData);
        drawLightChart(lightLabels, lightData);
      )";
      html += "</script>";
      
      html += makeHtmlFooter();
      request->send(200, "text/html", html);
  });
}

//-----------------------------------------------------------
// Endpoint "/history" - Historie záznamů
//-----------------------------------------------------------
static inline void setupHistoryPage() {
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
      int limit = 10;
      if (request->hasParam("limit")) {
        limit = request->getParam("limit")->value().toInt();
        if (limit <= 0) limit = 10;
      }
      String html = makeHtmlHeader("Historie dat");
      html += "<hr><h3>Poslední záznamy senzorů</h3>";
      html += "<form method='GET' action='/history'>";
      html += "  <label>Počet záznamů k zobrazení:</label>";
      html += "  <input type='number' name='limit' value='" + String(limit) + "'>";
      html += "  <input type='submit' class='btn' value='Zobrazit'>";
      html += "</form>";
      
      // Načtení senzorových dat z ESP32-CAM
      String sensorDataJSON = fetchSensorData();
      StaticJsonDocument<4096> doc;
      DeserializationError err = deserializeJson(doc, sensorDataJSON);
      html += "<div class='table-container'><table>";
      html += "<tr><th>SensorID</th><th>Půdní vlhkost (%)</th><th>Teplota (°C)</th><th>Vlhkost (%)</th><th>Světlo</th><th>Time</th></tr>";
      
      if (!err) {
        JsonArray readings = doc["readings"].as<JsonArray>();
        int total = readings.size();
        int start = (total > limit) ? total - limit : 0;
        for (int i = total - 1; i >= start; i--) {
          JsonObject r = readings[i];
          html += "<tr>";
          html += "<td>" + String((const char*)r["sensorID"]) + "</td>";
          html += "<td>" + String(r["soilMoisture"] | 0.0) + "</td>";
          html += "<td>" + String(r["temperature"] | 0.0) + "</td>";
          html += "<td>" + String(r["humidity"] | 0.0) + "</td>";
          html += "<td>" + String(r["lightLevel"] | 0.0) + "</td>";
          html += "<td>" + formatEpochTime(r["timestamp"] | 0) + "</td>";
          html += "</tr>";
        }
      } else {
        html += "<tr><td colspan='6'>Chyba při načítání dat.</td></tr>";
      }
      html += "</table></div>";
      html += makeHtmlFooter();
      request->send(200, "text/html", html);
  });
}

//-----------------------------------------------------------
// Endpointy "/wifi" a "/watering" zůstávají nezměněny
//-----------------------------------------------------------
static inline void setupWifiPage() {
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
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
            html += "<option value='" + ssidFound + "'>" + ssidFound + "</option>";
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
}

static inline void setupWateringPage() {
  server.on("/watering", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = makeHtmlHeader("Nastavení zalévání");
      html += "<h3>Základní nastavení</h3>";
      html += "<form method='POST' action='/setwatering'>";
      html += "<div class='form-group'><label>Režim zalévání:</label>";
      html += "<select name='autoWatering'>";
      html += "<option value='false' " + String(!autoWatering ? "selected" : "") + ">Manuální</option>";
      html += "<option value='true' "  + String(autoWatering ? "selected" : "") + ">Automatický</option>";
      html += "</select></div>";
      html += "<div class='form-group'><label>Prahová vlhkost půdy (%):</label>";
      html += "<input type='number' step='1' name='moistureThreshold' value='" + String(moistureThreshold) + "'>";
      html += "</div>";
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
        double secondsFloat = oneTimeML * 0.03;
        int secondsToPump  = (int)round(secondsFloat);
        Serial.printf("Jednorázové zalití: %.2f ml -> %d sekund.\n", oneTimeML, secondsToPump);
        broadcastRunPump(secondsToPump);
      }
      request->redirect("/watering");
  });
}

//-----------------------------------------------------------
// Spuštění webového serveru
//-----------------------------------------------------------
static inline void startAsyncWebServer() {
  setupOverviewPage();
  setupChartsPage();
  setupHistoryPage();
  setupWifiPage();
  setupWateringPage();
  
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "image/x-icon", "");
  });
  
  server.begin();
  Serial.println("AsyncWebServer spuštěn na portu 80");
}

#endif // FARM_HUB_WEBSERVER_H
