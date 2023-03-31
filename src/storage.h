#ifndef STORAGE_HPP_
#define STORAGE_HPP_

extern void UDBDebug(String message);
extern void MQTT_Send(char const * topic, float value); 
extern void MQTT_Send(char const * topic, int16_t value);
extern void MQTT_Send(char const * topic, long value);
extern void MQTT_Send(char const * topic, String value);
extern void EMail_Send(String textmessage);

class Storage
{
    protected:
        int32_t lastUpdated=0;

    public:
        virtual String serialize();
        virtual void Run(int32_t zeit);
        virtual String WriteHeader();
        virtual String WriteDayHeader(); 
        virtual String WriteData();
        virtual String WriteDayData();
        virtual String readLastLine(String &lastline);
        bool get_token_Stored_Data(String &from, float &to);
};

#endif