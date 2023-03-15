#include "main.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "waage.h"
#include "MHZSensor.h"

WiFiManager wifiManager;
#define NTP_SERVER "de.pool.ntp.org"
#define DefaultTimeZone "CET-1CEST,M3.5.0/02,M10.5.0/03"  
String MY_TZ = DefaultTimeZone ;
const char* wifihostname = "ESPHomeServer";

const char* host = "http://192.168.0.34";
const int httpPort = 80;

struct tm timeinfo;
char SDLog_Lasthour = -1;
char SDLog_Lastmin = -1;
char time_last_restart_day = -1;

File sdcard;

#define RXD2 27
#define TXD2 26  // MHZ Sensor

#define Touchsensor 12

WebServer server(80);
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

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Config mode");
  String ipaddress = WiFi.softAPIP().toString();
  Serial.println(ipaddress);
}

void setup() {
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
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


  waage = new Waage();
  storage[storagecounter++] = waage;
  mhzsensor = new MHZSensor(&Serial2);
  storage[storagecounter++] = mhzsensor;

  server.on("/4DAction/Strom", handleStrom);
  server.on("/sendfile", handleFile);
  server.begin();

  Serial.println("all systems go...");
 
}

void loop() { 
  ArduinoOTA.handle();
  server.handleClient();

  int32_t zeit = millis();
  bool needReport = false; 

  for (int16_t i=0; i<storagecounter;i++) {  
    storage[i]->Run(zeit);
    needReport |= storage[i]->needReport(zeit);
  }
  
  for (int16_t i=0; i<storagecounter;i++) {
    if (needReport) {
      storage[i]->doReport();
    }  
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


void handleFile() {
  if (server.hasArg("File"))  {
    Serial.println(server.arg("File"));
    String job = "/"+server.arg("File");
    Serial.println("Try to send: "+job);
    if (SD.exists(job)) {
      sdcard = SD.open(job, FILE_READ);
      size_t sent = server.streamFile(sdcard, "text/html");
      sdcard.close();
      Serial.println("file send...");
    }
    else
      server.send(200, "text/html", "File not found");
  }  
  else  
    server.send(200, "text/html", "File name not given");
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
  data = "";
  for (int16_t i=0; i<storagecounter; i++) {
    data = data + storage[i]->WriteData();
  }  
  sdcard.println(data);
  sdcard.close();
}