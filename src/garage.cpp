#include "Arduino.h"
#include "Garage.h"

extern struct tm timeinfo;

void Garage::NewReport(long newcounter, float newtemp) {
    int32_t zeit = millis();
    lastUpdated = zeit;
    //UDBDebug("Garage "+String(counter)+" "+String(temp));

    if (newcounter != 0)
    {
        long offset;
        
        if (counter != 0)
         offset =  newcounter-counter;
        else
         offset = 1;
        counter = newcounter;
        if (offset != 0)
            MQTT_Send((char const *) "HomeServer/Garage/Counter", offset); 

        // counter 5 min
        counter5min+=offset;
        if ((counter5minTime + 300000)<zeit) {
            if (counter5min != 0) {
                MQTT_Send((char const *) "HomeServer/Garage/Counter5min", counter5min); 
                counter5min = 0;
            }
            counter5minTime = zeit;
        }
        counterday+=offset;   

    }


    if (newtemp != 127) {
        temp = newtemp;
        MQTT_Send((char const *) "HomeServer/Garage/Temp", temp);
    }

    // UDBDebug(serialize());
}       

String Garage::serialize() {
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

String Garage::WriteHeader() {
    return ";Garage_Counter;Garage_Temp;Garage_Day";
}

String Garage::WriteDayHeader() {
    return ";Garage_Day";
}

String Garage::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%d;%.1f;%d", counter, temp, counterday);
  return buffer;    
}

String Garage::WriteDayData() {
  MQTT_Send((char const *) "HomeServer/Garage/CounterDay", counterday); 
  char buffer[100];
  snprintf(buffer, 100, ";%d", counterday);
  counterday=0;
  return buffer;    
}

String Garage::readLastLine(String &lastline) {
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