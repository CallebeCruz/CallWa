#include "CallEffects.h"

void CallEffects::to_aqui() {

    static unsigned long lastFrame = 0;
    unsigned long now = millis();

    // ===== MODO 1 — RESPIRAÇÃO =====
    if (_velocidade < 50) {

        const uint16_t interval = 15;
        if (now - lastFrame < interval) return;
        lastFrame = now;

        uint8_t brilho = beatsin8(8, 10, 255);

        fill_solid(leds, NUM_LEDS, CHSV(200, 255, brilho));
        FastLED.show();
        return;
    }

    // ===== MODO 2 — RASTRO =====

    const uint16_t interval = map(_velocidade, 50, 100, 40, 10);
    if (now - lastFrame < interval) return;
    lastFrame = now;

    const uint8_t steps = 6;

    // Fade geral
    fadeToBlackBy(leds, NUM_LEDS, 40);

    // Desenha rastro
    for (int j = 0; j <= steps; j++) {

        int pos = _step - j;
        if (pos >= 0 && pos < NUM_LEDS) {

            uint8_t brilho = 255 - (j * (255 / steps));
            leds[pos] = CHSV(200, 255, brilho);
        }
    }

    FastLED.show();

    _step++;
    if (_step >= NUM_LEDS) {
        _step = 0;
    }
}