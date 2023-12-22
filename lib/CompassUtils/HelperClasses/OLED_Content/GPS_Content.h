#pragma once

#include "Navigation_Manager.h"
#include "OLED_Content.h"

class GPS_Content : public OLED_Content
{
public:
    GPS_Content(Adafruit_SSD1306 *disp);
    ~GPS_Content();

    void printContent();
    void encUp();
    void encDown();

private:
};