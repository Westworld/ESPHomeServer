#ifndef STORAGE_HPP_
#define STORAGE_HPP_

extern void UDBDebug(String message);
extern void MQTT_Send(char const * topic, float value); 
extern void MQTT_Send(char const * topic, int16_t value);
extern void MQTT_Send(char const * topic, long value);

class Storage
{
    protected:
        int32_t lastUpdated=0;

    public:
        virtual String serialize();
        virtual void Run(int32_t zeit);
        virtual String WriteHeader();
        virtual String WriteData();
        virtual String readLastLine(String &lastline);
        bool get_token_Stored_Data(String &from, float &to);
};

#endif