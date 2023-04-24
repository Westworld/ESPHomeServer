#include "Arduino.h"
#include "waage.h"

extern struct tm timeinfo;
extern ESP32WebServer server;

Waage::Waage(void) {
    lastUpdateBuddy = lastUpdateMika = lastUpdateMatti = lastUpdateTimmi = timeinfo.tm_yday;
}


bool Waage::HandleMQTT(String message, short joblength, String value) {
  const char IsTimmi[] = "HomeServer/Tiere/Timmi";
  const char IsBuddy[] = "HomeServer/Tiere/Buddy";
  const char IsMatti[] = "HomeServer/Tiere/Matti";
  const char IsMika[] = "HomeServer/Tiere/Mika";

  switch (joblength) {
    case sizeof(IsTimmi):  /* wie isBuddy und Matti */
        if (message == IsTimmi) {
            Timmi = value.toFloat(); 
            lastUpdateTimmi = timeinfo.tm_yday;
            return true;
        }
        if (message == IsBuddy) {
            Buddy = value.toFloat(); 
            lastUpdateBuddy = timeinfo.tm_yday;
            tag_Buddy_last = millis();  
            CheckTagUpdate();
            return true;
        }
        if (message == IsMatti) {
            Matti = value.toFloat(); 
            tag_Matti_last = millis();  
            lastUpdateMatti = timeinfo.tm_yday;
            CheckTagUpdate();
            return true;
        }
        break;
    case sizeof(IsMika):  
        if (message == IsMika) {
            Mika = value.toFloat(); 
            tag_Mika_last = millis();  
            lastUpdateMika = timeinfo.tm_yday;
            CheckTagUpdate();
            return true;
        }        
    default:
        break;
  }
  return false;
}

void Waage::CheckTagUpdate() {
        int32_t zeit = millis() - 20000; // 15 sekunden
        if ((tag_Buddy_last != 0) && (tag_Buddy_last > zeit) && (tag_Mika_last > zeit)  && (tag_Matti_last > zeit)) {
            tag_Buddy = Buddy;
            tag_Mika = Mika;
            tag_Matti = Matti;
            MQTT_Send((char const *) "HomeServer/Tiere/Tag_Buddy", Buddy); 
            MQTT_Send((char const *) "HomeServer/Tiere/Tag_Mika", Mika); 
            MQTT_Send((char const *) "HomeServer/Tiere/Tag_Matti", Matti); 
        //UDBDebug(serialize());
        }
}

void Waage::run(int32_t zeit) {
    int16_t last = timeinfo.tm_yday-3;
    if (lastUpdateTimmi<last) Timmi = 0;
    if (lastUpdateBuddy<last) { Buddy = 0; tag_Buddy = 0;}
    if (lastUpdateMatti<last) { Matti = 0; tag_Matti = 0;}
    if (lastUpdateBuddy<last) { Mika = 0; tag_Mika = 0;}
}

String Waage::serialize() {
    String result="";

    result = "Buddy = ";
    result += Buddy;
    result += ", Mika = ";
    result += Mika;
    result += ", Matti = ";
    result += Matti;
    result += ", Timmi = ";
    result += Timmi;
    result += " Tageswerte: Buddy = ";
    result += tag_Buddy;
    result += " Mika = ";
    result += tag_Mika;
    result += " Matti = ";
    result += tag_Matti;
    result += " Last Zeit: Buddy = ";
    result += tag_Buddy_last;
    result += " Mika = ";
    result += tag_Mika_last;
    result += " Matti = ";
    result += tag_Matti_last;  

    return result;
}


void Waage::ToJson(JsonObject json){
    json["Buddy"] = round2(Buddy);
    json["Mika"] = round2(Mika);
    json["Matti"] = round2(Matti);
    json["Timmi"] = round2(Timmi);
    json["tag_Buddy"] = round2(tag_Buddy);
    json["tag_Mika"] = round2(tag_Mika);
    json["tag_Matti"] = round2(tag_Matti);     
}

void Waage::StatusToJson(JsonObject json){
    ToJson(json);
}

void Waage::JsonReceive(JsonObject data) {
    if (Buddy == 0)
        Buddy = data["Buddy"]; // 6.69
    if (Mika == 0)
        Mika = data["Mika"]; // 6.72
    if (Matti == 0)
        Matti = data["Matti"]; // 7.18
    if (Timmi == 0)
        Timmi = data["Timmi"]; // 8.32
    if (tag_Buddy == 0)
        tag_Buddy = data["tag_Buddy"]; // 6.69
    if (tag_Mika == 0)
        tag_Mika = data["tag_Mika"]; // 6.7
    if (tag_Matti == 0)
        tag_Matti = data["tag_Matti"]; // 7.33

}
