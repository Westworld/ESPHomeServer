#include "Arduino.h"
#include "Strom.h"

extern struct tm timeinfo;
extern ESP32WebServer server;


bool Strom::HandleMQTT(String message, short joblength, String value) {
  const char IsKauf[] =            "HomeServer/Strom/Kauf";
  const char IsVerkauf[] =         "HomeServer/Strom/Verkauf";
  const char IsBatProd[] =         "HomeServer/Batterie/Production_W";
  const char IsBatVerbrauch[] =    "HomeServer/Batterie/Consumption_W";
  const char IsEinzelVerbrauch[] = "HomeServer/Einzel/Gesamt";
  const char IsProduktion[] =      "HomeServer/Strom/Produktion";

  const char IsWallboxCar[] =      "HomeServer/Wallbox/car";
  const char IsWallboxAmp[] =      "HomeServer/Wallbox/amp";
  const char IsWallboxAlw[] =      "HomeServer/Wallbox/alw";
  const char IsWallboxEto[] =      "HomeServer/Wallbox/eto";
  const char IsWallboxPsm[] =      "HomeServer/Wallbox/psm";
  const char IsWallboxNrg[] =      "HomeServer/Wallbox/nrg";

/*
  if (message.startsWith("HomeServer/Strom/")) {
    UDBDebug("erhalten: "+message+": "+value);
  }
*/

  switch (joblength) {
    case sizeof(IsKauf): 
        if (message == IsKauf) {
            float curValue = value.toFloat(); 
            //UDBDebug("StromKauf: "+String(curValue));
            if (StromKauf != 0) {
                curStromKauf = (curValue - StromKauf)*1000;
                StromKauf = curValue;
                TagStromKauf = (curValue - TagStromKaufStart);
                MQTT_Send((char const *) "HomeServer/Strom/curKauf", curStromKauf); 
                MQTT_Send((char const *) "HomeServer/Strom/TagKauf", TagStromKauf); 
            }
            else {
                StromKauf = curValue;
                TagStromKaufStart = curValue;
            }
            lastUpdated = millis();
            return true;
        }
        break;

    case sizeof(IsVerkauf):   // and IsEinzelVerbrauch
        if (message == IsVerkauf) {
            float curValue = value.toFloat(); 
            if (StromVerkauf != 0) {
                curStromVerkauf = (curValue - StromVerkauf)*1000;
                StromVerkauf = curValue;
                TagStromVerkauf = (curValue - TagStromVerkaufStart);
                MQTT_Send((char const *) "HomeServer/Strom/curVerkauf", curStromVerkauf); 
                MQTT_Send((char const *) "HomeServer/Strom/TagVerkauf", TagStromVerkauf); 
            }
            else {
                StromVerkauf = curValue;
                TagStromVerkaufStart = curValue;
            }
            lastUpdated = millis();
            return true;
        }

        if (message == IsEinzelVerbrauch) {
            Einzelverbrauch = value.toFloat();
            CurStundeEinzelVerbrauch += Einzelverbrauch;
            StundeEinzelVerbrauchCounter++; 
            return true;
        }
        break;

    case sizeof(IsProduktion):   // and IsEinzelVerbrauch
        if (message == IsProduktion) {
            float Prod = value.toFloat();
            CurStundeProduktion += Prod;
            StundeProduktionCounter++;
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
            CurStundeBatVerbrauch += BatterieVerbrauch;
            CurStundeBatCounter++;
            return true;
        }
        break;

    case sizeof(IsBatProd):   // and IsEinzelVerbrauch
        if (message == IsBatProd) {
            BatterieProduktion = value.toFloat();
            CurStundeBatProduktion += BatterieProduktion;
            StundeBatProdCounter++;
            MQTT_Send((char const *) "HomeServer/Strom/TagBatProduktion", (long)(TagBatProduktion)); 
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


    default:
        break;
  }
  return false;
}

void Strom::runStunde() {
    // nachdem Hour log geschrieben
    if (StundeBatProdCounter != 0)
        TagBatProduktion += ((CurStundeBatProduktion/StundeBatProdCounter)/60000);
    if (CurStundeBatCounter != 0) {
        TagBatVerbrauch += ((CurStundeBatVerbrauch/CurStundeBatCounter)/60000);
        MQTT_Send((char const *) "HomeServer/Strom/TagBatVerbrauch", (long)(TagBatVerbrauch)); 
    }
    if (StundeProduktionCounter != 0) {
        TagProduktion += ((CurStundeProduktion/StundeProduktionCounter)/60000);
        MQTT_Send((char const *) "HomeServer/Strom/TagProduktion", (long)(TagProduktion));
    }
    if (StundeEinzelVerbrauchCounter != 0) {
        TagEinzelVerbrauch += ((CurStundeEinzelVerbrauch/StundeEinzelVerbrauchCounter)/60000);
        MQTT_Send((char const *) "HomeServer/Strom/TagEinzelVerbrauch", (long)(TagEinzelVerbrauch)); 
    }   
    MQTT_Send((char const *) "HomeServer/Wallbox/TagEto", (long)(WallboxEto-WallboxEtoStart)); 

    CurStundeEinzelVerbrauch= CurStundeBatProduktion= CurStundeBatVerbrauch = 0;
    StundeEinzelVerbrauchCounter= StundeBatProdCounter= CurStundeBatCounter=0; 
}

/* ##########   Wallbox  */



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
    result += ", BatterieProduktion = ";
    result += BatterieProduktion; 
    result += ", BatterieVerbrauch = ";
    result += BatterieVerbrauch; 

    result += "\n CurStundeEinzelVerbrauch = ";
    result += CurStundeEinzelVerbrauch; 
    result += ", CurStundeBatProduktion = ";
    result += CurStundeBatProduktion; 
    result += ", CurStundeBatVerbrauch = ";        
    result += CurStundeBatVerbrauch; 
    result += ", StundeBatProdCounter = ";        
    result += StundeBatProdCounter; 

    result += "\n TagProduktion = ";
    result += TagProduktion;
    result += ", TagBatProduktion = ";
    result += TagBatProduktion;
    result += ", TagBatVerbrauch = ";
    result += TagBatVerbrauch;
    result += ", TagEinzelverbrauch = ";
    result += TagEinzelVerbrauch;
    result += ", WallboxCar = ";
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
    json["TagBatProduktion"] = round2(TagBatProduktion);
    json["TagBatVerbrauch"] = round2(TagBatVerbrauch);  
    json["TagEinzelVerbrauch"] = round2(TagEinzelVerbrauch); 
    json["TagWallBoxEto"] == WallboxEto-WallboxEtoStart;
}


