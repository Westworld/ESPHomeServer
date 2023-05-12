#include "Arduino.h"
#include "Strom.h"

extern struct tm timeinfo;
extern ESP32WebServer server;


bool Strom::HandleMQTT(String message, short joblength, String value) {
  const char IsKauf[] =            "HomeServer/Strom/Kauf";
  const char IsVerkauf[] =         "HomeServer/Strom/Verkauf";
  const char IsBatVerbrauch[] =    "HomeServer/Batterie/Consumption_W";
  const char IsEinzelVerbrauch[] = "HomeServer/Einzel/Gesamt";
  const char IsProduktion[] =      "HomeServer/Strom/Produktion";
  const char IsProduktionDaily[] = "HomeServer/Strom/ProduktionDaily";

  const char IsWallboxCar[] =      "HomeServer/Wallbox/car";
  const char IsWallboxAmp[] =      "HomeServer/Wallbox/amp";
  const char IsWallboxAlw[] =      "HomeServer/Wallbox/alw";
  const char IsWallboxEto[] =      "HomeServer/Wallbox/eto";
  const char IsWallboxPsm[] =      "HomeServer/Wallbox/psm";
  const char IsWallboxNrg[] =      "HomeServer/Wallbox/nrg";

  long curmillis = millis();

  switch (joblength) {
    case sizeof(IsKauf): 
        if (message == IsKauf) {
            float curValue = value.toFloat(); 
            if (StromKauf != 0) {
                curStromKauf = (curValue - StromKauf);
                StromKauf = curValue;
                TagStromKauf = (curValue - TagStromKaufStart);
                MQTT_Send((char const *) "HomeServer/Strom/curKauf", curStromKauf); 
                MQTT_Send((char const *) "HomeServer/Strom/TagKauf", TagStromKauf); 
            }
            else {
                StromKauf = curValue;
                TagStromKaufStart = curValue;
            }
            lastUpdated = curmillis;
            return true;
        }
        break;

    case sizeof(IsVerkauf):   // and IsEinzelVerbrauch
        if (message == IsVerkauf) {
            float curValue = value.toFloat(); 
            if (StromVerkauf != 0) {
                curStromVerkauf = (curValue - StromVerkauf);
                StromVerkauf = curValue;
                TagStromVerkauf = (curValue - TagStromVerkaufStart);
                MQTT_Send((char const *) "HomeServer/Strom/curVerkauf", curStromVerkauf); 
                MQTT_Send((char const *) "HomeServer/Strom/TagVerkauf", TagStromVerkauf); 
            }
            else {
                StromVerkauf = curValue;
                TagStromVerkaufStart = curValue;
            }
            lastUpdated = curmillis;
            return true;
        }

        if (message == IsEinzelVerbrauch) {
            Einzelverbrauch = round2(value.toFloat()*0.92);
            float zeit = (curmillis - LastEinzelVerbrauch);
            zeit = zeit / 60000.0;  // Zeit in ms, jetzt minuten
            float dist = (Einzelverbrauch / 60000.0 * zeit);
            TagEinzelVerbrauch += dist;
            //UDBDebug("Einzelverbrauch dist: "+String(dist)+" zeit: "+String(zeit)+" last: "+String(LastEinzelVerbrauch)+" cur: "+String(curmillis));
            MQTT_Send((char const *) "HomeServer/Strom/TagEinzelVerbrauch", TagEinzelVerbrauch); 
            if (WallboxNrg > 0) {
                // Auto lädt, nehme Einzelwerte
                GesamtVerbrauch = Einzelverbrauch;
                TagGesamtVerbrauch += dist;
                MQTT_Send((char const *) "HomeServer/Strom/GesamtVerbrauch", GesamtVerbrauch); 
                MQTT_Send((char const *) "HomeServer/Strom/TagGesamtVerbrauch", TagGesamtVerbrauch); 
            }
            LastEinzelVerbrauch = curmillis;
            return true;
        }
        break;

    case sizeof(IsProduktion):   // and IsEinzelVerbrauch
        if (message == IsProduktion) {
            float Prod = value.toFloat();
            float zeit = (curmillis - LastProduktion);
            zeit  = zeit / 60000.0;  // Zeit in ms, jetzt minuten
            float dist = (Prod / 60000.0 * zeit);  // ein 60stel, in kw
            LastProduktion = curmillis;
            TagProduktion += dist;
            MQTT_Send((char const *) "HomeServer/Strom/TagProduktion", (float)(TagProduktion)); 

            if (ProduktionCounter>14) {
                for (short i=0;i<14;i++) {
                    ProduktionAvg[i] = ProduktionAvg[i+1];
                }
                ProduktionCounter--;
            }
            ProduktionAvg[ProduktionCounter++] = Prod;
            Prod = 0;
            for (short i=0;i<ProduktionCounter;i++) {
                Prod += ProduktionAvg[i];
            }            
            Prod /= ProduktionCounter;
            MQTT_Send((char const *) "HomeServer/Strom/ProduktionAVG", (long)(Prod)); 

            return true;
        }
        break;

    case sizeof(IsBatVerbrauch):   // and IsEinzelVerbrauch
        if (message == IsBatVerbrauch) {
            BatterieVerbrauch = value.toFloat();
            float zeit = (curmillis - LastBatVerbrauch) / 60000;  // Zeit in ms, jetzt minuten
            float dist = (BatterieVerbrauch / 60000 * zeit); // ein 60stel, in kw
            if (WallboxNrg < 10) {
                // Auto lädt nicht, nehme Batterie
                GesamtVerbrauch = BatterieVerbrauch;
                TagGesamtVerbrauch += dist;
                MQTT_Send((char const *) "HomeServer/Strom/GesamtVerbrauch", GesamtVerbrauch); 
                MQTT_Send((char const *) "HomeServer/Strom/TagGesamtVerbrauch", TagGesamtVerbrauch); 
            }
            LastBatVerbrauch = curmillis;
            return true;
        }
        break;


    case sizeof(IsWallboxAlw):   // and andereWaööbpx
        if (message == IsWallboxCar) {
            WallboxCar = value.toInt();
            return true;
        }
        if (message == IsWallboxAmp) {
            WallboxAmp = value.toInt();
            return true;
        }
        if (message == IsWallboxAlw) {
            if (value == "false")
                WallboxAlw = false;
            else   
                WallboxAlw = true;
            return true;
        }
        if (message == IsWallboxEto) {
            WallboxEto = value.toInt();
            if (WallboxEtoStart == 0) WallboxEtoStart = WallboxEto;
            return true;
        }
        if (message == IsWallboxPsm) {
            WallboxPsm = value.toInt();
            return true;
        }
        if (message == IsWallboxNrg) {
            WallboxNrg = value.toInt();
            return true;
        }                        
        break;

    case sizeof(IsProduktionDaily): 
        if (message == IsProduktionDaily) {
            TagProduktion = value.toFloat(); 
            MQTT_Send((char const *) "HomeServer/Strom/TagProduktion", TagProduktion); 
            return true;
        }
        break;

    default:
        break;
  }
  return false;
}

