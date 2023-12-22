#pragma once

#include "OLED_Content.h"
#include "Network_Manager.h"
#include "Navigation_Manager.h"
#include "Settings_Manager.h"
#include "LED_Manager.h"
#include "Message_Types.h"

class Tracking_Content : public OLED_Content
{
public:
    Tracking_Content(Adafruit_SSD1306 *disp)
    {
        display = disp;
        type = ContentType::TRACKING;
        Tracking_Content::thisInstance = this;
    }

    ~Tracking_Content()
    {
        thisInstance = nullptr;
    }

    void printContent()
    {
        if (currMsg == nullptr)
        {
            return;
        }

        Navigation_Manager::read();

        char statusTxt[STATUS_LENGTH];
        currMsg->toString(statusTxt, STATUS_LENGTH);
        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

        display->setCursor(OLED_Content::centerTextHorizontal(statusTxt), 16);
        display->print(statusTxt);

        display->setCursor(0, 8);
        display->print(currMsg->senderName);

        if (currMsg->msgType == MessageType::MESSAGE_PING)
        {
            Message_Ping *msg = (Message_Ping *)currMsg;
            double distance = Navigation_Manager::getDistanceTo(msg->lat, msg->lng);
            if (distance < 2000)
            {
                display->setCursor(OLED_WIDTH - 42, 8);
                display->print((uint32_t)distance);
                display->print("m");
            }
            else
            {
                display->setCursor(OLED_WIDTH - 54, 24);
                display->print(distance / 1000);
                display->print("km");
            }

            double heading = Navigation_Manager::getHeadingTo(msg->lat, msg->lng);
            LED_Manager::pointToHeading(Navigation_Manager::InvertXAzimuth(Navigation_Manager::getAzimuth()),
                                        heading,
                                        distance,
                                        msg->color_R,
                                        msg->color_G,
                                        msg->color_B);

            display->display();
        }
    }
    void encUp() {}
    void encDown() {}

    void assignMsg(Message_Base *msg)
    {
        currMsg = msg;
        thisInstance = this;
    }

    void unassignMsg()
    {
        currMsg = nullptr;
        thisInstance = nullptr;
    }

    void stop()
    {
        if (updateTimer != NULL)
        {
            xTimerStop(updateTimer, 0);
        }
    }

    void start()
    {
        if (updateTimer != NULL)
        {
            xTimerStart(updateTimer, 0);
        }
    }

    static Tracking_Content *thisInstance;

private:
    static void updateDisplay(TimerHandle_t xTimer)
    {
        if (thisInstance == nullptr)
        {
            xTimerStop(updateTimer, 0);
            return;
        }
        Tracking_Content::thisInstance->printContent();
    }

    static TimerHandle_t updateTimer;
    static StaticTimer_t updateTimerBuffer;
    Message_Base *currMsg;
};
