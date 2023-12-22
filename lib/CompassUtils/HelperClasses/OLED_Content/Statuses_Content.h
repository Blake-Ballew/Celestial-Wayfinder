#pragma once

#include "OLED_Content.h"
#include "Network_Manager.h"
#include "Navigation_Manager.h"
#include "Settings_Manager.h"
#include "LED_Manager.h"

class Statuses_Content : public OLED_Content
{
public:
    Statuses_Content(Adafruit_SSD1306 *disp);
    ~Statuses_Content();

    void printContent();
    void encUp();
    void encDown();

    void updateMessages() { printContent(); }

    Message_Base *getCurrentMessage()
    {
        if (Network_Manager::messages.size() == 0)
        {
            return nullptr;
        }
        return Network_Manager::findMessageByIdx(msgIdx);
    }

private:
    void printMessageAge(uint64_t timeDiff);

    uint16_t msgIdx = 0;
};