void Strom::runStunde() {
    // nachdem Hour log geschrieben
 
    MQTT_Send((char const *) "HomeServer/Wallbox/EtoTag", (long)(WallboxEto-WallboxEtoStart)); 
   //   UDBDebug("hourly report_strom ende");
}


String Strom::serialize() {
    String result="";

    result = "Strom StromKauf = ";
    result += StromKauf;
    result += ", StromVerkauf = ";
    result += StromVerkauf;    
    result += ", TagStromKauf = ";
    result += TagStromKauf; 
    result += ", TagStromVerkauf = ";
    result += TagStromVerkauf; 
    result += ", Einzelverbrauch = ";
    result += Einzelverbrauch; 
    result += ", BatterieVerbrauch = ";
    result += BatterieVerbrauch; 

    result += "\n TagProduktion = ";
    result += TagProduktion;
    result += ", GesamtVerbrauch = ";
    result += GesamtVerbrauch;
    result += ", TagGesamtVerbrauch = ";
    result += TagGesamtVerbrauch;
    result += ", TagEinzelverbrauch = ";
    result += TagEinzelVerbrauch;
    result += "\nWallboxCar = ";
    result += WallboxCar;
    result += ", WallboxAmp = ";
    result += WallboxAmp;
    result += ", WallboxAlw = ";
    result += WallboxAlw;
    result += ", WallboxEto = ";
    result += WallboxEto;
    result += ", WallboxPsm = ";
    result += WallboxPsm;
    result += ", WallboxNrg = ";
    result += WallboxNrg;
    result += ", WallboxEtoStart = ";
    result += WallboxEtoStart;
    result += "\nLastUpdate = ";
    result += lastUpdated;
    result += "\n";
    return result;
}

