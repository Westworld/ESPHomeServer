#include "main.h"

//#include "FS.h"
//#include "SD.h"
#include "SPI.h"
#include <PubSubClient.h>
#include <HTTPClient.h>

#include <Adafruit_NeoPixel.h>

#define PIN        8
#define NUMPIXELS  1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);




#include <iostream>
#include <fstream>

#include "waage.h"
//#include "wasser.h"
#include "garage.h"
#include "Strom.h"

/*
selbst: http://192.168.0.59/
http://esphomeserver.fritz.box/sendfile?Folder
http://esphomeserver.fritz.box/debug?Delete

Rolladen 3 - Lets do it - 192.168.0.85
Dreamer Power - 192.168.0.107
Waschmaschine - 192.168.0.111
Rule1 war:
15:06:46 MQT: Haus/Waschmaschine/RESULT = {"Rule1":"ON","Once":"OFF","StopOnError":"OFF","Free":317,"Rules":"on Power1#State=1 do websend [192.168.0.34:8000] /4DAction/Strom?job=Waschmaschine&power=1 endon on Power1#State=0 do websend [192.168.0.34:8000] /4DAction/Strom?job=Waschmaschine&power=0 endon "}


*/

#define StorageVersion 1

extern void Homematic_Set(String device, int8_t status);

#define UDPDEBUG 1
#ifdef UDPDEBUG
WiFiUDP udp;
const char * udpAddress = "192.168.0.95";
const int udpPort = 19814;
#endif

WiFiClient wifiClient;

#define NTP_SERVER "de.pool.ntp.org"
#define DefaultTimeZone "CET-1CEST,M3.5.0/02,M10.5.0/03"  
String MY_TZ = DefaultTimeZone ;
const char* wifihostname = "ESPHomeServer";

int8_t mqtterrorcounter=0;

const char* mqtt_server = "192.168.0.46";
// MQTT_User and MQTT_Pass defined via platform.ini, external file, not uploaded to github
PubSubClient mqttclient(wifiClient);
bool startuprestore = false; // to receive loaded data just once

struct tm timeinfo;
char SDLog_Lasthour = -1;
char SDLog_Lastmin = -1;
char time_last_restart_day = -1;
char SDLog_Lastday = -1;

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The SMTP Session object used for Email sending */
SMTPSession smtp;
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

ESP32WebServer server(80);
Sensor * sensor[10];
int16_t sensorcounter=0;
Waage * waage;
//Wasser * wasser;
Garage * garage;
Strom * strom;

#define Jsonsize 1024

void setTimeZone(String TimeZone) {
  configTzTime(TimeZone.c_str(), NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
  // aufruf für 10 Sekunden, bis Zeit aktualisiert.
  // verlangsamt booten, ja

  for (short i=0;i<100;i++) {
      getLocalTime(&timeinfo, 10000);      // Versuche 10 s zu Synchronisieren
       delay(100);
  } 
     UDBDebug("setTimeZone-Zeit: "+String(timeinfo.tm_hour)+":"+(timeinfo.tm_min));  

  #ifdef webdebug  
    Serial.println("TimeZone: "+TimeZone);
    Serial.println(&local, "%A, %B %d %Y %H:%M:%S");
  #endif  
}

void WifiConnect() {
    WiFi.setHostname(wifihostname);  
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    short counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (++counter > 50)
        ESP.restart();
    }

    IPAddress ip = WiFi.localIP();
    Serial.println(F("WiFi connected"));
    Serial.println(ip);
}

