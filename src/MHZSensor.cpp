#include "Arduino.h"
#include "HardwareSerial.h"
#include "MHZSensor.h"


MHZSensor::MHZSensor(Stream * stream) {
    //mhz = new MHZ19(stream);
        mhz = new MHZ19(&Serial1);
    lastUpdated = -repeatTimer;
}

void MHZSensor::Run(int32_t zeit) {
    //Serial.print("MHZ: ");//+zeit);

     if (zeit > (lastUpdated+repeatTimer)) {
        lastUpdated = zeit;
        //if (!nextSend)
        //    nextSend = zeit + 50000;  // in next 60 seconds

        MHZ19_RESULT response = mhz->retrieveData();
        if (response == MHZ19_RESULT_OK)
        {
            Co2 = mhz->getCO2();
            temp = mhz->getTemperature();
            temp -= 6;  // geht falsch...
            //MQTT_Send((char const *) "HomeServer/WZ/Temp", temp);
            if (lasttemp == temp) {
             if (tempcounter++ > 60)   {
                MQTT_Send("hm/set/CUX9002001:1/SET_TEMPERATURE",temp);
                tempcounter = 0;
                }
            }
            else {
                MQTT_Send("hm/set/CUX9002001:1/SET_TEMPERATURE",temp);  
                tempcounter = 0;
            }
            lasttemp = temp;  

            Co2 = round(Co2/10)*10;
            if (lastCo2 == Co2) {
             if (Co2counter++ > 60)   {
                MQTT_Send((char const *) "HomeServer/WZ/Co2", Co2); 
                Co2counter = 0;
                }
            }
            else {
               MQTT_Send((char const *) "HomeServer/WZ/Co2", Co2); 
               Co2counter = 0;
            }
            lastCo2 = Co2;            
        }
        else
        {
            Serial.print(F("Error MHZSensor, code: "));
            Serial.println(response);
        }
    }
    
}


String MHZSensor::serialize() {
    String result="";
    result = "CO2 = ";
    result += Co2;
    result += ", Temp = ";
    result += temp;
    result += "\nLastUpdate = ";
    result += lastUpdated;
    return result;
}

String MHZSensor::WriteHeader() {
    return ";CO2;Temp";
}

String MHZSensor::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%d;%d", Co2, temp);
  return buffer;    
}

String MHZSensor::readLastLine(String &lastline) {
    // first ; already removed
    int16_t index = 0;
    float fCo2, ftemp = 0;
    if (!get_token_Stored_Data(lastline, fCo2)) return "";
    Co2 = fCo2;
    if (!get_token_Stored_Data(lastline, ftemp)) return "";
    temp = ftemp;
    return lastline;

}