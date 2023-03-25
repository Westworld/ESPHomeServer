#ifndef WAAGE_HPP_
#define WAAGE_HPP_

#include "storage.h"

class Waage: public Storage
{
    private:
        float Buddy=0, Mika=0, Matti=0, Timmi=0;
        float tag_Buddy=0, tag_Mika=0, tag_Matti=0;
        int32_t tag_Buddy_last=0, tag_Mika_last=0, tag_Matti_last=0;
        int16_t lastUpdateBuddy, lastUpdateMika, lastUpdateMatti, lastUpdateTimmi;

    public:
        Waage();
        enum  wer { warBuddy=0, warMika, warMatti, warTimmi, unbekannt };
        void NewScale(wer welchewaage, float Gewicht);
        virtual String serialize();
        virtual String WriteHeader();
        virtual String WriteData();  
        virtual void run(int32_t zeit); 
        virtual String WriteDayHeader();
        virtual String WriteDayData();  
        virtual String readLastLine(String &lastline);     
};

#endif