void Strom::ToJson(JsonObject json){
    // log, hourly
    json["StromKauf"] = round2(StromKauf);
    json["StromVerkauf"] = round2(StromVerkauf);
    json["TagStromKaufStart"] = round2(TagStromKaufStart);
    json["TagStromVerkaufStart"] = round2(TagStromVerkaufStart);
    json["EinzelVerbrauch"] = round2(Einzelverbrauch);
    json["BatterieProduktion"] = round2(BatterieProduktion);
    json["BatterieVerbrauch"] = round2(BatterieVerbrauch);     
    json["TagBatProduktion"] = round2(TagBatProduktion);
    json["TagBatVerbrauch"] = round2(TagBatVerbrauch);  
    json["TagEinzelVerbrauch"] = round2(TagEinzelVerbrauch);   
    json["WallboxEtoStart"] = WallboxEtoStart;     
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
    if (BatterieProduktion == 0)
        BatterieProduktion = strom["BatterieProduktion"]; // 384
    if (BatterieVerbrauch == 0)
        BatterieVerbrauch = strom["BatterieVerbrauch"]; // 328
    if (TagBatProduktion == 0)
        TagBatProduktion = strom["TagBatProduktion"]; // 1544.32
    if (TagBatVerbrauch == 0)
        TagBatVerbrauch = strom["TagBatVerbrauch"]; // 1416.45
    if (TagEinzelVerbrauch == 0)
        TagEinzelVerbrauch = strom["TagEinzelVerbrauch"]; // 673.44   
    if (WallboxEtoStart == 0)
        WallboxEtoStart = strom["WallboxEtoStart"];
}

void Strom::runDay() {
  TagStromKaufStart=StromKauf;
  TagStromVerkaufStart=StromVerkauf;
  TagStromKauf=0;
  TagStromVerkauf=0;
  TagProduktion=0;
  TagEinzelVerbrauch=0;
  TagBatProduktion=0;
  TagBatVerbrauch=0;
  WallboxEtoStart = WallboxEto;
  MQTT_Send((char const *) "HomeServer/Strom/TagVerkauf", TagStromVerkauf); 
  MQTT_Send((char const *) "HomeServer/Strom/TagKauf", TagStromKauf); 
  MQTT_Send((char const *) "HomeServer/Strom/TagProduktion", TagProduktion); 
  MQTT_Send((char const *) "HomeServer/Strom/TagBatProduktion", TagBatProduktion); 
  MQTT_Send((char const *) "HomeServer/Strom/TagEinzelVerbrauch", TagEinzelVerbrauch); 
  MQTT_Send((char const *) "HomeServer/Strom/TagBatVerbrauch", TagBatVerbrauch); 
  MQTT_Send((char const *) "HomeServer/Wallbox/EtoTag", 0L); 
}
