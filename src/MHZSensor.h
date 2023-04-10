#ifndef MHZSensor_HPP_
#define MHZSensor_HPP_

#include "Sensor.h"
#include <MHZ19.h>

#define repeatTimer 60000

class MHZSensor: public Sensor
{
    private:
        int16_t Co2, temp, Accuracy, lasttemp, tempcounter=0, lastCo2, Co2counter=0;
        MHZ19 * mhz;

    public:
        MHZSensor(Stream * stream);
        virtual void Run(int32_t zeit);
        virtual String serialize();
};

#endif