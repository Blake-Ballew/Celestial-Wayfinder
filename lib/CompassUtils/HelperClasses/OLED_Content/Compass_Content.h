#pragma once

#include "Navigation_Manager.h"
#include "OLED_Content.h"
// #include "QMC5883LCompass.h"
#include "LED_Manager.h"

class Compass_Content : public OLED_Content
{
public:
    Compass_Content(Adafruit_SSD1306 *disp);
    ~Compass_Content();

    void printContent();
    void encUp();
    void encDown();

    void Pause();
    void Resume();

private:
    static TimerHandle_t updateTimer;
    static StaticTimer_t updateTimerBuffer;
    static Compass_Content *thisInstance;

    static void updateCompass(TimerHandle_t xTimer);
};