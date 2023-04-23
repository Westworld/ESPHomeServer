#include <Arduino.h>
#include "WiFi.h"
#include <WiFiUdp.h>
#include <ESP32WebServer.h>
#include <time.h>  
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>

void MQTT_reconnect();
void handleStrom();
void logSDFile(bool headeronly);
void logDaySDFile(void);
String SDDaywriteHeader();
void webdebug(void);
void webtest(void);
void UDBDebug(String message);
void UDBDebug(const char *message);
void MQTT_Send(char const * topic, String value);
void MQTT_Send(char const * topic, float value); 
void MQTT_Send(char const * topic, int16_t value);
void MQTT_Send(char const * topic, long value);
void MQTT_callback(char* topic, byte* payload, unsigned int length);
void EMail_Send(String message);
void smtpCallback(SMTP_Status status);
void jsonstatussend();
void jsonstore();