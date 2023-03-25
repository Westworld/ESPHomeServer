#include "Arduino.h"
#include "Garage.h"

extern struct tm timeinfo;

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
        temp = newtemp;
        MQTT_Send((char const *) "HomeServer/Garage/Temp", temp);
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