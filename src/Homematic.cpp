#include "Arduino.h"
#include "Homematic.h"

extern void HTTP_Send(String url);
extern void UDBDebug(String message);


void Homematic_Set(String device, int8_t status) {
    String Code = Homematic_NameToCode(device);
    // http://192.168.0.46/addons/red/Licht?msg=HmIP-RF.000857099102F1:4.STATE&load=2
    if (Code != "") {
        String url = "http://192.168.0.46/addons/red/Licht?msg="+Code;
        url += "&load=";
        url += String(status);
        UDBDebug(url);
        HTTP_Send(url);
    }    
    else
        UDBDebug("invalid: "+device);
}

String Homematic_NameToCode(String Name) {
    if (Name == "Bad")
        return "HmIP-RF.000857099102F1:4.STATE";

    if (Name == "Dach")
        return "BidCos-RF.NEQ0383519:1.STATE";

    if (Name == "Lichtwarner_SZ")
        return "CUxD.CUX2801002:4.STATE";
    return "";
}

String Homematic_CodeToName(String Code) {
    if (Code == "HmIP-RF.000857099102F1:4.STATE") {
        return "Bad";
    }

    return ""; 
}