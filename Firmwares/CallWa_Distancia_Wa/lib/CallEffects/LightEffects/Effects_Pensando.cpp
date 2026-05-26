#include "CallEffects.h"

void CallEffects::pensando() {

    static unsigned long lastFrame = 0;
    static int direction = 1;
    static uint8_t ciclo = 0;

    const uint8_t maxCiclos = 3;

    unsigned long now = millis();

    uint16_t interval = map(_velocidade, 0, 100, 40, 5);
    if (now - lastFrame < interval) return;
    lastFrame = now;

    fadeToBlackBy(leds, NUM_LEDS, 80);

    CRGB cor = hexToCRGB(_cor);
    leds[_step] = cor;

    _step += direction;

    if (_step >= NUM_LEDS - 1) {
        _step = NUM_LEDS - 1;
        direction = -1;
    }

    if (_step <= 0) {
        _step = 0;
        direction = 1;

        ciclo++;

        if (ciclo >= maxCiclos) {
            ciclo = 0;
            direction = 1;
            _overlayFinished = true;
            return;
        }
    }

    _frameDirty = true;
}