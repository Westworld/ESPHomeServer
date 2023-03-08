#include <Arduino.h>
#include <WiFiManager.h> 
//#include <WiFiUdp.h>
#include <time.h>  
#include <ArduinoOTA.h>


WiFiManager wifiManager;
#define NTP_SERVER "de.pool.ntp.org"
#define DefaultTimeZone "CET-1CEST,M3.5.0/02,M10.5.0/03"  
String MY_TZ = DefaultTimeZone ;
const char* wifihostname = "ESPHomeServer";

void setTimeZone(String TimeZone) {
  struct tm local;
  configTzTime(TimeZone.c_str(), NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
  getLocalTime(&local, 10000);      // Versuche 10 s zu Synchronisieren
  #ifdef webdebug  
    Serial.println("TimeZone: "+TimeZone);
    Serial.println(&local, "%A, %B %d %Y %H:%M:%S");
  #endif  
 
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Config mode");
  String ipaddress = WiFi.softAPIP().toString();
  Serial.println(ipaddress);
}

void setup() {
  Serial.begin(115200);

  wifiManager.setHostname(wifihostname);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConnectRetries(10);
  wifiManager.setConnectTimeout(10);
  wifiManager.autoConnect(wifihostname); 

  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }
  String ipaddress = WiFi.localIP().toString();
  Serial.println(ipaddress);  

    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  setTimeZone(MY_TZ);
}

void loop() {
  ArduinoOTA.handle();
}