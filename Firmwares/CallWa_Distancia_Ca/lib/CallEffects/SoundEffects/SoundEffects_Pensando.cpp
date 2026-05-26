#include "CallEffects.h"

void CallEffects::pensando() {

    static unsigned long lastFrame = 0;
    static int direction = 1;   // 1 = vai, -1 = volta

    unsigned long now = millis();

    uint16_t interval = map(_velocidade, 0, 100, 40, 5);
    if (now - lastFrame < interval) return;
    lastFrame = now;

    // Limpa anterior
    fadeToBlackBy(leds, NUM_LEDS, 80);

    // Desenha ponto atual
    leds[_step] = CRGB::DeepSkyBlue;

    FastLED.show();

    // Atualiza posição
    _step += direction;

    if (_step >= NUM_LEDS - 1) {
        _step = NUM_LEDS - 1;
        direction = -1;
    }

    if (_step <= 0) {
        _step = 0;
        direction = 1;
    }
}