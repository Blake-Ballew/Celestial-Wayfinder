#pragma once

#include <FastLED.h>
#include "globalDefines.h"
#include "Button_Flash.h"
#include "Settings_Manager.h"

#define NUM_COMPASS_LEDS 16

/*
LED Mappings:
0-15: compass ring
16: SOS button
17: Button 4
18: Button 3
19: Button 2
20: Encoder up
21: Encoder down
22: Button 1
23-29: Flashlight
*/

class LED_Manager
{
public:
    static void init();
    static CRGB leds[NUM_LEDS];

    static void pointNorth(int Azimuth);
    static void pointToHeading(int Azimuth, double heading, double distanceAway, uint8_t r, uint8_t g, uint8_t b);
    static void lightRing(uint8_t r, uint8_t g, uint8_t b);
    static void clearRing();
    static void toggleFlashlight();
    static void buzzerNotification(uint16_t frequency = DEFAULT_TONE, size_t duration = 100);
    static void ledShutdownAnimation();

    static void pulseButton(uint8_t buttonNumber);

    static void pulseCircle(uint8_t r, uint8_t g, uint8_t b, size_t tick);

private:
    static bool flashlightOn;
    static TimerHandle_t patternTimer;
    static StaticTimer_t patternTimerBuffer;

    static uint8_t r, g, b;

    static void updatePattern(TimerHandle_t xTimer);

    static void interpolateLEDsDegrees(double deg, double distanceAway, uint8_t r, uint8_t g, uint8_t b);
};