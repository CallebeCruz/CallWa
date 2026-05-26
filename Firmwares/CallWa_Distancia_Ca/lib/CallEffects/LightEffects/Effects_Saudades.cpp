#include "CallEffects.h"

void CallEffects::saudades() {

    const uint16_t atk1 = 130;
    const uint16_t dec1 = 190;
    const uint16_t pausa1 = 70;

    const uint16_t atk2 = 100;
    const uint16_t dec2 = 230;
    const uint16_t pausa2 = 360;

    const uint8_t maxCiclos = 3;

    static uint8_t phase = 0;
    static unsigned long phaseStart = 0;
    static uint16_t phaseDuration = atk1;
    static uint8_t ciclo = 0;

    unsigned long now = millis();

    // Reset real quando overlay acabou de iniciar
    if (_step == 0 && phaseStart == 0) {
        phase = 0;
        phaseStart = now;
        phaseDuration = atk1;
        ciclo = 0;
    }

    unsigned long elapsed = now - phaseStart;

    if (elapsed >= phaseDuration) {
        phase++;
        phaseStart = now;

        switch (phase) {
            case 1: phaseDuration = dec1; break;
            case 2: phaseDuration = pausa1; break;
            case 3: phaseDuration = atk2; break;
            case 4: phaseDuration = dec2; break;
            case 5: phaseDuration = pausa2; break;

            default:
                phase = 0;
                phaseDuration = atk1;
                ciclo++;

                if (ciclo >= maxCiclos) {
                    ciclo = 0;
                    phaseStart = 0;   // prepara para próximo início limpo
                    _overlayFinished = true;
                    _step = 0;
                    return;
                }
                break;
        }
    }

    uint8_t brilho = 0;

    if (phase == 0) brilho = map(elapsed, 0, atk1, 20, 255);
    else if (phase == 1) brilho = map(elapsed, 0, dec1, 255, 20);
    else if (phase == 3) brilho = map(elapsed, 0, atk2, 20, 200);
    else if (phase == 4) brilho = map(elapsed, 0, dec2, 200, 20);
    else brilho = 20;

    CRGB cor = hexToCRGB(_cor);
    cor.nscale8(brilho);

    fill_solid(leds, NUM_LEDS, cor);

    _step++;
    _frameDirty = true;
}