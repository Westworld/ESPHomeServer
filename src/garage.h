#ifndef GARAGE_HPP_
#define GARAGE_HPP_

#include "Sensor.h"

class Garage: public Sensor
{
    private:
        float temp=0, lasttemp, tempcounter=0;
        long counter=0;
        long counterday=0;
        long counterdaystart=0;
        enum  Tor { Auf=0, Zu, GehtAuf, GehtZu, Unklar};
        int8_t BMW = Zu;
        int8_t Mini = Zu;
        char BMWBild[5]="Zu";
        char MiniBild[5]="Zu";     
        void SetGarageDoor(String door);  
        void SetGarageDoorNew(int8_t Seite, int8_t Zustand, String ZustandBild);

    public:
        void NewReport(long counter, float temp, String Button);
        virtual bool HandleWebCall(String job, short strlen);
        virtual bool HandleMQTT(String message, short joblength, String value);
        virtual String serialize();
        virtual void RunDay();   
        virtual void ToJson(JsonObject json); 
        virtual void StatusToJson(JsonObject json);
        virtual void JsonReceive(JsonObject data);
};

#endif