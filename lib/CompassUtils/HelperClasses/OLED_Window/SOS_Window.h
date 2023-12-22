#pragma once

#include "OLED_Window.h"
#include "SOS_Content.h"

class SOS_Window : public OLED_Window
{
public:
    SOS_Window(OLED_Window *parent);
    ~SOS_Window();

    void execBtnCallback(uint8_t buttonNumber, void *arg);
    void Pause();
    void Resume();

private:
    SOS_Content *sosContent;
};