/*
   LightModule.ino – samostatný firmware pro ESP8266 na ovládání světla.
   - Připojí se k WiFi (FarmHub síti), kde je dostupný internet.
   - Zkusí synchronizovat čas přes NTP.
   - WebSocketem přijímá nastavení: manuální/automatický režim, čas zapnutí/vypnutí,
     možnost svítit jen když < 50 lux.
   - Lokálně měří lux (analogRead(A0)) a řídí pin D1 (relé).
*/

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <time.h>            // Pro NTP synchronizaci

// Piny
static const int PIN_LIGHT = D1;  // D1 -> ovládání relé

// Globální proměnné konfigurace (od Hubu)
bool autoLight        = false;
int  lightStartHour   = 8;
int  lightStartMinute = 0;
int  lightEndHour     = 20;
int  lightEndMinute   = 0;
bool lightOnlyIfDark  = false;
bool manualLightOn    = false;

// --- NTP: nastavení ---
static const char* ntpServer = "195.113.144.201";
static const long  gmtOffset_sec = 3600;     // Pro ČR: GMT+1 => 3600
static const int   daylightOffset_sec = 0;   // Letní čas ručně, anebo 3600 pokud je letní;

// Připojení k WiFi
const char* ssid = "FarmHub-AP";  // nebo vaše domácí síť
const char* pass = "farmhub123";  //

// IP adresa hubu (pro WebSocket)
IPAddress hubIP(192,168,4,1);     // pokud je v AP módu => 192.168.4.1, příp. jiná

// WebSocket klient
WebSocketsClient webSocket;

// -------------------------------------------------------------------------------------
// Pomocné funkce

// Vrátí počet minut od půlnoci podle zadané hodiny/minuty
int toMinutesFromMidnight(int h, int m) {
  return h * 60 + m;
}

// Zjistí, zda je aktuální lokální čas (podle RTC) uvnitř intervalu start..end
bool isInLightingWindow() {
  // Získání lokálního času z RTC (synced z NTP)
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);
  if (!timeInfo) {
    // Pokud se NTP nepovedlo, timeInfo může být nullptr,
    // fallback: považujme to, jako by byl 0:0
    return false;
  }
  int currentHour   = timeInfo->tm_hour;
  int currentMinute = timeInfo->tm_min;
  
  int nowMin   = toMinutesFromMidnight(currentHour, currentMinute);
  int startMin = toMinutesFromMidnight(lightStartHour, lightStartMinute);
  int endMin   = toMinutesFromMidnight(lightEndHour,   lightEndMinute);

  if (startMin < endMin) {
    return (nowMin >= startMin && nowMin < endMin);
  } else {
    // rozmezí přes půlnoc (např. 20:00 -> 06:00)
    if (nowMin >= startMin || nowMin < endMin) {
      return true;
    } else {
      return false;
    }
  }
}

// -------------------------------------------------------------------------------------
// WebSocket callback – příjem příkazů z FarmHubu

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("[WS] Připojeno k Hubu!");
      // Můžeme si vyžádat nastavení LIGHT_SETTINGS, ale obvykle ho Hub pošle automaticky.
      break;

    case WStype_DISCONNECTED:
      Serial.println("[WS] Odpojeno!");
      {  WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, pass);
        Serial.print("Connecting to WiFi");
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 20000)) {
          delay(500);
          Serial.print(".");
        }
      }
      break;

    case WStype_TEXT:
      payload[length] = 0; // ukončit řetězec
      Serial.printf("[WS] Message: %s\n", (char*)payload);

      // Rozparsovat JSON
      {
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
          String cmd = doc["cmd"] | "";
          if (cmd == "LIGHT_SETTINGS") {
            // Načteme si novou konfiguraci
            manualLightOn    = doc["manualOn"]       | false;
            autoLight        = doc["autoLight"]      | false;
            lightStartHour   = doc["startHour"]      | 8;
            lightStartMinute = doc["startMinute"]    | 0;
            lightEndHour     = doc["endHour"]        | 20;
            lightEndMinute   = doc["endMinute"]      | 0;
            lightOnlyIfDark  = doc["onlyIfDark"]     | false;

            Serial.println("[WS] Přijal LIGHT_SETTINGS z Hubu:");
            Serial.printf("  manualOn=%d, autoLight=%d, %02d:%02d->%02d:%02d, onlyDark=%d\n",
                manualLightOn, autoLight,
                lightStartHour, lightStartMinute,
                lightEndHour,   lightEndMinute,
                lightOnlyIfDark
            );
          } else if (cmd == "TIME_SYNC") {
            // Pokud byste chtěli přebírat čas z Hubu, tady ho zpracujete.
            // (Zde nepotřebujeme, protože bereme NTP.)
          }
        }
      }
      break;

    default:
      break;
  }
}