void setup() {
  Serial.begin(115200);

  WifiConnect() ;

  pixels.clear();
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();

    ArduinoOTA
    .onStart([]() {
      jsonstore();
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      UDBDebug("Start update");
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

 // MQTT
 Serial.printf("vor MQTT");
    mqttclient.setServer(mqtt_server, 1883);
    mqttclient.setCallback(MQTT_callback);
    mqttclient.setBufferSize(1024);
   if (mqttclient.connect(wifihostname, MQTT_User, MQTT_Pass)) {
      //mqttclient.publish("outTopic","hello world");
      UDBDebug("MQTT connect successful"); 
      //const char *TOPIC = "Haus/Sonoff POW R1/R2/Dreamer_Power/#";
      const char *TOPIC = "garage/TargetDoorState/#";
      mqttclient.subscribe(TOPIC);
      const char *TOPIC2 = "HomeServer/#";
      mqttclient.subscribe(TOPIC2);
      //mqttclient.subscribe("Haus/+/Power");
   }  
    else
       UDBDebug("MQTT connect error");  

   Serial.printf("nach MQTT2");   

  SDLog_Lastday = timeinfo.tm_mday;

 // DEVICES
  waage = new Waage();
  sensor[sensorcounter++] = waage;
  //wasser = new Wasser();
  //sensor[sensorcounter++] = wasser;
  garage = new Garage();
  sensor[sensorcounter++] = garage;
  strom = new Strom();
  sensor[sensorcounter++] = strom;

// Web request handler
// aufruf von Garage, Raspi 63 /var/www/html/GarageStatus.php
  server.on("/4DAction/Strom", handleStrom);
  server.on("/debug", webdebug);
  server.on("/test", webtest);
 // server.on("/", handleFile);
  server.begin();

  // Email
  smtp.callback(smtpCallback);

  Serial.println("all systems go...");
  UDBDebug("all systems go...");
}


void loop() { 
  if (WiFi.status() != WL_CONNECTED)
    WifiConnect();
  ArduinoOTA.handle();
  
  server.handleClient();
  
  if (!mqttclient.loop()) {
    if (mqttclient.connect(wifihostname, MQTT_User, MQTT_Pass)) {
      UDBDebug("MQTT reconnect successful"); 
      mqtterrorcounter=0;
   }  
    else
       UDBDebug("MQTT reconnect error");  
       if (mqtterrorcounter++ > 5)
        ESP.restart();
  };
  
  static int32_t zeit=0, altzeit=0, gesamtzeit=0;
  static int32_t zeitcounter=0;

  zeit = millis();
/*
  for (int16_t i=0; i<sensorcounter;i++) {  
    sensor[i]->Run(zeit);
  }
*/

  if(!getLocalTime(&timeinfo, 10000)){
    Serial.println("Failed to obtain time");
    UDBDebug("Failed to obtain time");  
  }
  else
  { 
    //UDBDebug("Zeit: "+String(timeinfo.tm_hour)+":"+(timeinfo.tm_min));  
    
      if (timeinfo.tm_hour != SDLog_Lasthour)  {  
         
        SDLog_Lasthour = timeinfo.tm_hour;
        SDLog_Lastmin = timeinfo.tm_min;

        if (timeinfo.tm_min <= 1) {
            for (int16_t i=0; i<sensorcounter;i++) {  
              sensor[i]->runStunde();
            }
            jsonstore();  // log, hourly
        }    
      }

      if (timeinfo.tm_mday != SDLog_Lastday)  {
        UDBDebug("start run day");
        SDLog_Lastday = timeinfo.tm_mday;
        jsonstatussend(); // data, daily, number from previous day
        for (int16_t i=0; i<sensorcounter;i++) {  
          sensor[i]->runDay();
        } 
        jsonstore(); 
        
        String memory = "getFreeHeap: "+String(ESP.getFreeHeap());
        memory += "  getMaxAllocHeap: ";
        memory += String(ESP.getMaxAllocHeap());
        UDBDebug(memory);
        //UDBDebug("daily restart");
        delay(500);
        //ESP.restart();

      }      

/*
      if ((time_last_restart_day == 6) && (timeinfo.tm_wday == 0))
      {
        // time for restart !! one restart every week
        jsonstore();
        UDBDebug("weekly restart");
        ESP.restart();
      }
      else time_last_restart_day = timeinfo.tm_wday;
*/

  }  // time success received
}

void handleStrom() {

  // aufruf von Garage, Raspi 63 /var/www/html/GarageStatus.php

  if (server.hasArg("Job"))  {
    String job = server.arg("Job");
    int8_t joblength = job.length();
    
      for (int16_t i=0; i<sensorcounter;i++) {  
        if (sensor[i]->HandleWebCall(job, joblength))
          return;
      }

   if (job == "sendtemp") {
        String name="";
        String name2="";
        if (server.hasArg("valuename")) {
          name = server.arg("valuename"); 
        }
        String STemp = "127";
        if (server.hasArg("value")) {
          STemp = server.arg("value"); 
        }
        String SName2 = "";
        if (server.hasArg("name")) {
          name2 = server.arg("name"); 
        }
        if ((name2 == "temp2_wz") && (name == "Temperature") && (STemp != "127")) {
          MQTT_Send("HomeServer/WZ2/Temp",STemp);  
          server.send(200, "text/html", String("OK temp"));  
          return;
        }
        if ((name2 == "temp2_wz") && (name == "Humidity") && (STemp != "")) {
          MQTT_Send("HomeServer/WZ2/Humidity",STemp);  
          server.send(200, "text/html", String("OK temp"));  
          return;
        }
      }

    String message;
    message = String("Error unknown job ")+job;
    message += String(" url ");
    message += String(server.uri());
    message += String(" ip ");
    message += server.client().remoteIP().toString();
    server.send(404, "text/html", message);
    UDBDebug(message); 

    return;
  } 
  UDBDebug(String("Error job missing")); 
  server.send(404, "text/html", String("Error job missing"));
}

void webdebug() {
  String message= "";

  if (server.hasArg("Job"))  {
    String job = server.arg("Job");
    if (job == "resetday") {
      strom->runDay();
      message="resetday";
    }
    if (job == "resetstunde") {
      strom->runStunde();
      message="resetstunde";
    }
  }
  else {  
      for (int16_t i=0; i<sensorcounter; i++) {
        message = message + sensor[i]->serialize() + "\n";
      }
      message = message + "\ncurtime: "+millis();
  }

  server.send(200, "text/plain", message);
}

void webtest() {
    UDBDebug("webtest");

    const int capacity = 500; 
    StaticJsonDocument<capacity> doc;

    garage->ToJson(doc.createNestedObject("garage"));
   // wasser->ToJson(doc.createNestedObject("wasser"));
    waage->ToJson(doc.createNestedObject("waage"));
    strom->ToJson(doc.createNestedObject("strom"));

    //char output[500];
    String output;
    serializeJson(doc, output);

    server.send(200, "text/plain", output);
}

void jsonstatussend() {
      StaticJsonDocument<Jsonsize> doc;

    garage->StatusToJson(doc.createNestedObject("garage"));
    //wasser->StatusToJson(doc.createNestedObject("wasser"));
    waage->StatusToJson(doc.createNestedObject("waage"));
    strom->StatusToJson(doc.createNestedObject("strom"));

    doc["stamp"] = String(timeinfo.tm_hour)+":"+(timeinfo.tm_min);

    String output;
    serializeJson(doc, output);

    MQTT_Send("HomeServer/Server/Data", output);
}

void jsonstore() {
     // backup hourly, reload on startup
      StaticJsonDocument<Jsonsize> doc;

    garage->ToJson(doc.createNestedObject("garage"));
    //wasser->ToJson(doc.createNestedObject("wasser"));
    waage->ToJson(doc.createNestedObject("waage"));
    strom->ToJson(doc.createNestedObject("strom"));

    doc["stamp"] = String(timeinfo.tm_hour)+":"+(timeinfo.tm_min);

    String output;
    serializeJson(doc, output);


    MQTT_Send("HomeServer/Server/Log", output);
    
}

void jsonreceive(String value) {
  // reads HomeServer/Server/Log
    StaticJsonDocument<Jsonsize> doc;

    DeserializationError error = deserializeJson(doc, value);

    if (error) {
      String errortext = "deserializeJson() failed: ";
      errortext += error.c_str();
      UDBDebug(errortext);
      return;
    }

UDBDebug("jsonreceive 1");

    garage->JsonReceive(doc["garage"]);
UDBDebug("jsonreceive 1b");
   // wasser->JsonReceive(doc["wasser"]); 
   // UDBDebug("jsonreceive 1c");
    waage->JsonReceive(doc["waage"]);
    UDBDebug("jsonreceive 2");
    strom->JsonReceive(doc["strom"]);
UDBDebug("jsonreceive 3");
}


void UDBDebug(String message) {
#ifdef UDPDEBUG
  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t* ) message.c_str(), (size_t) message.length());
  udp.endPacket();
#endif  
}

