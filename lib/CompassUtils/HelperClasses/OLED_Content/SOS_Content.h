#pragma once

#include "OLED_Content.h"
#include "Navigation_Manager.h"
#include "Network_Manager.h"
#include "LED_Manager.h"
#include "Message_Types.h"

#define SOS_CONTENT_TIMER_TICK 17
#define SOS_CONTENT_TICKS_PER_MESSAGE 1800

namespace
{
    const char *SOS_CONTENT_MESSAGE PROGMEM = "SOS";
    const char *OK_CONTENT_MESSAGE PROGMEM = "OK";
}

class SOS_Content : OLED_Content
{
public:
    bool confirmed;

    SOS_Content(Adafruit_SSD1306 *disp);
    ~SOS_Content();

    void printContent();
    static void updateDisplay(TimerHandle_t timer);

    void Pause();
    void Resume();

    void encUp() {}
    void encDown() {}

    void confirmSOS();
    void unconfirmSOS();

private:
    void sendSOS();
    void sendOkay();

    static size_t currentTick;
    uint32_t msgID;

    static SOS_Content *thisInstance;
    static StaticTimer_t updateTimerBuffer;
    static TimerHandle_t updateTimer;
};