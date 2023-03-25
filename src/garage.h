#ifndef GARAGE_HPP_
#define GARAGE_HPP_

#include "storage.h"

class Garage: public Storage
{
    private:
        float temp=0;
        long counter=0;
        long counterday=0;
        enum  Tor { Auf=0, Zu, GehtAuf, GehtZu, Unklar=-1 };
        int8_t BMW = Unklar;
        int8_t Mini = Unklar;
        char BMWBild[5]="<>";
        char MiniBild[5]="<>";     
        void SetGarageDoor(String door);  
        void SetGarageDoorNew(int8_t Seite, int8_t Zustand, String ZustandBild);

    public:
        void NewReport(long counter, float temp, String Button);
        virtual String serialize();
        virtual String WriteHeader();
        virtual String WriteData();   
        virtual String WriteDayHeader();
        virtual String WriteDayData();   
        virtual String readLastLine(String &lastline);     
};

#endif