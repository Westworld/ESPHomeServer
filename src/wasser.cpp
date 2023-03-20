#include "Arduino.h"
#include "wasser.h"

extern struct tm timeinfo;



Wasser::Wasser() {
    countercurday = timeinfo.tm_wday;
}

void Wasser::NewReport(long newcounter, float newtemp) {
    int32_t zeit = millis();
    lastUpdated = zeit;
    UDBDebug("Wasser "+String(counter)+" "+String(temp));

    if (newcounter != 0)
    {
        counter = newcounter;
        lastwasser = zeit;
        if (wasserstarted == 0) wasserstarted = zeit;
        else {
            if ((wasserstarted + wasserAlert)< zeit) {
                // alarm !
                if (wasseralarm == 0)
                    MQTT_Send((char const *) "HomeServer/Heizung/WasserAlarm", 1L);   
                wasseralarm = -1;         
            }
        }
        MQTT_Send((char const *) "HomeServer/Heizung/Wasser", counter); 

        // counter 5 min
        counter5min++;
        if ((counter5minTime + 300000)<zeit) {
            if (counter5min != 0) {
                MQTT_Send((char const *) "HomeServer/Heizung/Wasser5min", counter5min); 
                counter5min = 0;
            }
            counter5minTime = zeit;
        }

        counterday++;
        if (countercurday != timeinfo.tm_wday) {
            countercurday = timeinfo.tm_wday;
            MQTT_Send((char const *) "HomeServer/Heizung/WasserDay", counterday); 
            counterday = 0;
        }
    }


    if (newtemp != 127) {
        temp = newtemp;
        MQTT_Send((char const *) "HomeServer/Heizung/Temp", temp);
    }

    UDBDebug(serialize());
}

void Wasser::Run(int32_t zeit) {
    if (wasserstarted != 0) {
        if ((lastwasser + 30000)< zeit) {  // 30 Sekunden ohne Wasser
            wasserstarted = 0;
            if (wasseralarm  != 0) {
                MQTT_Send((char const *) "HomeServer/Heizung/WasserAlarm", 0L); 
                wasseralarm = 0;
            }    
        }
    }  
}

String Wasser::serialize() {
    String result="";

    result = "Counter = ";
    result += counter;
    result += ", Temp = ";
    result += temp;
    result += ", DayCounter = ";
    result += counterday;    
    result += "\nLastUpdate = ";
    result += lastUpdated;

    return result;
}

String Wasser::WriteHeader() {
    return ";Wasser_Counter;Heizung_Temp;Wasser_Day";
}

String Wasser::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%d;%.1f;%d", counter, temp, counterday);
  return buffer;    
}

String Wasser::readLastLine(String &lastline) {
    // first ; already removed
    int16_t index = 0;
    float result = 0;
    if (!get_token_Stored_Data(lastline, result)) return "";
    counter = result;
    if (!get_token_Stored_Data(lastline, temp)) return "";
    if (!get_token_Stored_Data(lastline, result)) return "";
    counterday = result;
    return lastline;

}