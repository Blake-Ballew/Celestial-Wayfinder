#pragma once

#include "OLED_Content.h"
#include "Navigation_Manager.h"
#include "Settings_Manager.h"
#include "Network_Manager.h"
#include "System_Utils.h"

const uint8_t HOME_CONTENT_MAIN = 1;
const uint8_t HOME_CONTENT_MESSAGES = 2;

class Home_Content : public OLED_Content
{
public:
    uint8_t contentMode;

    Home_Content(Adafruit_SSD1306 *display);
    ~Home_Content();

    void printContent();
    void encUp();
    void encDown();

    void Pause();
    void Resume();

    static void timerCallback(TimerHandle_t xTimer);

    Message_Base *getCurrentMessage();

private:
    static Home_Content *thisInstance;
    TimerHandle_t timer;

    std::map<uint64_t, Message_Base *>::iterator msgIterator;
};