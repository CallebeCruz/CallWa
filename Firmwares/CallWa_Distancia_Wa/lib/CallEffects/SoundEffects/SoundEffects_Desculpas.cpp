#include "CallEffects.h"

void CallEffects::desculpas() {

    if (millis() - _lastUpdate < _velocidade) return;
    _lastUpdate = millis();

    float phase = (_step / 50.0f) * TWO_PI;
    float wave = (sin(phase) + 1.0f) * 0.5f;

    CRGB azul = CRGB(0, 80, 255);
    CRGB branco = CRGB::White;

    CRGB mistura = blend(azul, branco, wave * 255);

    fill_solid(leds, NUM_LEDS, mistura);
    FastLED.show();

    _step++;

    if (_step >= 50) {
        _overlayFinished = true;
    }
}