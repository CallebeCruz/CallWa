#pragma once
#include <FastLED.h>
#include "CallConfig.h"

class CallEffects {
public:
    void begin();
    void play(const char* nome, const String& cor = "#ffffff", int velocidade = 50);
    void update();
    void showWifiConnecting();
    void flashWifiConnected();
    void flashMqttConnected();
    void stop();
    void cancelOverlayOnly();
    
private:
    enum EffectType {
        NONE,
        PENSANDO,
        SAUDADES,
        DESCULPAS,
        TO_AQUI,
        SURPRESINHA,
        BAJULINHO,
        ASPECTRO
    };
    
    CRGB hexToCRGB(const String& hex);
    // Sistemas de Prioridade.
    EffectType _overlay = NONE;
    EffectType _overlayScheduled = NONE;

    bool _repeatEnabled = false;
    unsigned long _nextRunAt = 0;

    bool _overlayFinished = false;
    bool _frameDirty = false;
    
    bool _baseEnabled = false;
    CRGB _baseColor = CRGB::Black;


    CRGB leds[NUM_LEDS];

    unsigned long _statusClearAt = 0;
    unsigned long _flashUntil = 0;

    void startFlash(int times, CRGB color);
    int _flashCount = 0;
    int _flashTarget = 0;
    bool _flashState = false;
    CRGB _flashColor = CRGB::Black;

    // controle interno de animação (se existir)
    unsigned long _lastUpdate = 0;
    int _step = 0;

    String _cor = "#ffffff";
    int _velocidade = 50;

    // Declarações (implementadas nos arquivos separados)
    void pensando();
    void saudades();
    void desculpas();
    void to_aqui();
    void surpresinha();
    void bajulinho();
    void aspectro();
};