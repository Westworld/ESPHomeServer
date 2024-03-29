#ifndef WAAGE_HPP_
#define WAAGE_HPP_

#include "Sensor.h"

class Waage: public Sensor
{
    private:
        float Buddy=0, Mika=0, Matti=0, Timmi=0;
        float tag_Buddy=0, tag_Mika=0, tag_Matti=0;
        int32_t tag_Buddy_last=0, tag_Mika_last=0, tag_Matti_last=0;
        int16_t lastUpdateBuddy, lastUpdateMika, lastUpdateMatti, lastUpdateTimmi;
        void CheckTagUpdate();

    public:
        Waage();
        enum  wer { warBuddy=0, warMika, warMatti, warTimmi, unbekannt };
        virtual String serialize(); 
        virtual void run(int32_t zeit);
        virtual bool HandleMQTT(String message, short joblength, String value);
        virtual void ToJson(JsonObject json);     
        virtual void StatusToJson(JsonObject json);
        virtual void JsonReceive(JsonObject data);
};

#endif