void UDBDebug(const char * message) {
#ifdef UDPDEBUG
  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t*) message, strlen(message));
  udp.endPacket();
#endif  
}


void MQTT_Send(char const * topic, String value) {
    String strtopic = topic;
    if (!mqttclient.publish(topic, value.c_str(), true)) {
       UDBDebug("MQTT error");  
       if (!mqttclient.loop()) {
           if (mqttclient.connect(wifihostname, MQTT_User, MQTT_Pass)) {
              UDBDebug("MQTT reconnect successful"); 
              if (!mqttclient.publish(topic, value.c_str(), true)) {
                  UDBDebug("MQTT error");  
              }    
           }  
       else
          UDBDebug("MQTT reconnect error");  
       };
    };
}

void MQTT_Send(char const * topic, float value) {
    char buffer[10];
    snprintf(buffer, 10, "%f", value);
    MQTT_Send(topic, buffer);
}

void MQTT_Send(char const * topic, int16_t value) {
    char buffer[10];
    snprintf(buffer, 10, "%d", value);
    MQTT_Send(topic, buffer);
}

void MQTT_Send(char const * topic, long value) {
    char buffer[10];
    snprintf(buffer, 10, "%d", value);
    MQTT_Send(topic, buffer);
}

  // Callback function
void MQTT_callback(char* topic, byte* payload, unsigned int length) {

    String message = String(topic);
    int8_t joblength = message.length()+1;// 0 char
    payload[length] = '\0';
    String value = String((char *) payload);

    if (!startuprestore) {
      if (message == "HomeServer/Server/Log") {
        UDBDebug("Startmessage");
            startuprestore = true;
            jsonreceive(value);
            //const char *TOPIC = "HomeServer/Server/Log";
            //mqttclient.unsubscribe(TOPIC);
            UDBDebug("Startmessage ende");
            return;
        }  
    }
    else {
     // UDBDebug("### "+message +" - "+value);
    for (int16_t i=0; i<sensorcounter;i++) {  
        if (sensor[i]->HandleMQTT(message, joblength, value))
          return;
      }
    }  

}

// EMAIL

void EMail_Send(String textmessage) {
/* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = email_user;
  session.login.password = email_pass;
  session.login.user_domain = email_domain;

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = email_user;
  message.subject = F("HomeServer Alert");
  message.addRecipient(email_user, email_user);

  message.text.content = textmessage.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    UDBDebug("Error sending Email, " + smtp.errorReason());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
 // UDBDebug(String(status.info()));

  /* Print the sending result */
  if (status.success())
  {
    //UDBDebug("Message sent success: "+String(status.completedCount()));
    //UDBDebug("Message sent failed: "+String(status.failedCount()));
    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}


void HTTP_Send(String url) {

  HTTPClient http;

  UDBDebug(url);
  
  // Your Domain name with URL path or IP address with path
  http.begin(url.c_str());
  http.setAuthorization(red_user, red_pass);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    UDBDebug("HTTP Response code: "+String(httpResponseCode));
    Serial.println(httpResponseCode);
    String payload = http.getString();
    UDBDebug(payload);
  }
  else {
    UDBDebug("HTTP Error code "+String(httpResponseCode));
  }
  // Free resources
  http.end();
}
