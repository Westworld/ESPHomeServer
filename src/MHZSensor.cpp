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
        if (!nextSend)
            nextSend = zeit + 30000;  // in next 60 seconds
   Serial.print("MHZ last");
 //  Serial.println(lastUpdated);

        MHZ19_RESULT response = mhz->retrieveData();
        if (response == MHZ19_RESULT_OK)
        {
            Co2 = mhz->getCO2();
           Serial.print("co2: ");
           Serial.println(Co2); 
            temp = mhz->getTemperature();
                //       Serial.print("temp: ");
                //       Serial.println(temp); 
            Accuracy = mhz->getAccuracy();
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
    result += ", Accuracy = ";
    result += Accuracy;
    result += "\nLastUpdate = ";
    result += lastUpdated;
    result += " nextSend = ";
    result += nextSend; 
    return result;
}

void MHZSensor::doReport() {
    if (nextSend) {
        nextSend = 0;
        Serial.println("Neuer Report für MHZSensor: ");
        Serial.println(serialize());
    }
}

void MHZSensor::WriteHeader(File sdcard) {
    sdcard.print(";CO2;Temp;Accuracy");
}

void MHZSensor::WriteData(File sdcard) {
  char buffer[100];
  snprintf(buffer, 100, ";%d;%d;%d", Co2, temp, Accuracy);
  sdcard.print(buffer);    
}