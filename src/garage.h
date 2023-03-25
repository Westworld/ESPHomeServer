#ifndef GARAGE_HPP_
#define GARAGE_HPP_

#include "storage.h"

class Garage: public Storage
{
    private:
        float temp=0;
        long counter=0;
        long counter5min=0; long counter5minTime=0;
        long counterday=0;

    public:
        void NewReport(long counter, float temp);
        virtual String serialize();
        virtual String WriteHeader();
        virtual String WriteData();   
        virtual String WriteDayHeader();
        virtual String WriteDayData();   
        virtual String readLastLine(String &lastline);     
};

#endif