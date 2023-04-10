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
            return true;
        }
        break;
    case sizeof(IsCounter):
        if (message == IsCounter) {
            counter = value.toFloat(); 
            lastUpdated = millis();
            return true;
        }
        break;
    default:
        break;
  }

  return false;
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

String Wasser::WriteHeader() {
    return ";Wasser_Counter;Wasser_Day";
}

String Wasser::WriteDayHeader() {
    return ";Wasser_Day";
}

String Wasser::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%d;%d", counter, counterday);
  return String(buffer);    
}

String Wasser::WriteDayData() {
  char buffer[100];
  snprintf(buffer, 100, ";%d", counterday);
  counterday = 0;
  return String(buffer);    
}

String Wasser::readLastLine(String &lastline) {
    // first ; already removed
    int16_t index = 0;
    float result = 0;
    if (!get_token_Stored_Data(lastline, result)) return "";
    counter = result;
    if (!get_token_Stored_Data(lastline, result)) return "";
    counterday = result;
    return lastline;

}