#include "CallEffects.h"

void CallEffects::aspectro() {

    static unsigned long lastFrame = 0;
    static bool rising = true;
    static uint8_t brilho = 0;
    static uint8_t ciclo = 0;

    const uint8_t maxCiclos = 3;

    unsigned long now = millis();

    // Reset quando começa
    if (_step == 0) {
        rising = true;
        brilho = 0;
        ciclo = 0;
    }

    uint16_t interval = map(_velocidade, 0, 100, 25, 5);
    if (now - lastFrame < interval) return;
    lastFrame = now;

    // Fade manual
    if (rising) {
        brilho += 4;
        if (brilho >= 250) {
            brilho = 250;
            rising = false;
        }
    } else {
        brilho -= 4;
        if (brilho <= 5) {
            brilho = 5;
            rising = true;

            // Cada ciclo completo é quando termina uma descida
            ciclo++;

            if (ciclo >= maxCiclos) {
                ciclo = 0;
                _overlayFinished = true;
                _step = 0;
                return;
            }
        }
    }

    CRGB corBase = hexToCRGB(_cor);
    corBase.nscale8_video(brilho);

    fill_solid(leds, NUM_LEDS, corBase);

    _step++;
    _frameDirty = true;
}