#include "CallNetwork.h"
#include "CallEffects.h"
#include "CallOTA.h"
#include <ArduinoOTA.h>

CallEffects effects;
CallNetwork network(effects);

void setup() {
    Serial.begin(115200);

    effects.begin();
    network.begin();
    Serial.println(WiFi.localIP());

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
}

Serial.println(WiFi.localIP());

ArduinoOTA.setHostname(DEVICE_ID);
ArduinoOTA.setPassword("123456");
ArduinoOTA.begin();


    network.setEffects(&effects);   
}

void loop() {
    ArduinoOTA.handle();
    network.update();
    effects.update();
    Serial.println(WiFi.localIP());
}