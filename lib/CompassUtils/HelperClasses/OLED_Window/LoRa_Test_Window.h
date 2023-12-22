#pragma once

#include "OLED_Window.h"
#include "LoRa_Test_Content.h"

class LoRa_Test_Window : public OLED_Window
{
public:
    LoRa_Test_Window(OLED_Window *parent);

    void execBtnCallback(uint8_t buttonNumber, void *arg);
};