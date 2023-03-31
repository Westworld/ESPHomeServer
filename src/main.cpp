#include "main.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <PubSubClient.h>


#include <iostream>
#include <fstream>

#include "waage.h"
#include "MHZSensor.h"
#include "wasser.h"
#include "garage.h"

extern void Homematic_Set(String device, int8_t status);

#define UDPDEBUG 1
#ifdef UDPDEBUG
WiFiUDP udp;
const char * udpAddress = "192.168.0.34";
const int udpPort = 19814;
#endif

WiFiClient wifiClient;

#define NTP_SERVER "de.pool.ntp.org"
#define DefaultTimeZone "CET-1CEST,M3.5.0/02,M10.5.0/03"  
String MY_TZ = DefaultTimeZone ;
const char* wifihostname = "ESPHomeServer";

const char* host = "http://192.168.0.34";
const int httpPort = 80;


const char* mqtt_server = "192.168.0.57";
// MQTT_User and MQTT_Pass defined via platform.ini, external file, not uploaded to github
PubSubClient mqttclient(wifiClient);

struct tm timeinfo;
char SDLog_Lasthour = -1;
char SDLog_Lastmin = -1;
char time_last_restart_day = -1;
char SDLog_Lastday = -1;

File sdcard;

#define RXD2 27
#define TXD2 26  // MHZ Sensor

#define Touchsensor 12

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The SMTP Session object used for Email sending */
SMTPSession smtp;
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

ESP32WebServer server(80);
Storage * storage[10];
int16_t storagecounter=0;
Waage * waage;
MHZSensor * mhzsensor;
Wasser * wasser;
Garage * garage;

void setTimeZone(String TimeZone) {
  struct tm local;
  configTzTime(TimeZone.c_str(), NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
  getLocalTime(&local, 10000);      // Versuche 10 s zu Synchronisieren
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

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }
    IPAddress ip = WiFi.localIP();
    Serial.println(F("WiFi connected"));
    Serial.println(ip);
}

void setup() {
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.begin(115200);

  WifiConnect() ;

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

  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
  }


  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);


 // MQTT
 Serial.printf("vor MQTT");
    mqttclient.setServer(mqtt_server, 1883);
    mqttclient.setCallback(MQTT_callback);
   Serial.printf("nach MQTT");
   if (mqttclient.connect(wifihostname, MQTT_User, MQTT_Pass)) {
      //mqttclient.publish("outTopic","hello world");
      UDBDebug("MQTT connect successful"); 
      //mqttclient.subscribe("garage/TargetDoorState");
      //const char *TOPIC = "Haus/Sonoff POW R1/R2/Dreamer_Power/#";
      const char *TOPIC = "garage/TargetDoorState/#";
      mqttclient.subscribe(TOPIC);
      //mqttclient.subscribe("Haus/+/Power");
   }  
    else
       UDBDebug("MQTT connect error");  

   Serial.printf("nach MQTT");   

  while (!getLocalTime(&timeinfo)) {
    UDBDebug("error getLocalTime"); 
    delay(2000);
  }

  SDLog_Lastday = timeinfo.tm_mday;

 // DEVICES
  waage = new Waage();
  storage[storagecounter++] = waage;
  mhzsensor = new MHZSensor(&Serial2);
  storage[storagecounter++] = mhzsensor;
  wasser = new Wasser();
  storage[storagecounter++] = wasser;
  garage = new Garage();
  storage[storagecounter++] = garage;

// Web request handler
  server.on("/4DAction/Strom", handleStrom);
  server.on("/sendfile", handleFile);
  server.on("/debug", webdebug);
  server.begin();

  // Email
  smtp.callback(smtpCallback);

  ReadAndParseLastLine();

  logSDFile(true);

  Serial.println("all systems go...");
  UDBDebug("all systems go...");

  //EMail_Send("all systems go...");
}


