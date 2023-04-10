#include "Arduino.h"
#include "Garage.h"

extern struct tm timeinfo;
extern ESP32WebServer server;

bool Garage::HandleWebCall(String job, short strlen) {
    if (job == "Garage") {
        long counter = 0;
        float temp = 127;
        String Button="";
        if (server.hasArg("Strom")) {
          String SCounter = server.arg("Strom"); 
          counter = SCounter.toInt();
        }
        if (server.hasArg("Temp2")) {
          String STemp = server.arg("Temp2"); 
          temp = STemp.toFloat();
        }   
        if (server.hasArg("Button")) {
          Button = server.arg("Button"); 
        } 
        NewReport(counter, temp, Button);   
        server.send(200, "text/html", String("OK Garage"));  
        return true;
    }

    return false;
}

bool Garage::HandleMQTT(String message, short joblength, String value) {
  const char IsLeft[] = "garage/TargetDoorState/BMW";
  const char IsRight[] = "garage/TargetDoorState/Mini";

  switch (joblength) {
    case sizeof(IsLeft):
        if (message == IsLeft) {
            UDBDebug("open BMW");
            EMail_Send("HomeKit Open VW Garage"); 
            return true;
        }
        break;
    case sizeof(IsRight):
        if (message == IsRight) {
            UDBDebug("open Mini");
            return true;
        }
        break;
    default:
        break;
  }

  return false;
}

void Garage::NewReport(long newcounter, float newtemp, String Button) {
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

        counterday+=offset;   

    }


    if (newtemp != 127) {
        temp = round(newtemp);
            if (lasttemp == temp) {
             if (tempcounter++ > 60)   {
                //MQTT_Send((char const *) "HomeServer/Garage/Temp", temp);
                MQTT_Send("hm/set/CUX9002004:1/SET_TEMPERATURE",temp);  
                tempcounter = 0;
                }
            }
            else {
                //MQTT_Send((char const *) "HomeServer/Garage/Temp", temp); 
                MQTT_Send("hm/set/CUX9002004:1/SET_TEMPERATURE",temp);  
                tempcounter = 0;
            }
            lasttemp = temp;  

        
    }

    if (Button != "") {
        //UDBDebug("Garage Button: "+Button); 
        if (Button.length()>3) {
            SetGarageDoor(Button.substring(0, 3));      // substring, last index excluding   
            SetGarageDoor(Button.substring(3, 6));         
        }
        else {
            SetGarageDoor(Button);
        }
    }
    // UDBDebug(serialize());
}     

void Garage::SetGarageDoorNew(int8_t Seite, int8_t Zustand, String ZustandBild) { 

    if (Seite == 0) {
        // BMW/VW
        BMW = Zustand;
        strncpy(BMWBild, ZustandBild.c_str(), 5);
        MQTT_Send("garage/CurrentDoorState/BMW", (int16_t) Zustand);
        MQTT_Send("garage/CurrentDoorState/BMWBild", ZustandBild);
        if (Zustand == Zu) {
            
        }
    }
    else {
        // Mini
        Mini = Zustand;
        strncpy(MiniBild, ZustandBild.c_str(), 5);
        MQTT_Send("garage/CurrentDoorState/Mini", (int16_t) Zustand);
        MQTT_Send("garage/CurrentDoorState/MiniBild", ZustandBild);
        }
    //MQTT_Send("display/status", String(millis()));  ??? bevor Daten bei 4D angekommen sind?
}

void Garage::SetGarageDoor(String door) {
    int8_t Seite = -1;
    int8_t ZustandAlt = -1;

    if (door.charAt(0) == 'A') {
        Seite = 0;
        ZustandAlt = BMW;
    }    
    if (door.charAt(0) == 'B') {
        Seite = 1;  
        ZustandAlt = Mini;
    }    
    if (Seite < 0) {
        UDBDebug("invalid Garage Seite: "+door);  
        return;
    }

    door = door.substring(1);

    if (door == "10") {
        if (ZustandAlt==Zu)
            return;
        else {
            SetGarageDoorNew(Seite, Zu, "Zu");
            return;
        }
    }
    if (door == "00") {
        if (ZustandAlt==Unklar)
            return;
        else {
            SetGarageDoorNew(Seite, Unklar, "<>");
            return;
        }
    }
    if (door == "01") {
        if (ZustandAlt==Auf)
            return;
        else {
            SetGarageDoorNew(Seite, Auf, "Auf");
            return;
        }
    }
    if (door == "11") {
        if (ZustandAlt==Auf) {
            SetGarageDoorNew(Seite, GehtZu, ">|");
            return;
        }    
        else 
            if (ZustandAlt==Zu) {
                SetGarageDoorNew(Seite, GehtAuf, "|>");
                return;
            }
            else
                {
                    SetGarageDoorNew(Seite, Unklar, "<>");
                    return;
                }
    }
    
 
}

String Garage::serialize() {
    String result="";

    result = "Garage Counter = ";
    result += counter;
    result += ", DayCounter = ";
    result += counterday;    
    result += "\nLastUpdate = ";
    result += lastUpdated;

    return result;
}

String Garage::WriteHeader() {
    return ";Garage_Counter;Garage_Day";
}

String Garage::WriteDayHeader() {
    return ";Garage_Day";
}

String Garage::WriteData() {
  char buffer[100];
  snprintf(buffer, 100, ";%d;%d", counter, counterday);
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
    if (!get_token_Stored_Data(lastline, result)) return "";
    counterday = result;
    return lastline;

}