#ifndef MHZSensor_HPP_
#define MHZSensor_HPP_

#include "storage.h"
#include <MHZ19.h>

#define repeatTimer 60000

class MHZSensor: public Storage
{
    private:
        int16_t Co2, temp, Accuracy;
        MHZ19 * mhz;

    public:
        MHZSensor(Stream * stream);
        virtual void Run(int32_t zeit);
        virtual String serialize();
        virtual void doReport();
        virtual String WriteHeader();
        virtual String WriteData();        
};

#endif