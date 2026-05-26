#include "CallEffects.h"

void CallEffects::saudades() {

    const uint16_t atk1 = 130;
    const uint16_t dec1 = 190;
    const uint16_t pausa1 = 70;

    const uint16_t atk2 = 100;
    const uint16_t dec2 = 230;
    const uint16_t pausa2 = 360;

    static uint8_t phase = 0;     // 0=atk1,1=dec1,2=pausa1,3=atk2,4=dec2,5=pausa2
    static unsigned long phaseStart = 0;
    static uint16_t phaseDuration = atk1;

    unsigned long now = millis();

    if (phaseStart == 0) {
        phaseStart = now;
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
                break;
        }
    }

    uint8_t brilho = 0;

    if (phase == 0) { // ataque 1
        brilho = map(elapsed, 0, atk1, 20, 255);
    }
    else if (phase == 1) { // decay 1
        brilho = map(elapsed, 0, dec1, 255, 20);
    }
    else if (phase == 3) { // ataque 2
        brilho = map(elapsed, 0, atk2, 20, 200);
    }
    else if (phase == 4) { // decay 2
        brilho = map(elapsed, 0, dec2, 200, 20);
    }
    else {
        brilho = 20; // pausas
    }

    fill_solid(leds, NUM_LEDS, CHSV(0, 240, brilho));
    FastLED.show();
}