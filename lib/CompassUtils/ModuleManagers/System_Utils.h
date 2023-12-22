#pragma once

#include "globalDefines.h"
#include <Arduino.h>
#include "LED_Manager.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
//#include "OLED_Manager.h"

class System_Utils
{
public:
    static bool silentMode;
    static Adafruit_SSD1306 *display;

    static void init(Adafruit_SSD1306 *display);
    static long getBatteryPercentage();
    static void monitorSystemHealth(TimerHandle_t xTimer);
    static void shutdownBatteryWarning();

private:
    static TimerHandle_t healthTimer;
    static StaticTimer_t healthTimerBuffer;
};