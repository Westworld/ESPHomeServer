#include "Arduino.h"
#include "storage.h"


String Storage::serialize() {
    String result="";

    return result;
}

bool Storage::needReport(int32_t zeit) {
    return ((nextSend !=0) && (nextSend < zeit));
}

void Storage::doReport(void) {
    nextSend = 0;
}

void Storage::Run(int32_t zeit) {
    //Serial.print("storage: ");//+zeit);
}

void Storage::WriteHeader(File sdcard) {

}

void Storage::WriteData(File sdcard) {

}