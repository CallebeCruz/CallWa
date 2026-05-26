#include "CallEffects.h"

void CallEffects::bajulinho() {

    static int turn = 0;
    static int col = 0;
    static unsigned long lastFrame = 0;

    const uint8_t trailCols  = 6;
    const uint8_t trailTurns = 3;
    const uint8_t fadeBase   = 180;

    const uint16_t frameInterval = map(_velocidade, 0, 100, 35, 15);

    unsigned long now = millis();
    if (now - lastFrame < frameInterval) return;
    lastFrame = now;

    // ==== GEOMETRIA CALCULADA AQUI ====
    const int numTurns = (NUM_LEDS + STEPS_PER_TURN - 1) / STEPS_PER_TURN;
    const int lastTurnCount = NUM_LEDS - ((numTurns - 1) * STEPS_PER_TURN);

    auto idxFromTurnCol = [&](int t, int c) -> int {

        if (t < 0 || t >= numTurns) return -1;

        int colsThisTurn = (t == numTurns - 1)
                           ? (lastTurnCount > 0 ? lastTurnCount : STEPS_PER_TURN)
                           : STEPS_PER_TURN;

        c = (c % colsThisTurn + colsThisTurn) % colsThisTurn;

        int base = t * STEPS_PER_TURN;
        int idx  = base + c;

        return (idx >= 0 && idx < NUM_LEDS) ? idx : -1;
    };

    // Fade geral
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i].nscale8(fadeBase);

    CRGB cor = CRGB::HotPink;

    for (uint8_t v = 0; v < trailTurns; v++) {

        int tActual = turn - v;
        if (tActual < 0) break;

        int colsThisTurn = (tActual == numTurns - 1)
                           ? (lastTurnCount > 0 ? lastTurnCount : STEPS_PER_TURN)
                           : STEPS_PER_TURN;

        for (uint8_t t = 0; t < trailCols; t++) {

            int c1 = col - t;
            c1 = (c1 % colsThisTurn + colsThisTurn) % colsThisTurn;

            int idx = idxFromTurnCol(tActual, c1);

            if (idx >= 0) {

                CRGB px = cor;

                uint8_t att = map(
                    v * trailCols + t,
                    0,
                    trailCols * trailTurns - 1,
                    220,
                    20
                );

                px.fadeLightBy(255 - att);
                leds[idx] += px;
            }
        }
    }

    FastLED.show();

    int colsThisTurn = (turn == numTurns - 1)
                       ? (lastTurnCount > 0 ? lastTurnCount : STEPS_PER_TURN)
                       : STEPS_PER_TURN;

    col++;
    if (col >= colsThisTurn) {
        col = 0;
        turn++;
        if (turn >= numTurns) {
            turn = 0;
        }
    }
}