void loop() { 
  if (WiFi.status() != WL_CONNECTED)
    WifiConnect();
  ArduinoOTA.handle();
  server.handleClient();
  if (!mqttclient.loop()) {
    if (mqttclient.connect(wifihostname, MQTT_User, MQTT_Pass)) {
      UDBDebug("MQTT reconnect successful"); 
   }  
    else
       UDBDebug("MQTT reconnect error");  
  };

  int32_t zeit = millis();
  for (int16_t i=0; i<storagecounter;i++) {  
    storage[i]->Run(zeit);
  }

  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  else
  {
    //  if ((timeinfo.tm_hour != SDLog_Lasthour) | ((timeinfo.tm_min != SDLog_Lastmin) & ((timeinfo.tm_min%5)==1))) {
      if (timeinfo.tm_hour != SDLog_Lasthour)  {
        SDLog_Lasthour = timeinfo.tm_hour;
        SDLog_Lastmin = timeinfo.tm_min;
        logSDFile(false);
      }

      if (timeinfo.tm_mday != SDLog_Lastday)  {
        SDLog_Lastday = timeinfo.tm_mday;
        logDaySDFile();
      }      


      if ((time_last_restart_day == 6) && (timeinfo.tm_wday == 0))
      {
        // time for restart !! one restart every week
        ESP.restart();
      }
      else time_last_restart_day = timeinfo.tm_wday;

  }  // time success received
}

// size_t sent = server.streamFile(file, contentType);

