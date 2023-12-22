#include "Statuses_Window.h"

Statuses_Window::Statuses_Window(OLED_Window *parent) : OLED_Window(parent)
{
    this->statusContent = new Statuses_Content(this->display);
    this->trackingContent = new Tracking_Content(this->display);
    this->pingContent = new Ping_Content(this->display, nullptr);
    this->content = statusContent;

    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "Track", 5);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
    assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Direct Msg", 10);
}

Statuses_Window::~Statuses_Window()
{
#if DEBUG == 1
    Serial.println("Statuses_Window::~Statuses_Window()");
#endif
    if (statusContent != nullptr)
    {
        delete statusContent;
        statusContent = nullptr;
    }
    if (trackingContent != nullptr)
    {
        delete trackingContent;
        trackingContent = nullptr;
    }
    if (pingContent != nullptr)
    {
        delete pingContent;
        pingContent = nullptr;
    }
    content = nullptr;
}

void Statuses_Window::execBtnCallback(uint8_t buttonNumber, void *arg)
{
    uint8_t callbackID;
    switch (buttonNumber)
    {
        // Track button
    case BUTTON_1:
        callbackID = btn1CallbackID;
        if (content->type == ContentType::STATUS)
        {
#if DEBUG == 1
            Serial.println("Swapping to tracking content");
#endif
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "\0", 1);

            LED_Manager::clearRing();

            content = trackingContent;
            trackingContent->assignMsg(statusContent->getCurrentMessage());
            trackingContent->start();
            // this->drawWindow();
        }
        break;
        // Broadcast button
    case BUTTON_2:
    {
        callbackID = btn2CallbackID;
        if (content->type == ContentType::STATUS)
        {
#if DEBUG == 1
            Serial.println("Swapping to broadcast content");
#endif
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Send", 4);

            LED_Manager::clearRing();

            content = pingContent;
            pingContent->unassignMsg();
            // this->drawWindow();
        }
        break;
    }
        // Back button
    case BUTTON_3:
        callbackID = btn3CallbackID;
        if (content->type == ContentType::PING)
        {
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "Track", 5);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
            assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Direct Msg", 10);

            pingContent->unassignMsg();
            content = statusContent;
            LED_Manager::clearRing();
            // this->drawWindow();
        }
        if (content->type == ContentType::TRACKING)
        {
#if DEBUG == 1
            Serial.println("Swapping back from tracking content");
#endif
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "Track", 5);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
            assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Direct Msg", 10);

            trackingContent->stop();
            trackingContent->unassignMsg();
            LED_Manager::clearRing();
            content = statusContent;
            // this->drawWindow();
        }
        break;
        // Direct Msg button
    case BUTTON_4:
#if DEBUG == 1
        Serial.println("Btn 4 pressed");
#endif
        callbackID = btn4CallbackID;
        if (content->type == ContentType::STATUS)
        {
            if (statusContent->getCurrentMessage() == nullptr)
            {
                break;
            }

            LED_Manager::clearRing();

            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Send", 4);

            content = pingContent;
            pingContent->assignMsg(statusContent->getCurrentMessage());
            // this->drawWindow();
        }
        else if (content->type == ContentType::PING)
        {
            pingContent->sendPing();

            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "Track", 5);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
            assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Direct Msg", 10);

            pingContent->unassignMsg();
            content = statusContent;
            LED_Manager::clearRing();
            // this->drawWindow();
        }
        break;
    default:
        break;
    }

    switch (callbackID)
    {
    case ACTION_BACK:

        break;
    default:
        break;
    }
}