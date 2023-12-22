#pragma once

#include "OLED_Window.h"
#include "Navigation_Manager.h"
#include "QMC5883LCompass.h"
#include "Compass_Content.h"
#include "LED_Manager.h"

class Compass_Window : public OLED_Window
{
public:
    Compass_Window(OLED_Window *parent);
    ~Compass_Window();

    void execBtnCallback(uint8_t buttonNumber, void *arg);

    void Pause();
    void Resume();

private:
    Compass_Content *compassContent;
};