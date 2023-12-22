#pragma once

#include <FastLED.h>
#include "globalDefines.h"

class Button_Flash
{
public:
    static CRGB *leds;
    static size_t animationTicks;
    static size_t ticksRemaining;

    static void init(size_t ledIdx, uint8_t r, uint8_t g, uint8_t b);
    static void update();

private:
    static uint8_t r;
    static uint8_t g;
    static uint8_t b;

    static size_t ledIdx;
};