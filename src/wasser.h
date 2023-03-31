#ifndef WASSER_HPP_
#define WASSER_HPP_

#include "storage.h"

#define wasserAlert 3000000  // 300000 normal, 5 Minuten

class Wasser: public Storage
{
    private:
        float temp=0;
        long counter=0;
        long wasserstarted=0, lastwasser=0;
        int8_t wasseralarm = 0;  
        int8_t heizungTempAlarm = 0; 
        long counterday=0;

    public:
        void NewReport(long counter, float temp);
        virtual void Run(int32_t zeit);
        virtual String serialize();
        virtual String WriteHeader();
        virtual String WriteData();   
        virtual String WriteDayHeader();
        virtual String WriteDayData();
        virtual String readLastLine(String &lastline);     
};

#endif