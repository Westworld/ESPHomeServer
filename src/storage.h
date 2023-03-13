#ifndef STORAGE_HPP_
#define STORAGE_HPP_

#include "SD.h"

class Storage
{
    protected:
        int32_t lastUpdated=0, nextSend=0;

    public:
        virtual String serialize();
        virtual bool needReport(int32_t zeit);
        virtual void doReport();
        virtual void Run(int32_t zeit);
        virtual void WriteHeader(File sdcard);
        virtual void WriteData(File sdcard);
};

#endif