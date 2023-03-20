#ifndef WAASER_HPP_
#define WAASER_HPP_

#include "storage.h"

#define wasserAlert 60000  // 300000 normal, 5 Minuten

class Wasser: public Storage
{
    private:
        float temp=0;
        long counter=0;
        long wasserstarted=0, lastwasser=0;
        int8_t wasseralarm = 1;  // -1 Alarm, 1 aus, 0 normal
        int16_t counter5min=0; long counter5minTime=0;
        int8_t countercurday;
        long counterday=0;

    public:
        Wasser();
        void NewReport(long counter, float temp);
        virtual void Run(int32_t zeit);
        virtual String serialize();
        String WriteHeader();
        String WriteData();   
        virtual String readLastLine(String &lastline);     
};

#endif