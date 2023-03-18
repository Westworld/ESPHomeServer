#include <Arduino.h>
#include <WiFiManager.h> 
//#include "WiFi.h"
//#include <ESP32WebServer.h>
//#include <WiFiUdp.h>
#include <time.h>  
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"

void handleStrom(void);
void handleFile(void);
String prepareHtmlPage(void);
void setSDFileName(char * name);
void logSDFile(void);
void WebSendDirList();
String ReadLastLine();
void webdebug(void);
void UDBDebug(String message);