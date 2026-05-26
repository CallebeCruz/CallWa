#pragma once
#include <ArduinoOTA.h>

class CallOTA {
public:
    void begin(const char* hostname);
    void update();
    
};