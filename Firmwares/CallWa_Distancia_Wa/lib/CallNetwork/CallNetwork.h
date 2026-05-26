#pragma once
#include <CallConfig.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class CallEffects;  

class CallNetwork {
public:
    void begin();
    void update();
    void setEffects(CallEffects* fx);

    void sendEffect(const char* efeito,
                    const char* cor = "#23d300ff",
                    int velocidade = 50,
                    const char* to = PEER_ID);

    void sendPing();

    bool connected();
    bool wifiConnected();

    CallNetwork(CallEffects& effects);

    bool _wifiConnectedLast = false;
    bool _mqttConnectedLast = false;

private:
    WiFiClientSecure _secureClient;
    PubSubClient _client{_secureClient};

    CallEffects* _effects = nullptr;

    unsigned long _lastPing = 0;
    unsigned long _lastReconnectAttempt = 0;

    void connectWiFi();
    void connectMQTT();

    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};