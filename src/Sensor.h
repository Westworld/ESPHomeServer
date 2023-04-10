#ifndef Sensor_HPP_
#define Sensor_HPP_

#include <ESP32WebServer.h>

extern void UDBDebug(String message);
extern void MQTT_Send(char const * topic, float value); 
extern void MQTT_Send(char const * topic, int16_t value);
extern void MQTT_Send(char const * topic, long value);
extern void MQTT_Send(char const * topic, String value);
extern void EMail_Send(String textmessage);

class Sensor
{
    protected:
        int32_t lastUpdated=0;

    public:
        virtual String serialize();
        virtual void Run(int32_t zeit);
        virtual bool HandleWebCall(String Job, short strlen);
        virtual bool HandleMQTT(String message, short joblength, String value);
        virtual String WriteHeader();
        virtual String WriteDayHeader(); 
        virtual String WriteData();
        virtual String WriteDayData();
        virtual String readLastLine(String &lastline);
        bool get_token_Stored_Data(String &from, float &to);
};

#endif