void handleStrom() {

  if (server.hasArg("Job"))  {
    Serial.println(server.arg("Job"));
    String job = server.arg("Job");
    if ((job == "Timmi") || (job.startsWith("Scale")) ) {
       if (!server.hasArg("Gewicht")) 
          return;
      String SGewicht = server.arg("Gewicht"); 
      float Gewicht = SGewicht.toFloat();
      if (job == "Timmi")
        waage->NewScale(Waage::warTimmi, Gewicht);
      else {
        String Katze = job.substring(5);
        Waage::wer katze = Waage::unbekannt;
        if (Katze == "Buddy") katze = Waage::warBuddy;
        if (Katze == "Mika") katze = Waage::warMika;
        if (Katze == "Matti") katze = Waage::warMatti;
        if (katze != Waage::unbekannt)
          waage->NewScale(katze, Gewicht);
        }

      server.send(200, "text/html", String("OK waage"));
      return;
    }  

    if (job == "Wasser") {
        long counter = 0;
        float temp = 127;
        if (server.hasArg("Counter")) {
          String SCounter = server.arg("Counter"); 
          counter = SCounter.toInt();
        }
        if (server.hasArg("Temp")) {
          String STemp = server.arg("Temp"); 
          temp = STemp.toFloat();
        }   
        wasser->NewReport(counter, temp);  
        server.send(200, "text/html", String("OK wasser"));
        return;   
    }

    if (job == "Garage") {
        long counter = 0;
        float temp = 127;
        String Button="";
        if (server.hasArg("Strom")) {
          String SCounter = server.arg("Strom"); 
          counter = SCounter.toInt();
        }
        if (server.hasArg("Temp2")) {
          String STemp = server.arg("Temp2"); 
          temp = STemp.toFloat();
        }   
        if (server.hasArg("Button")) {
          Button = server.arg("Button"); 
        } 
        garage->NewReport(counter, temp, Button);   
        server.send(200, "text/html", String("OK Garage"));  
        return;
    }

   if (job == "Licht") {
        String was="";
        int welchesLicht=0;
        if (server.hasArg("Was")) {
          was = server.arg("Was"); 
          welchesLicht = was.toInt();
          switch (welchesLicht) {
            case 1:
              Homematic_Set("Dach", 2);
              break;
            case 2:
              Homematic_Set("Bad", 2);
              break;
            case 3:
              //Homematic_Set("Dach", 2);
              break;
            case 4:
              Homematic_Set("Lichtwarner_SZ", 0);
              break;
            default:
              ;  // nothing  
          }
        }        
        server.send(200, "text/html", String("OK Licht"));  
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
        if ((name == "Heizung") && (STemp != "127")) {
          MQTT_Send("HomeServer/Heizung/Temp",STemp);
          // Send MQTT ohne lokale Speicherung    
          server.send(200, "text/html", String("OK temp"));  
          return;
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
  for (int16_t i=0; i<storagecounter; i++) {
    message = message + storage[i]->serialize() + "\n";
  }

  server.send(200, "text/plain", message);
}

void handleFile() {
  // http://esphomeserver.fritz.box/sendfile?File=Data_2023_02.txt
  //http://esphomeserver.fritz.box/sendfile?Folder
  if (server.hasArg("File"))  {
    Serial.println(server.arg("File"));
    String job = "/"+server.arg("File");
    Serial.println("Try to send: "+job);
    if (SD.exists(job)) {
      sdcard = SD.open(job, FILE_READ);
      size_t sent = server.streamFile(sdcard, "text/plain");
      sdcard.close();
      Serial.println("file send...");
    }
    else
      server.send(200, "text/html", "File not found");
  }  
  else   {
    if (server.hasArg("Folder"))  {
        WebSendDirList(true);
    }    
    else
    {  // irgendeinen anderen Namen
        WebSendDirList(false);
    }
  }
    
}

String prepareHtmlPage() {
  String htmlPage = String("OK");
  /*
    String("HTTP/1.1 200 OK\r\n") +
    "Content-Type: text/html\r\n" +
    "Connection: close\r\n" +  // the connection will be closed after completion of the response
    "\r\n" +
    "<!DOCTYPE HTML>" +
    "<html>" +
    "Done"
    "</html>" +
    "\r\n";
    */
  return htmlPage;
}

// SD Card
// once per hour store last values.  
// store when pin is pressed, to allow power remove (needed with battery?)
// File name like Data_2022_12.txt
// if file not exists create with titles in first line
// %;year-month-day_hour-min;Scale_Buddy;Scale_Buddy_Tag ... MHZ_CO2;MHZ_temp;MHZ_Accuracy
// on boot, load last values (needed with battery?)

void SDsetFileName(char * name) {
  snprintf(name, 30, "/Data_%04d_%02d.txt", timeinfo.tm_year+1900, timeinfo.tm_mon);
}

String SDwriteHeader() {
  String data;
  char buffer[100];
  snprintf(buffer, 100, "#;%04d-%02d-%02d_%02d-%02d", timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
  data = buffer;
  for (int16_t i=0; i<storagecounter; i++) {
    data = data + storage[i]->WriteHeader();
  }
  return data;
}

String SDDaywriteHeader() {
  String data;
  char buffer[100];
  snprintf(buffer, 100, "+;%04d", timeinfo.tm_year+1900);
  data = buffer;
  for (int16_t i=0; i<storagecounter; i++) {
    data = data + storage[i]->WriteDayHeader();
  }
  return data;
}

void logSDFile(bool headeronly) {
  char name[31];
  String data;

  SDsetFileName(name);

  if (SD.exists(name)) {
    sdcard = SD.open(name, FILE_APPEND);
  }
  else{
    sdcard = SD.open(name, FILE_WRITE);
    data = SDwriteHeader();
    sdcard.println(data);  
  }

  if (headeronly) {
    data = SDwriteHeader();
    sdcard.println(data);  
  }  
  else 
   {
    char buffer[500];
    snprintf(buffer, 500, "#;%04d-%02d-%02d_%02d-%02d", timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
    data = buffer;

    for (int16_t i=0; i<storagecounter; i++) {
      data = data + storage[i]->WriteData();
    }  
    data += ';';
    sdcard.println(data);
  }  
  sdcard.close();
}

void logDaySDFile(void) {
  char name[31];
  String data;

  snprintf(name, 30, "/Year_%04d.txt", timeinfo.tm_year+1900);

  if (SD.exists(name)) {
    sdcard = SD.open(name, FILE_APPEND);
  }
  else{
    sdcard = SD.open(name, FILE_WRITE);
    data = SDDaywriteHeader();
    sdcard.println(data);  
  }

  char buffer[100];
  snprintf(buffer, 100, "#;%04d-%02d-%02d", timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday);
  data = buffer;

  for (int16_t i=0; i<storagecounter; i++) {
    data = data + storage[i]->WriteDayData();
  }  
  data += ';';
  sdcard.println(data);
  sdcard.close();
}

void WebSendDirList(bool textonly)
{  
  String dirchar = "/";
  String content = "";
  File root;

  String answer;
  if (textonly)
   answer = "";
  else
   answer =  String("<!DOCTYPE HTML><html><body>");

  root = SD.open("/");
  while (true)
  {
    File entry =  root.openNextFile();
    short filetype=0;
    if (! entry)
    {
      // no more files
      break;
    }
    
    if (entry.isDirectory()) 
    {
      // Skip file if in subfolder
       entry.close(); // Close folder entry
    } 
    else
    {
      //Serial.println(entry.name());
      String dirname = root.name();
      String filename = entry.name();
      if (filename.startsWith(".")) continue;
      //filename = dirchar + filename;
      if (textonly)
        answer = answer + filename + String('\t')+String(entry.size())+String('\n');
      else
        answer = answer + String("<a href=\"/sendfile?File=")+filename+String("\">")+filename + String(" - ")+String(entry.size())+String("</a><br>\r\n");
      entry.close();
    }
  }
  if (!textonly)
    answer =  answer + String("</body></html>\r\n");
  server.send(200, "text/html", answer);
}

String ReadLastLine()
{
    char name [30];
    SDsetFileName(name);
    UDBDebug(name);
    if (!SD.exists(name)) {
      UDBDebug("File does not exists");
      return "";
    }
    else {
      sdcard = SD.open(name, FILE_READ);
      size_t filesize = sdcard.size();
      Serial.println(filesize);
      sdcard.seek(filesize-2);

          bool keepLooping = true;
          while(keepLooping) {
              char ch;
              ch = sdcard.read();  // Get current byte's data
              Serial.println(ch);

              if(ch == -1) {             // If the data was at or before the 0th byte
                  sdcard.seek(0);                       // The first line is the last line
                  keepLooping = false;                // So stop there
              }
              else if(ch == '\n') {                   // If the data was a newline
                  keepLooping = false;                // Stop at the current position.
              }
              else {                                  // If the data was neither a newline nor at the 0 byte
                  sdcard.seek(sdcard.position()-2);        // Move to the front of that data, then to the front of the data before it
                  Serial.print("new: ");Serial.println(sdcard.position());
              }
          }

          String lastLine;            
          lastLine = sdcard.readStringUntil('\n');                      // Read the current line

          sdcard.close();
          return(lastLine);
    }    
}

void ReadAndParseLastLine() {
  UDBDebug("read last values...");
  String lastline = ReadLastLine();
  UDBDebug(lastline);
  if (lastline.charAt(0) == '#') {
    lastline = lastline.substring(2);
    int16_t pos = lastline.indexOf(';');
    if(pos >= 0) {
      lastline = lastline.substring(pos+1);
      for (int16_t i=0; i<storagecounter;i++) {  
        lastline = storage[i]->readLastLine(lastline);  // first ; already removed
        UDBDebug(storage[i]->serialize());
        UDBDebug(lastline);
      }
    }
  }
}

void UDBDebug(String message) {
#ifdef UDPDEBUG
  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t* ) message.c_str(), (size_t) message.length());
  udp.endPacket();
#endif  
}

void MQTT_Send(char const * topic, String value) {
    //UDBDebug("MQTT " +String(topic)+" "+value);  
    if (!mqttclient.publish(topic, value.c_str(), true)) {
       UDBDebug("MQTT error");  
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
  payload[length] = '\0';
  String value = String((char *) payload);
  UDBDebug(message +" - "+value);
  if (strcmp(topic,"garage/TargetDoorState/BMW")==0){
    UDBDebug("open BMW");
  }
  if (strcmp(topic,"garage/TargetDoorState/Mini")==0){
    UDBDebug("open Mini");
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
  session.login.user_domain = "kolonialwarenmuseum.de";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = email_user;
  message.subject = "HomeServer Alert";
  message.addRecipient("Thomas Maul", email_user);

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
  UDBDebug(String(status.info()));

  /* Print the sending result */
  if (status.success())
  {
    UDBDebug("Message sent success: "+String(status.completedCount()));
    UDBDebug("Message sent failed: "+String(status.failedCount()));
    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}


void HTTP_Send(String host, int httpPort, String url) {

  WiFiClient client;
  if (!client.connect(host.c_str(), httpPort)) {   
    return;
  }

  UDBDebug("HTTP_Send: "+host+url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 1000) {    
      client.stop();
      return;
    }
  }
}