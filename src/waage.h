#ifndef WAAGE_HPP_
#define WAAGE_HPP_

#include "storage.h"

class Waage: public Storage
{
    private:
        float Buddy=0, Mika=0, Matti=0, Timmi=0;
        float tag_Buddy=0, tag_Mika=0, tag_Matti=0;
        int32_t tag_Buddy_last=0, tag_Mika_last=0, tag_Matti_last=0;

    public:
        enum  wer { warBuddy=0, warMika, warMatti, warTimmi, unbekannt };
        void NewScale(wer welchewaage, float Gewicht);
        virtual String serialize();
        virtual void doReport();
        void WriteHeader(File sdcard);
        void WriteData(File sdcard);        
};

#endif