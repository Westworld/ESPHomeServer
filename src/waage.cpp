#include "Arduino.h"
#include "waage.h"

extern struct tm timeinfo;

Waage::Waage(void) {
    lastUpdateBuddy = lastUpdateMika = lastUpdateMatti = lastUpdateTimmi = timeinfo.tm_yday;
}

void Waage::NewScale(wer welchewaage, float fGewicht) {
    int32_t zeit = millis();
    lastUpdated = zeit;
    //UDBDebug(String(welchewaage)+" "+String(fGewicht));

    if ((fGewicht > 5) ) {

        switch (welchewaage) {
            case warBuddy : 
                if ((fGewicht>6.0) & (fGewicht<6.8)) {
                    Buddy = fGewicht; 
                    tag_Buddy_last = zeit;  
                    lastUpdateBuddy = timeinfo.tm_yday;
                    MQTT_Send((char const *) "HomeServer/Tiere/Buddy", fGewicht); 
                }        
                break;
            case warMika : 
                fGewicht = fGewicht*1.01;
                if ((fGewicht>6.4) & (fGewicht<7.2)) {       
                    Mika = fGewicht;
                    MQTT_Send((char const *) "HomeServer/Tiere/Mika", fGewicht);          
                    lastUpdateMika = timeinfo.tm_yday;                
                    tag_Mika_last = zeit;
                }    
                break;
            case warMatti : 
                fGewicht = fGewicht*0.99;
                if ((fGewicht>6.9) & (fGewicht<7.5)) {
                    Matti = fGewicht;
                    lastUpdateMatti = timeinfo.tm_yday;
                    MQTT_Send((char const *) "HomeServer/Tiere/Matti", fGewicht); 
                    tag_Matti_last = zeit;
                }    
                break;
            case warTimmi : 
                fGewicht = fGewicht * 0.89;
                if ((fGewicht>8.2) & (fGewicht<9)) {
                    Timmi = fGewicht; 
                    lastUpdateTimmi = timeinfo.tm_yday;
                    MQTT_Send((char const *) "HomeServer/Tiere/Timmi", fGewicht); 
                }    
                break;
        }
        //UDBDebug(String(fGewicht));

        
        zeit -= 20000; // 15 sekunden
        if ((tag_Buddy_last != 0) && (tag_Buddy_last > zeit) && (tag_Mika_last > zeit)  && (tag_Matti_last > zeit)) {
            tag_Buddy = Buddy;
            tag_Mika = Mika;
            tag_Matti = Matti;
            MQTT_Send((char const *) "HomeServer/Tiere/Tag_Buddy", Buddy); 
            MQTT_Send((char const *) "HomeServer/Tiere/Tag_Mika", Mika); 
            MQTT_Send((char const *) "HomeServer/Tiere/Tag_Matti", Matti); 
        }
        UDBDebug(serialize());
    }

    if (fGewicht>2) {
        MQTT_Send((char const *) "display/Gewicht", fGewicht);
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
    result += " LastUpdate = ";
    result += lastUpdated;

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