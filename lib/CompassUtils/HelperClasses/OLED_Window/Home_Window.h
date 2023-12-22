#pragma once

#include "OLED_Window.h"
#include "Home_Content.h"
#include "Tracking_Content.h"
#include "Ping_Content.h"

class Home_Window : public OLED_Window
{
public:
    Home_Window(OLED_Window *parent);
    Home_Window();

    void Pause();
    void Resume();

    void encUp();
    void encDown();

    void drawWindow();

    // void drawWindow();
    void execBtnCallback(uint8_t buttonNumber, void *arg);

private:
    Home_Content *homeContent;
    Tracking_Content *trackingContent;
    Ping_Content *pingContent;

    void swapToTracking(Message_Base *msg);
    void swapToPing(Message_Base *msg);
    void swapToHome();

    void homeContent1();
    void homeContent2();
};