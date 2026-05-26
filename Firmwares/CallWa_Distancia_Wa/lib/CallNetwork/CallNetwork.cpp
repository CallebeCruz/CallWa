#include "CallNetwork.h"
#include "CallConfig.h"
#include "CallEffects.h"
#include "CallOTA.h"

static CallNetwork* instance = nullptr;
CallOTA ota;

void CallNetwork::begin() {
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    _secureClient.setInsecure();
    _client.setServer(MQTT_SERVER, MQTT_PORT);
    _client.setBufferSize(MQTT_BUFFER_SIZE);
    _client.setKeepAlive(MQTT_KEEPALIVE);
    _client.setSocketTimeout(MQTT_SOCKET_TIMEOUT);

    instance = this;
    _client.setCallback(mqttCallback);
}

// ======================================================
// WIFI
// ======================================================

void CallNetwork::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    static unsigned long lastAttempt = 0;

    if (millis() - lastAttempt > 3000) {
        lastAttempt = millis();
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
}

// ======================================================
// MQTT
// ======================================================

void CallNetwork::connectMQTT() {
    if (_client.connected()) return;

    static unsigned long lastAttempt = 0;

    if (millis() - lastAttempt > 2000) {
        lastAttempt = millis();

        _client.connect(CLIENT_ID, MQTT_USER, MQTT_PASS);

        if (_client.connected()) {
            _client.subscribe(MQTT_TOPIC_IN);
            _client.subscribe(MQTT_TOPIC_OUT);
        }
    }
}

// ======================================================
// UPDATE LOOP
// ======================================================

void CallNetwork::update() {

    bool wifiNow = (WiFi.status() == WL_CONNECTED);

if (!wifiNow) {
    if (_effects) _effects->showWifiConnecting();
    connectWiFi();
    _wifiConnectedLast = false;
    _mqttConnectedLast = false;
    return;
}

if (wifiNow && !_wifiConnectedLast) {
    if (_effects) _effects->flashWifiConnected();
    _wifiConnectedLast = true;
}

if (!_client.connected()) {
    connectMQTT();
}

bool mqttNow = _client.connected();

if (mqttNow && !_mqttConnectedLast) {
    if (_effects) _effects->flashMqttConnected();
    _mqttConnectedLast = true;
}

if (!mqttNow) {
    _mqttConnectedLast = false;
}

_client.loop();

    // -------- PING --------
    if (millis() - _lastPing > PING_INTERVAL_MS) {
        sendPing();
        _lastPing = millis();
    }
}

// ======================================================
// STATUS
// ======================================================

bool CallNetwork::connected() {
    return _client.connected();
}

bool CallNetwork::wifiConnected() {
    return WiFi.status() == WL_CONNECTED;

    ota.begin(DEVICE_ID);
}

// ======================================================
// SEND EFFECT
// ======================================================
void CallNetwork::setEffects(CallEffects* fx) {
    _effects = fx;
}

void CallNetwork::sendEffect(const char* efeito,
                             const char* cor,
                             int velocidade,
                             const char* to)

{

    if (_effects) {
        Serial.println("Cancelando overlay local (envio)");
        _effects->cancelOverlayOnly();
    }

    JsonDocument doc;
    doc["from"] = DEVICE_ID;
    doc["to"] = to;

    doc["efeito"] = efeito;
    doc["cor"] = cor;
    doc["velocidade"] = velocidade;

    char buffer[256];
    serializeJson(doc, buffer);

    _client.publish(MQTT_TOPIC_OUT, buffer);
}

// ======================================================
// SEND PING
// ======================================================

void CallNetwork::sendPing() {

    JsonDocument doc;
    doc["id"] = DEVICE_ID;
    doc["from"] = DEVICE_ID;
    doc["to"] = "ALL";
    doc["status"] = "ok";
    doc["timestamp"] = millis();

    char buffer[128];
    serializeJson(doc, buffer);

    _client.publish(MQTT_TOPIC_PING, buffer);
}

CallNetwork::CallNetwork(CallEffects& effects)
{
        _effects = &effects;
}

// ======================================================
// CALLBACK (ESTÁTICO)
// ======================================================

void CallNetwork::mqttCallback(char* topic, byte* payload, unsigned int length) {

    if (instance == nullptr) return;

    String mensagem;
    for (unsigned int i = 0; i < length; i++)
        mensagem += (char)payload[i];

    JsonDocument doc;
if (deserializeJson(doc, mensagem)) return;

const char* efeito = doc["efeito"];
const char* cor = doc["cor"];
const char* from = doc["from"];
const char* to = doc["to"];
int velocidade = doc["velocidade"] | 50;


// 🔵 Se veio de mim → perdi o token
if (from && String(from) == DEVICE_ID) {
    if (instance->_effects)
        instance->_effects->cancelOverlayOnly();
    return;
}


// 🟣 Se veio do WEB → cancela local sempre
if (from && String(from) == "web") {

    if (instance->_effects)
        instance->_effects->cancelOverlayOnly();

    if (to && String(to) == DEVICE_ID && efeito) {
        instance->_effects->play(
            efeito,
            cor ? cor : "#ffffff",
            velocidade
        );
    }

    return;
}


// 🔴 Se veio do outro ESP para mim
if (to && String(to) == DEVICE_ID && efeito) {

    if (instance->_effects)
        instance->_effects->cancelOverlayOnly();

    instance->_effects->play(
        efeito,
        cor ? cor : "#ffffff",
        velocidade
    );
}

}


//Perfeiitooo, Agora next etapa:

//Implementaremos OTA, pois para mim é muito dificil ficar ir buscando a luminária dela só para fazer ajustes
