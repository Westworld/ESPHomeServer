#ifndef Sensor_HPP_
#define Sensor_HPP_

#include <ESP32WebServer.h>
#include "ArduinoJson.h"

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
        virtual void runDay(); 
        virtual void runStunde(); 
        virtual void ToJson(JsonObject json);
        virtual void StatusToJson(JsonObject json);
        bool get_token_Stored_Data(String &from, float &to);
        float round2(float value);
        float round1(float value);        
};

#endif