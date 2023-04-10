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
            UDBDebug("DEBUG: Timmi erhalten über MQTT "+value);
            return true;
        }
        if (message == IsBuddy) {
            Buddy = value.toFloat(); 
            lastUpdateBuddy = timeinfo.tm_yday;
            tag_Buddy_last = millis();  
            UDBDebug("DEBUG: Buddy erhalten über MQTT "+value);
            CheckTagUpdate();
            return true;
        }
        if (message == IsMatti) {
            Matti = value.toFloat(); 
            tag_Matti_last = millis();  
            lastUpdateMatti = timeinfo.tm_yday;
            UDBDebug("DEBUG: Matti erhalten über MQTT "+value);
            CheckTagUpdate();
            return true;
        }
        break;
    case sizeof(IsMika):  
        if (message == IsMika) {
            Mika = value.toFloat(); 
            tag_Mika_last = millis();  
            lastUpdateMika = timeinfo.tm_yday;
            UDBDebug("DEBUG: Mika erhalten über MQTT "+value);
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
        UDBDebug(serialize());
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

String Waage::WriteHeader() {
    return ";Buddy;Mika;Matti;Timmi;Buddy_Tag;Mika_Tag;Matti_Tag";
}

String Waage::WriteDayHeader() {
    return ";Buddy;Mika;Matti;Timmi";
}

String Waage::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%.1f;%.1f;%.1f;%.1f;%.1f;%.1f;%.1f", Buddy, Mika, Matti, Timmi, tag_Buddy, tag_Mika, tag_Matti);
  return buffer;    
}  

String Waage::WriteDayData() {
  char buffer[100];
  snprintf(buffer, 100, ";%.1f;%.1f;%.1f;%.1f", Buddy, Mika, Matti, Timmi);
  return buffer;    
}

String Waage::readLastLine(String &lastline) {
    // first ; already removed
    int16_t index = 0;
    float result = 0;
    if (!get_token_Stored_Data(lastline, Buddy)) return "";
    if (!get_token_Stored_Data(lastline, Mika)) return "";
    if (!get_token_Stored_Data(lastline, Matti)) return "";
    if (!get_token_Stored_Data(lastline, Timmi)) return "";
    if (!get_token_Stored_Data(lastline, tag_Buddy)) return "";
    if (!get_token_Stored_Data(lastline, tag_Mika)) return "";
    if (!get_token_Stored_Data(lastline, tag_Matti)) return "";
    return lastline;

}