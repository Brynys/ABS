#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "mbedtls/base64.h"

// --- Původní funkcionality (např. práce se SPIFFS, čtení/sdílení senzorových dat) ---
// #include "YourSensorModule.h" 
// #include "YourFileStorageModule.h" 
// …

// --- Konfigurace WiFi ---
const char* ssid = "FarmHub-AP";
const char* password = "farmhub123";

// --- Konfigurace HTTP endpointu ---
// Upravte tuto adresu dle nastavení vašeho hubu, kde běží AsyncWebServer s /upload endpointem
const char* serverName = "http://192.168.4.1/upload";

// --- Funkce pro base64 kódování ---
String base64Encode(const uint8_t* input, size_t length) {
  size_t output_length = 0;
  // Zjistíme, jak velký buffer potřebujeme
  mbedtls_base64_encode(NULL, 0, &output_length, input, length);
  // Alokujeme o jeden bajt více pro ukončovací znak
  char* output = new char[output_length + 1];
  // Předáme output_length+1 jako velikost bufferu
  if (mbedtls_base64_encode((unsigned char*)output, output_length + 1, &output_length, input, length) == 0) {
    output[output_length] = '\0';
    String encoded = String(output);
    delete[] output;
    return encoded;
  }
  delete[] output;
  return "";
}


// --- Inicializace kamery ---
// Ponechte nebo upravte konfiguraci kamery dle svých potřeb.
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // Použijeme QQVGA pro menší rozlišení – zajišťuje, že velikost JPEG nebude příliš velká
  config.frame_size   = FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;  // Nastavte kvalitu dle potřeby
  config.fb_count     = 1;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera se nepodařilo inicializovat, chyba 0x%x\n", err);
    return;
  }
  Serial.println("Kamera inicializována");
}

// --- Funkce pro zachycení snímku a odeslání na HTTP endpoint ---
void broadcastPhoto() {
  // Pořízení fotografie
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Chyba při pořízení fotografie");
    return;
  }
  
  // Původní funkcionalita: např. uložení fotografií, zpracování dat apod.
  // savePhotoToSPIFFS(fb->buf, fb->len);
  
  // Zakódujeme JPEG data do base64 a přidáme prefix "image :" (s mezerou)
  String encodedImage = base64Encode(fb->buf, fb->len);

  if(encodedImage == "") { 
    Serial.print("chyba při překódování");
  }
  
  // Uvolníme buffer po zpracování
  esp_camera_fb_return(fb);
  
  // Vytvoření JSON objektu, který obsahuje pole "image"
  String postData = "{\"image\":\"" + encodedImage + "\"}";
  
  // Odeslání HTTP POST požadavku na endpoint /upload
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(postData);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Data odeslána, HTTP response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.println("Chyba při odesílání dat: " + http.errorToString(httpResponseCode));
  }
  http.end();
  
  Serial.println("Odeslána HTTP POST zpráva s fotografií");
}

void setup() {
  Serial.begin(115200);
  
  // Připojení k WiFi
  Serial.print("Připojuji se k WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPřipojeno!");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
  
  // Inicializace kamery
  initCamera();
  
  // Inicializujte zde i další vaše moduly (např. SPIFFS, senzory apod.)
  // initSPIFFS();
  // initSensorModule();
}

void loop() {
  // Pravidelně odesíláme aktuální fotografii na HTTP endpoint
  broadcastPhoto();
  
  // Interval mezi odesláním – upravte dle potřeby
  delay(5000);
}
