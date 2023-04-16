#include "Arduino.h"
#include "Strom.h"

extern struct tm timeinfo;
extern ESP32WebServer server;


bool Strom::HandleMQTT(String message, short joblength, String value) {
  const char IsKauf[] =          "  HomeServer/Strom/Kauf";
  const char IsVerkauf[] =         "HomeServer/Strom/Verkauf";
  const char IsBatProd[] =         "HomeServer/Batterie/Production_W";
  const char IsBatVerbrauch[] =    "HomeServer/Batterie/Consumption_W";
  const char IsEinzelVerbrauch[] = "HomeServer/Einzel/Gesamt";
  const char IsProduktion[] =      "HomeServer/Strom/Produktion";

  switch (joblength) {
    case sizeof(IsKauf): 
        if (message == IsKauf) {
            float curValue = value.toFloat(); 
            if (StromKauf != 0) {
                curStromKauf = curValue - StromKauf;
                StromKauf = curValue;
                TagStromKauf = curValue - TagStromKaufStart;
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
                curStromVerkauf = curValue - StromVerkauf;
                StromVerkauf = curValue;
                TagStromVerkauf = curValue - TagStromVerkaufStart;
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
        break;
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
            MQTT_Send((char const *) "HomeServer/Strom/ProduktionAVG", Prod); 
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
    CurStundeEinzelVerbrauch= CurStundeBatProduktion= CurStundeBatVerbrauch = 0;
    StundeEinzelVerbrauchCounter= StundeBatProdCounter= CurStundeBatCounter=0; 
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
    result += ", BatterieProduktion = ";
    result += BatterieProduktion; 
    result += ", BatterieVerbrauch = ";
    result += BatterieVerbrauch; 
    result += "\nLastUpdate = ";
    result += lastUpdated;

    return result;
}



String Strom::WriteHeader() {
    return ";StromKauf;StromVerkauf;TagStromKaufStart;TagStromVerkaufStart;TagStromKauf;TagStromVerkauf;\
TagEinzelVerbrauch;TagBatProduktion;TagBatVerbrauch;\
CurStundeEinzelVerbrauch;CurStundeBatVerbrauch;CurStundeBatProduktion;CurStundeProduktion;TagProduktion";
}

String Strom::WriteDayHeader() {
    return ";StromKauf;StromVerkauf;TagStromKaufStart;TagStromVerkaufStart;TagStromKauf;TagStromVerkauf;\
Einzelverbrauch;BatterieProduktion;BatterieVerbrauch;Produktion";
}

String Strom::WriteData() {
  char buffer[100];
  float _CurStundeEinzelVerbrauch =0;
  if (StundeEinzelVerbrauchCounter != 0)
    _CurStundeEinzelVerbrauch = CurStundeEinzelVerbrauch/StundeEinzelVerbrauchCounter;
  float _CurStundeBatVerbrauch=0;
  if (CurStundeBatCounter != 0)
    _CurStundeBatVerbrauch = CurStundeBatVerbrauch/CurStundeBatCounter;
  float _CurStundeBatProduktion=0;
  if (StundeBatProdCounter != 0)
    _CurStundeBatProduktion = CurStundeBatProduktion/StundeBatProdCounter;

  snprintf(buffer, 100, ";%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.0f;%.0f;%0.f;%.0f;%.0f;%0.f", StromKauf, StromVerkauf,\
TagStromKaufStart, TagStromVerkaufStart, TagStromKauf, TagStromVerkauf,\
TagEinzelVerbrauch,TagBatProduktion,TagBatVerbrauch, _CurStundeEinzelVerbrauch, \
    _CurStundeBatVerbrauch, _CurStundeBatProduktion);
  return String(buffer);    
}

String Strom::WriteDayData() {
  char buffer[100];
  snprintf(buffer, 100, ";%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.0f;%.0f;%0.f;%.f", StromKauf, StromVerkauf, \
TagStromKaufStart, TagStromVerkaufStart, TagStromKauf, TagStromVerkauf,TagEinzelVerbrauch,TagBatProduktion,TagBatVerbrauch,TagProduktion);
  TagStromKaufStart=StromKauf;
  TagStromVerkaufStart=StromVerkauf;
  TagStromKauf=0;
  TagStromVerkauf=0;
  TagProduktion=0;
  TagEinzelVerbrauch=0;
  TagBatProduktion=0;
  TagBatVerbrauch=0;
  MQTT_Send((char const *) "HomeServer/Strom/TagVerkauf", TagStromVerkauf); 
  MQTT_Send((char const *) "HomeServer/Strom/TagKauf", TagStromKauf); 
  MQTT_Send((char const *) "HomeServer/Strom/TagProduktion", TagProduktion); 
  MQTT_Send((char const *) "HomeServer/Strom/TagBatProduktion", TagBatProduktion); 
  MQTT_Send((char const *) "HomeServer/Strom/TagEinzelVerbrauch", TagEinzelVerbrauch); 
  MQTT_Send((char const *) "HomeServer/Strom/TagBatVerbrauch", TagBatVerbrauch);
  return String(buffer);    
}

String Strom::readLastLine(String &lastline) {
    // first ; already removed
    int16_t index = 0;
    float result = 0;
    if (!get_token_Stored_Data(lastline, StromKauf)) return "";
    if (!get_token_Stored_Data(lastline, StromVerkauf)) return "";
    if (!get_token_Stored_Data(lastline, TagStromKaufStart)) return "";
    if (!get_token_Stored_Data(lastline, TagStromVerkaufStart)) return "";
    if (!get_token_Stored_Data(lastline, TagStromKauf)) return "";
    if (!get_token_Stored_Data(lastline, TagStromVerkauf)) return "";
    if (!get_token_Stored_Data(lastline, Einzelverbrauch)) return "";
    if (!get_token_Stored_Data(lastline, BatterieProduktion)) return "";
    if (!get_token_Stored_Data(lastline, BatterieVerbrauch)) return "";
    return lastline;
}