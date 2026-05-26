#include "CallEffects.h"

void CallEffects::begin() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();
}

void CallEffects::stop() {
    _overlay = NONE;
    _overlayScheduled = NONE;
    _repeatEnabled = false;

    _flashCount = 0;
    _flashTarget = 0;

    FastLED.clear();
    FastLED.show();
}

void CallEffects::showWifiConnecting() {
    fill_solid(leds, NUM_LEDS, CRGB(0,255,255));
    FastLED.show();
}

void CallEffects::startFlash(int times, CRGB color) {
    _flashTarget = times * 2;
    _flashCount = 0;
    _flashColor = color;
    _flashState = false;
    _flashUntil = millis();
}

void CallEffects::flashWifiConnected() {
    startFlash(1, CRGB(0,255,120));
}

void CallEffects::flashMqttConnected() {
    startFlash(3, CRGB(0,255,120));
}

CRGB CallEffects::hexToCRGB(const String& hex) {
    if (hex.length() < 7) return CRGB::White;

    long number = strtol(hex.substring(1).c_str(), nullptr, 16);

    return CRGB(
        (number >> 16) & 0xFF,
        (number >> 8) & 0xFF,
        number & 0xFF
    );
}

void CallEffects::play(const char* nome, const String& cor, int velocidade) {

    _cor = cor;
    _velocidade = velocidade;
    _step = 0;
    _lastUpdate = 0;
    _overlayFinished = false;

    // ESTÁTICO
    if (strcmp(nome, "estatico") == 0) {
        _baseEnabled = true;
        _baseColor = hexToCRGB(cor);
        return;
    }

    // DESLIGAR
    if (strcmp(nome, "desligar") == 0) {
        _baseEnabled = false;
        cancelOverlayOnly();
        FastLED.clear();
        FastLED.show();
        return;
    }

    // EFEITOS EMOCIONAIS
    if (strcmp(nome, "pensando") == 0) _overlay = PENSANDO;
    else if (strcmp(nome, "saudades") == 0) _overlay = SAUDADES;
    else if (strcmp(nome, "desculpas") == 0) _overlay = DESCULPAS;
    else if (strcmp(nome, "to_aqui") == 0) _overlay = TO_AQUI;
    else if (strcmp(nome, "surpresinha") == 0) _overlay = SURPRESINHA;
    else if (strcmp(nome, "bajulinho") == 0) _overlay = BAJULINHO;
    else if (strcmp(nome, "aspectro") == 0) _overlay = ASPECTRO;
    else return;

    _overlayScheduled = _overlay;
    _repeatEnabled = true;

    // agenda próxima execução temporal
    _nextRunAt = millis() + EFFECT_REPEAT_INTERVAL_MS;
}

void CallEffects::update() {

    // PRIORIDADE 1: Flash
    if (_flashCount < _flashTarget) {
        if (millis() > _flashUntil) {

            _flashState = !_flashState;

            if (_flashState)
                fill_solid(leds, NUM_LEDS, _flashColor);
            else
                FastLED.clear();

            FastLED.show();

            _flashUntil = millis() + 120;
            _flashCount++;
        }
        return;
    }

    // PRIORIDADE 2: Overlay ativo
    if (_overlay != NONE) {

        switch (_overlay) {
            case PENSANDO: pensando(); break;
            case SAUDADES: saudades(); break;
            case DESCULPAS: desculpas(); break;
            case TO_AQUI: to_aqui(); break;
            case SURPRESINHA: surpresinha(); break;
            case BAJULINHO: bajulinho(); break;
            case ASPECTRO: aspectro(); break;
            default: break;
        }

        if (_overlayFinished) {

            _overlay = NONE;
            _overlayFinished = false;

            if (!_baseEnabled) {
                FastLED.clear();
                FastLED.show();
            }
        }

        if (_frameDirty) {
            FastLED.show();
            _frameDirty = false;
        }

        return;
    }

    // PRIORIDADE 3: Repetição temporal
    if (_repeatEnabled && millis() >= _nextRunAt) {

        _overlay = _overlayScheduled;
        _overlayFinished = false;
        _step = 0;
        _lastUpdate = 0;

        _nextRunAt = millis() + EFFECT_REPEAT_INTERVAL_MS;
        return;
    }

    // PRIORIDADE 4: Base estática
    if (_baseEnabled) {
        fill_solid(leds, NUM_LEDS, _baseColor);
        FastLED.show();
    }
}

void CallEffects::cancelOverlayOnly() {

    _overlay = NONE;
    _overlayScheduled = NONE;
    _repeatEnabled = false;
    _overlayFinished = false;
    _nextRunAt = 0;

    if (_baseEnabled) {
        fill_solid(leds, NUM_LEDS, _baseColor);
    } else {
        FastLED.clear();
    }

    FastLED.show();
}