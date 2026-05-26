#include "CallEffects.h"

#include "CallEffects.h"

void CallEffects::surpresinha() {

    static uint8_t heat[NUM_LEDS];
    static unsigned long lastFrame = 0;
    static uint16_t frameCount = 0;
    static uint8_t ciclo = 0;

    const uint8_t maxCiclos = 3;

    const uint8_t cooling  = map(_velocidade, 0, 100, 20, 55);
    const uint8_t sparking = map(_velocidade, 0, 100, 60, 200);
    const uint8_t glitterTh = map(_velocidade, 0, 100, 3, 14);

    const uint16_t frameInterval = 20;

    unsigned long now = millis();
    if (now - lastFrame < frameInterval) return;
    lastFrame = now;

    // 1️⃣ Cooling
    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t cooldown = random8(0, ((cooling * 10) / NUM_LEDS) + 2);
        heat[i] = qsub8(heat[i], cooldown);
    }

    // 2️⃣ Heat diffusion upward
    for (int k = NUM_LEDS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // 3️⃣ Sparking
    if (random8() < sparking) {
        int y = random8(0, min(7, NUM_LEDS));
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // 4️⃣ Render
    for (int j = 0; j < NUM_LEDS; j++) {
        uint8_t colorIndex = scale8(heat[j], 240);
        leds[j] = ColorFromPalette(HeatColors_p, colorIndex);
    }

    // 5️⃣ Glitter
    if (random8() < glitterTh) {
        int p = random16(NUM_LEDS);
        leds[p] += CRGB(80, 60, 20);
    }

    frameCount++;

    // define ciclo por tempo (ex: ~5s)
    if (frameCount >= 250) {
        frameCount = 0;
        ciclo++;

        if (ciclo >= maxCiclos) {
            ciclo = 0;
            _overlayFinished = true;
            return;
        }
    }

    _frameDirty = true;
}