#include "CallEffects.h"

void CallEffects::aspectro() {

    static unsigned long lastFrame = 0;
    static bool rising = true;
    static uint8_t brilho = 0;

    unsigned long now = millis();

    // Intervalo controlado por velocidade
    uint16_t interval = map(_velocidade, 0, 100, 25, 5);
    if (now - lastFrame < interval) return;
    lastFrame = now;

    // Converte cor recebida (_cor)
    CRGB corBase = CRGB::White;

    if (_cor.length() >= 7) {
        long number = strtol(_cor.substring(1, 7).c_str(), NULL, 16);
        corBase = CRGB(
            (number >> 16) & 0xFF,
            (number >> 8) & 0xFF,
            number & 0xFF
        );
    }

    // Controle manual de fade
    if (rising) {
        brilho += 4;
        if (brilho >= 250) rising = false;
    } else {
        brilho -= 4;
        if (brilho <= 5) rising = true;
    }

    CRGB stepColor = corBase;
    stepColor.nscale8_video(brilho);

    fill_solid(leds, NUM_LEDS, stepColor);
    FastLED.show();
}