// -------------------------------------------------------------------------------------
// Setup & Loop

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LIGHT, INPUT);

  // Připojení k WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 20000)) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Připojeno!");
    Serial.print("[WiFi] IP adresa: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Gateway:    ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[WiFi] DNS:        ");
    Serial.println(WiFi.dnsIP());

    // Zkusíme nejprve zjistit, zda funguje DNS překlad
    IPAddress ntpServerIP;
    if (WiFi.hostByName(ntpServer, ntpServerIP) == 1) {
      Serial.print("[NTP] DNS OK, NTP server: ");
      Serial.println(ntpServerIP);
    } else {
      Serial.print("[NTP] Nelze přeložit NTP server: ");
      Serial.println(ntpServer);
    }

    // Nastavíme parametry pro NTP a spustíme synchronizaci
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("[NTP] Připojuji se k NTP serveru...");
    delay(2000);

    // Otestujeme, zda jsme čas úspěšně získali
    int tries = 0;
    while (!time(nullptr) && tries < 15) {
      Serial.println("[NTP] Čekám na čas...");
      delay(1000);
      tries++;
    }
    if (time(nullptr)) {
      Serial.println("[NTP] Synchronizace času OK");
      // Můžeme vypsat datum/čas
      time_t now = time(nullptr);
      Serial.print("[NTP] Lokální čas: ");
      Serial.println(ctime(&now));
    } else {
      Serial.println("[NTP] Nepovedlo se získat čas (vypršel čas).");
    }
  } else {
    Serial.println("\n[WiFi] Nepodařilo se připojit k WiFi (timeout).");
  }

  // Nastavení WebSocket klienta (předpoklad: Hub běží na IP hubIP, port 80, path /ws)
  webSocket.begin(hubIP.toString(), 80, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(3000); // Reconnect každý 3 s
}

void loop() {
  // WebSocket zpracování
  webSocket.loop();

  // (1) – Pokud je manuální zapnutí, to má přednost
  if (manualLightOn) {
    pinMode(PIN_LIGHT, OUTPUT);
  }
  else {
    // (2) – Pokud není manuál, koukneme na autoLight
    if (autoLight) {
      bool inWindow = isInLightingWindow();
      if (inWindow) {
        if (lightOnlyIfDark) {
          // Přečteme lokální senzor (analogRead(A0)) – příklad
          int sensorVal = analogRead(A0); 
          // Tady je třeba dle rozsahu čidla definovat podmínku < 50
          // (Pokud je LDR zapojen jinak, může být 0=světlo atd., v praxi vyžaduje kalibraci.)
          if (sensorVal < 50) {
            pinMode(PIN_LIGHT, OUTPUT);
          } else {
            pinMode(PIN_LIGHT, INPUT);
          }
        } else {
          // Svítit bez ohledu na okolní světlo
          pinMode(PIN_LIGHT, OUTPUT);
        }
      }
      else {
        // Mimo nastavený interval
        pinMode(PIN_LIGHT, INPUT);
      }
    }
    else {
      // (3) – Ani manuál, ani auto => vypnuto
      pinMode(PIN_LIGHT, INPUT);
    }
  }

  // Lze doplnit periodické odeslání stavu (lux, zapnuto/vypnuto) na Hub
  // ...
}
