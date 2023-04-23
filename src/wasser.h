#ifndef WASSER_HPP_
#define WASSER_HPP_

#include "Sensor.h"

#define wasserAlert 3000000  // 300000 normal, 5 Minuten

class Wasser: public Sensor
{
    private:
        long counter=0;
        long lastwasser=0;
        long counterday=0;

    public:
        virtual bool HandleMQTT(String message, short joblength, String value);
        virtual String serialize();
        virtual void RunDay();   
        virtual void ToJson(JsonObject json); 
        virtual void StatusToJson(JsonObject json);
};

#endif