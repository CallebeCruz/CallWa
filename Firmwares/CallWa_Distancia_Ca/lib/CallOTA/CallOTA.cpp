#include "CallOTA.h"
#include "CallConfig.h"
#include <ESPmDNS.h>

void CallOTA::begin(const char* hostname) {

    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword("123456");

    ArduinoOTA.onStart([]() {
        Serial.println("OTA Start");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]\n", error);
    });

    
    if (!MDNS.begin(DEVICE_ID)) {
        Serial.println("Erro MDNS");
    }

    ArduinoOTA.begin();
}


void CallOTA::update() {
    ArduinoOTA.handle();
}