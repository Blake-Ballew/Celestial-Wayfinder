#include "Button_Flash.h"

CRGB *Button_Flash::leds = nullptr;
size_t Button_Flash::animationTicks = 15;
size_t Button_Flash::ticksRemaining = 0;

uint8_t Button_Flash::r = 0;
uint8_t Button_Flash::g = 0;
uint8_t Button_Flash::b = 0;

size_t Button_Flash::ledIdx = 0;

void Button_Flash::init(size_t ledIdx, uint8_t r, uint8_t g, uint8_t b)
{

#if DEBUG == 1
    Serial.print("Button_Flash::init() called with ledIdx: ");
    Serial.println(ledIdx);
#endif
    Button_Flash::ledIdx = ledIdx;
    Button_Flash::r = r;
    Button_Flash::g = g;
    Button_Flash::b = b;

    ticksRemaining = animationTicks;
}

void Button_Flash::update()
{
    if (ticksRemaining == 0)
    {
        return;
    }

    ticksRemaining--;
    float brightness = float(ticksRemaining) / float(animationTicks);

    leds[ledIdx] = CRGB(r * brightness, g * brightness, b * brightness);
    FastLED.show();
}