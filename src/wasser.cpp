#include "Arduino.h"
#include "wasser.h"

extern struct tm timeinfo;



bool Wasser::HandleMQTT(String message, short joblength, String value) {

  const char IsDay[] = "HomeServer/Heizung/WasserDay";
  const char IsCounter[] = "HomeServer/Heizung/Wasser";

  switch (joblength) {
    case sizeof(IsDay):
        if (message == IsDay) {
            counterday = value.toFloat(); 
            lastUpdated = millis();
            //UDBDebug("DEBUG IsDay "+message +" - "+String(counterday));
            return true;
        }
        break;
    case sizeof(IsCounter):
        if (message == IsCounter) {
            counter = value.toFloat(); 
            lastUpdated = millis();
            //UDBDebug("DEBUG### isCounter "+message +" - "+String(counter));
            return true;
        }
        break;
    default:
        break;
  }

  return false;
}

void Wasser::ToJson(JsonObject json){
    json["counter"] = counter;
    json["counterday"] = counterday;
}

void Wasser::StatusToJson(JsonObject json){
    ToJson(json);
}

void Wasser::RunDay() {
  MQTT_Send((char const *) "HomeServer/Wasser/CounterDay", counterday); 
  counterday=0; 
}

String Wasser::serialize() {
    String result="";

    result = "Wasser Counter = ";
    result += counter;
    result += ", DayCounter = ";
    result += counterday;    
    result += "\nLastUpdate = ";
    result += lastUpdated;

    return result;
}