void Strom::StatusToJson(JsonObject json){
    // data, daily
    json["StromKauf"] = round2(StromKauf);
    json["StromVerkauf"] = round2(StromVerkauf);
    json["TagStromKauf"] = round2(TagStromKauf);
    json["TagStromVerkauf"] = round2(TagStromVerkauf);    
    json["TagGesamtVerbrauch"] = round2(TagGesamtVerbrauch);
    json["TagEinzelVerbrauch"] = round2(TagEinzelVerbrauch); 
    json["TagProduktion"] = round2(TagProduktion); 
    json["TagWallBoxEto"] = WallboxEto-WallboxEtoStart;
}


void Strom::ToJson(JsonObject json){
    // log, hourly
    json["StromKauf"] = round2(StromKauf);
    json["StromVerkauf"] = round2(StromVerkauf);
    json["TagStromKaufStart"] = round2(TagStromKaufStart);
    json["TagStromVerkaufStart"] = round2(TagStromVerkaufStart);
    json["EinzelVerbrauch"] = int(Einzelverbrauch);
    json["GesamtVerbrauch"] = round2(GesamtVerbrauch);
    json["BatterieVerbrauch"] = round2(BatterieVerbrauch);     
    json["TagGesamtVerbrauch"] = round2(TagGesamtVerbrauch); 
    json["TagEinzelVerbrauch"] = round2(TagEinzelVerbrauch);
    json["TagProduktion"] = round2(TagProduktion);   
    json["TagWallBoxEto"] = WallboxEto-WallboxEtoStart;        
    }

void Strom::JsonReceive(JsonObject strom) {
    if(StromKauf == 0)
        StromKauf = strom["StromKauf"]; // 12752.22
    if (StromVerkauf == 0)
        StromVerkauf = strom["StromVerkauf"]; // 24101.43
    if (TagStromKaufStart == 0)
        TagStromKaufStart = strom["TagStromKaufStart"]; // 19.53
    if (TagStromVerkaufStart == 0)
        TagStromVerkaufStart = strom["TagStromVerkaufStart"]; // 60.55
    if (Einzelverbrauch == 0)
        Einzelverbrauch = strom["Einzelverbrauch"]; // 411
    if (BatterieVerbrauch == 0)
        BatterieVerbrauch = strom["BatterieVerbrauch"]; // 328
    if (TagGesamtVerbrauch == 0)
        TagGesamtVerbrauch = strom["TagGesamtVerbrauch"]; // 1544.32
    if (TagProduktion == 0)
        TagProduktion = strom["TagProduktion"]; // 1544.32
    if (GesamtVerbrauch == 0)
        GesamtVerbrauch = strom["GesamtVerbrauch"]; // 1416.45
    if (TagEinzelVerbrauch == 0)
        TagEinzelVerbrauch = strom["TagEinzelVerbrauch"]; // 673.44   
    if (WallboxEtoStart == 0)
        WallboxEtoStart = strom["WallboxEtoStart"];
}

void Strom::runDay() {
  //UDBDebug("daily report_strom start");
  TagStromKaufStart=StromKauf;
  TagStromVerkaufStart=StromVerkauf;
  TagStromKauf=0;
  TagStromVerkauf=0;
  TagProduktion=0;
  TagEinzelVerbrauch=0;
  TagGesamtVerbrauch=0;
  WallboxEtoStart = WallboxEto;
  MQTT_Send((char const *) "HomeServer/Strom/TagVerkauf", TagStromVerkauf); 
  MQTT_Send((char const *) "HomeServer/Strom/TagKauf", TagStromKauf); 
  MQTT_Send((char const *) "HomeServer/Strom/TagProduktion", TagProduktion); 
  MQTT_Send((char const *) "HomeServer/Strom/TagGesamtVerbrauch", TagGesamtVerbrauch); 
  MQTT_Send((char const *) "HomeServer/Strom/TagEinzelVerbrauch", TagEinzelVerbrauch); 
  MQTT_Send((char const *) "HomeServer/Wallbox/EtoTag", 0L); 
  //UDBDebug("daily report_strom ende");
}
