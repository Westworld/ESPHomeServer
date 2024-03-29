#ifndef STROM_HPP_
#define define

#include "Sensor.h"


class Strom: public Sensor
{
    private:
        float StromKauf=0, StromVerkauf=0;
        float curStromKauf=0, curStromVerkauf=0;
        float TagStromKaufStart=0, TagStromVerkaufStart=0;
        float TagStromKauf=0, TagStromVerkauf=0;
        float Einzelverbrauch=0, BatterieVerbrauch = 0, GesamtVerbrauch=0;
        float TagEinzelVerbrauch=0, TagGesamtVerbrauch=0, LastBatVerbrauch=0;
        long LastEinzelVerbrauch=0, LastGesamtVerbrauch=0, LastProduktion=0;
        float ProduktionAvg[15];
        int8_t ProduktionCounter = 0;
        float TagProduktion=0;
        int8_t WallboxCar=0, WallboxAmp=0, WallboxPsm=0;
        int32_t WallboxEto=0, WallboxNrg=0, WallboxEtoStart=0;
        bool WallboxAlw=false;


    public:
        virtual bool HandleMQTT(String message, short joblength, String value);
        virtual String serialize();
        virtual void ToJson(JsonObject json); 
        virtual void StatusToJson(JsonObject json); 
        virtual void runStunde(); 
        virtual void runDay();    
        virtual void JsonReceive(JsonObject data);    
};

#endif