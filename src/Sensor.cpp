#include "Arduino.h"
#include "Sensor.h"

extern ESP32WebServer server;

String Sensor::serialize() {
    String result="";

    return result;
}

void Sensor::Run(int32_t zeit) {
    //Serial.print("Sensor: ");//+zeit);
}

void Sensor::runStunde() {
    
}

bool Sensor::HandleWebCall(String Job, short strlenr) {
    return false;
}

bool Sensor::HandleMQTT(String message, short joblength, String value) {
    return false;
}

String Sensor::WriteHeader() {
    return ";Sensor";
}

String Sensor::WriteDayHeader() {
    return "";
}

String Sensor::WriteData() {
    return ";Sensor";
}


String Sensor::WriteDayData() {
    return "";
}

String Sensor::readLastLine(String &lastline) {
    return lastline;
}

void Sensor::ToJson(JsonObject json) {

}

bool Sensor::get_token_Stored_Data(String &from, float &to)
{
  int16_t pos = from.indexOf(';');
  if(pos >= 0) {
    to = from.substring(0, pos).toFloat();
    from = from.substring(pos+1);
    return true;
  } 
  else  {
    to = from.toFloat();
    from = "";
    return false;
  }  
}

double Sensor::round2(double value) {
   return (int)(value * 100 + 0.5) / 100.0;
}