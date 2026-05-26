#include "CallEffects.h"

void CallEffects::to_aqui() {

    static unsigned long lastFrame = 0;
    static uint8_t ciclo = 0;

    const uint8_t maxCiclos = 3;

    unsigned long now = millis();

    const uint16_t interval = map(_velocidade, 50, 100, 40, 10);
    if (now - lastFrame < interval) return;
    lastFrame = now;

    const uint8_t steps = 6;

    fadeToBlackBy(leds, NUM_LEDS, 40);

    CRGB cor = hexToCRGB(_cor);

    for (int j = 0; j <= steps; j++) {

        int pos = _step - j;
        if (pos >= 0 && pos < NUM_LEDS) {

            CRGB px = cor;
            uint8_t brilho = 255 - (j * (255 / steps));
            px.nscale8(brilho);
            leds[pos] += px;
        }
    }

    _step++;

    if (_step >= NUM_LEDS) {
        _step = 0;
        ciclo++;

        if (ciclo >= maxCiclos) {
            ciclo = 0;
            _overlayFinished = true;
            return;
        }
    }

    _frameDirty = true;
}