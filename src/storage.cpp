#include "Arduino.h"
#include "storage.h"


String Storage::serialize() {
    String result="";

    return result;
}

void Storage::Run(int32_t zeit) {
    //Serial.print("storage: ");//+zeit);
}

String Storage::WriteHeader() {
    return ";storage";
}

String Storage::WriteData() {
    return ";storage";
}

