#include <Arduino.h>
#include "WiFi.h"
#include <WiFiUdp.h>
#include <ESP32WebServer.h>
#include <time.h>  
#include <ArduinoOTA.h>
#include <HTTPClient.h>

void MQTT_reconnect();
void handleStrom();
void handleFile(void);
String prepareHtmlPage(void);
void setSDFileName(char * name);
void logSDFile(bool headeronly);
void logDaySDFile(void);
String SDDaywriteHeader();
void WebSendDirList();
String ReadLastLine();
void ReadAndParseLastLine();
void webdebug(void);
void UDBDebug(String message);
void MQTT_Send(char const * topic, String value);
void MQTT_Send(char const * topic, float value); 
void MQTT_Send(char const * topic, int16_t value);
void MQTT_Send(char const * topic, long value);
void MQTT_callback(char* topic, byte* payload, unsigned int length);