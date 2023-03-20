#include "Arduino.h"
#include "storage.h"


String Storage::serialize() {
    String result="";

    return result;
}

void Storage::Run(int32_t zeit) {
    //Serial.print("storage: ");//+zeit);
}

String Storage::WriteHeader() {
    return ";storage";
}

String Storage::WriteData() {
    return ";storage";
}

String Storage::readLastLine(String &lastline) {
    return lastline;
}

bool Storage::get_token_Stored_Data(String &from, float &to)
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

