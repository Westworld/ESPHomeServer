#include "Arduino.h"
#include "Homematic.h"

extern void HTTP_Send(String host, int httpPort, String url);
extern void UDBDebug(String message);

String HM_host = "192.168.0.46";
int HM_Port = 80;

void Homematic_Set(String device, int8_t status) {
    String Code = Homematic_NameToCode(device);
    // http://192.168.0.46/addons/red/Licht?msg=HmIP-RF.000857099102F1:4.STATE&load=2
    if (Code != "") {
        String url = "/addons/red/Licht?msg="+Code;
        url += "&load=";
        url += String(status);
        UDBDebug(url);
        HTTP_Send(HM_host, HM_Port, url);
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