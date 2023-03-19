#include "Arduino.h"
#include "waage.h"

void Waage::NewScale(wer welchewaage, float fGewicht) {
    int32_t zeit = millis();
    lastUpdated = zeit;
    UDBDebug(String(welchewaage)+" "+String(fGewicht));

    switch (welchewaage) {
        case warBuddy : 
            if ((fGewicht>6.0) & (fGewicht<6.8)) {
                Buddy = fGewicht; 
                tag_Buddy_last = zeit;  
                MQTT_Send((char const *) "HomeServer/Tiere/Buddy", fGewicht); 
            }        
            break;
         case warMika : 
            fGewicht = fGewicht*1.01;
            if ((fGewicht>6.4) & (fGewicht<7)) {
                Mika = fGewicht;
                MQTT_Send((char const *) "HomeServer/Tiere/Mika", fGewicht); 
                tag_Mika_last = zeit;
            }    
            break;
         case warMatti : 
            fGewicht = fGewicht*0.99;
            if ((fGewicht>7) & (fGewicht<7.5)) {
                Matti = fGewicht;
                MQTT_Send((char const *) "HomeServer/Tiere/Matti", fGewicht); 
                tag_Matti_last = zeit;
            }    
            break;
        case warTimmi : 
            fGewicht = fGewicht * 0.89;
            if ((fGewicht>8.2) & (fGewicht<9)) {
                Timmi = fGewicht; 
                MQTT_Send((char const *) "HomeServer/Tiere/Timmi", fGewicht); 
            }    
            break;
    }
    UDBDebug(String(fGewicht));

    
    zeit -= 3000; // 3 sekunden
    if ((tag_Buddy_last > zeit) && (tag_Mika_last > zeit)  && (tag_Matti_last > zeit)) {
        tag_Buddy = Buddy;
        tag_Mika = Mika;
        tag_Matti = Matti;
        MQTT_Send((char const *) "HomeServer/Tiere/Tag_Buddy", Buddy); 
        MQTT_Send((char const *) "HomeServer/Tiere/Tag_Mika", Mika); 
        MQTT_Send((char const *) "HomeServer/Tiere/Tag_Matti", Matti); 
    }
    UDBDebug(serialize());
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
    result += "\nTageswerte: Buddy = ";
    result += tag_Buddy;
    result += " Mika = ";
    result += tag_Mika;
    result += " Matti = ";
    result += tag_Matti;
    result += "\nLast Zeit: Buddy = ";
    result += tag_Buddy_last;
    result += " Mika = ";
    result += tag_Mika_last;
    result += " Matti = ";
    result += tag_Matti_last;  
    result += "\nLastUpdate = ";
    result += lastUpdated;

    return result;
}

String Waage::WriteHeader() {
    return ";Buddy;Mika;Matti;Timmi;Buddy_Tag;Mika_Tag;Matti_Tag";
}

String Waage::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%.1f;%.1f;%.1f;%.1f;%.1f;%.1f;%.1f", Buddy, Mika, Matti, Timmi, tag_Buddy, tag_Matti, tag_Mika);
  return buffer;    
}