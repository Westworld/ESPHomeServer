#include "main.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <PubSubClient.h>

#include <iostream>
#include <fstream>

#include "waage.h"
#include "MHZSensor.h"

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


#define useMQTT
const char* mqtt_server = "192.168.0.57";
// MQTT_User and MQTT_Pass defined via platform.ini, external file, not uploaded to github
PubSubClient mqttclient(wifiClient);

struct tm timeinfo;
char SDLog_Lasthour = -1;
char SDLog_Lastmin = -1;
char time_last_restart_day = -1;

File sdcard;

#define RXD2 27
#define TXD2 26  // MHZ Sensor

#define Touchsensor 12

ESP32WebServer server(80);
Storage * storage[10];
int16_t storagecounter=0;
Waage * waage;
MHZSensor * mhzsensor;

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

#ifdef useMQTT
 // MQTT
 Serial.printf("vor MQTT");
    mqttclient.setServer(mqtt_server, 1883);
   Serial.printf("nach MQTT");
   if (mqttclient.connect(wifihostname, MQTT_User, MQTT_Pass)) {
      //mqttclient.publish("outTopic","hello world");
      UDBDebug("MQTT connect successful"); 
   }  
    else
       UDBDebug("MQTT connect error");  

   Serial.printf("nach MQTT");   
#endif

 // DEVICES
  waage = new Waage();
  storage[storagecounter++] = waage;
  mhzsensor = new MHZSensor(&Serial2);
  storage[storagecounter++] = mhzsensor;

// Web request handler
  server.on("/4DAction/Strom", handleStrom);
  server.on("/sendfile", handleFile);
  server.on("/debug", webdebug);
  server.begin();

  Serial.println("all systems go...");
  UDBDebug("all systems go...");
}


void loop() { 
  if (WiFi.status() != WL_CONNECTED)
    WifiConnect();
  ArduinoOTA.handle();
  server.handleClient();
  #ifdef useMQTT
  mqttclient.loop();
  #endif

  int32_t zeit = millis();
  for (int16_t i=0; i<storagecounter;i++) {  
    storage[i]->Run(zeit);
  }

  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }

//  if ((timeinfo.tm_hour != SDLog_Lasthour) | ((timeinfo.tm_min != SDLog_Lastmin) & ((timeinfo.tm_min%5)==1))) {
  if (timeinfo.tm_hour != SDLog_Lasthour)  {
    SDLog_Lasthour = timeinfo.tm_hour;
    SDLog_Lastmin = timeinfo.tm_min;
    logSDFile();
  }


  if ((time_last_restart_day == 6) && (timeinfo.tm_wday == 0))
  {
    // time for restart !! one restart every week
    ESP.restart();
  }
  else time_last_restart_day = timeinfo.tm_wday;

  
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

      server.send(200, "text/html", prepareHtmlPage());
      return;
    }  
  }

  if (server.hasArg("Gewicht"))  {
    Serial.println(server.arg("Gewicht"));
    }    

  server.send(200, "text/html", prepareHtmlPage());
}

void webdebug() {
  String message= "";
  for (int16_t i=0; i<storagecounter; i++) {
    message = message + storage[i]->serialize() + "\n";
  }

  server.send(200, "text/plain", message);
}

void handleFile() {
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
          WebSendDirList();
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
  sprintf(name, "/Data_%04d_%02d.txt", 30, timeinfo.tm_year+1900, timeinfo.tm_mon);
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

void logSDFile(void) {
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

  char buffer[500];
  snprintf(buffer, 500, "#;%04d-%02d-%02d_%02d-%02d", timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
  data = buffer;

  for (int16_t i=0; i<storagecounter; i++) {
    data = data + storage[i]->WriteData();
  }  
  sdcard.println(data);
  sdcard.close();
}

void WebSendDirList()
{  
  String dirchar = "/";
  String content = "";
  File root;

  String answer =  String("<!DOCTYPE HTML><html><body>");

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
      answer = answer + String("<a href=\"/sendfile?File=")+filename +String("\">")+filename + String("</a><br>\r\n");
      entry.close();
    }
  }
  answer =  answer + String("</body></html>\r\n");
  server.send(200, "text/html", answer);
}

String ReadLastLine()
{
    String name = "/Data_0030_2023.txt";
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

void UDBDebug(String message) {
#ifdef UDPDEBUG
  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t* ) message.c_str(), (size_t) message.length());
  udp.endPacket();
#endif  
}

void MQTT_Send(char const * topic, float value) {
    #ifdef useMQTT
    UDBDebug("MQTT " +String(topic)+" "+String(value));  
    char buffer[10];
    snprintf(buffer, 10, "%f", value);
    if (!mqttclient.publish(topic, buffer)) {
       UDBDebug("MQTT error");  
    };
    #endif
}

void MQTT_Send(char const * topic, int16_t value) {
    #ifdef useMQTT
    UDBDebug("MQTT " +String(topic)+" "+String(value));  
    char buffer[10];
    snprintf(buffer, 10, "%d", value);
    if (!mqttclient.publish(topic, buffer)) {
       UDBDebug("MQTT error");  
    };
    #endif
}