#include "Home_Content.h"

Home_Content *Home_Content::thisInstance = nullptr;

Home_Content::Home_Content(Adafruit_SSD1306 *display)
{
    this->type = ContentType::HOME;
    this->display = display;
    this->contentMode = 1;
    thisInstance = this;
    timer = xTimerCreate("Home_Content_Timer", pdMS_TO_TICKS(60000), pdTRUE, (void *)0, Home_Content::timerCallback);
    xTimerStart(this->timer, 0);
}

Home_Content::~Home_Content()
{
}

void Home_Content::printContent()
{
    if (contentMode == HOME_CONTENT_MAIN)
    {
        Navigation_Manager::updateGPS();
        Navigation_Manager::read();
        // TinyGPSDate date;
        TinyGPSTime time = Navigation_Manager::getTime();
        if (time.isValid())
        {
            // Adjust for timezone -4
            int hour = time.hour();
            if (hour < 4)
            {
                hour += 20;
            }
            else
            {
                hour -= 4;
            }

            if (Settings_Manager::settings["Device"]["24HTime"].as<bool>())
            {
                display->setCursor(OLED_Content::alignTextRight(5), OLED_Content::selectTextLine(2));
                display->printf("%02d:%02d", hour, time.minute());
            }
            else
            {
                display->setCursor(OLED_Content::alignTextRight(8), OLED_Content::selectTextLine(2));
                display->printf("%02d:%02d %s", hour % 12, time.minute(), hour < 12 ? "AM" : "PM");
            }
        }
        else
        {
            display->setCursor(OLED_Content::alignTextRight(6), OLED_Content::selectTextLine(2));
            display->printf("No GPS");
        }

        OLED_Content::drawBatteryIcon(OLED_Content::alignTextLeft(0), OLED_Content::selectTextLine(2), System_Utils::getBatteryPercentage());

        size_t unreadMsgs = Network_Manager::getNumUnreadMessages();

        /*display->setCursor(OLED_Content::alignTextLeft(), OLED_Content::selectTextLine(3));
        display->printf("Unread: %d", unreadMsgs);*/

        OLED_Content::drawBellIcon(OLED_Content::alignTextLeft(3), OLED_Content::selectTextLine(2), System_Utils::silentMode);

        OLED_Content::drawMessageIcon(OLED_Content::alignTextLeft(6), OLED_Content::selectTextLine(2));
        display->setCursor(OLED_Content::alignTextLeft(8), OLED_Content::selectTextLine(2));
        display->printf(":%d", unreadMsgs);

        if (unreadMsgs > 0 ) {
            display->drawLine(OLED_WIDTH / 2, OLED_HEIGHT - 1, (OLED_WIDTH / 2) - 3, OLED_HEIGHT - 3, WHITE);
            display->drawLine((OLED_WIDTH / 2) + 1, OLED_HEIGHT - 1, (OLED_WIDTH / 2) + 4, OLED_HEIGHT - 3, WHITE);
            display->setCursor(OLED_Content::alignTextLeft(6), OLED_Content::selectTextLine(4));
            display->print("Msgs");
        }
    }
    else if (contentMode == HOME_CONTENT_MESSAGES)
    {
        Message_Base *msg = msgIterator->second;
        msg->displayMessage(display);
    }
    display->display();
}

void Home_Content::Pause()
{
    xTimerStop(this->timer, 0);
}

void Home_Content::Resume()
{
    xTimerStart(this->timer, 0);
}

void Home_Content::timerCallback(TimerHandle_t xTimer)
{
    if (thisInstance->contentMode == HOME_CONTENT_MAIN)
    {
        thisInstance->printContent();
    }
}

Message_Base *Home_Content::getCurrentMessage()
{
    if (contentMode == HOME_CONTENT_MESSAGES)
    {
        return msgIterator->second;
    }
    return nullptr;
}

void Home_Content::encUp()
{
    switch (contentMode)
    {
    case HOME_CONTENT_MESSAGES:
    {
        auto newIt = Network_Manager::decrementUnreadIterator(msgIterator);
        if (newIt != Network_Manager::getEndIterator())
        {
            msgIterator = newIt;
        }
        else
        {
            LED_Manager::clearRing();
            contentMode = HOME_CONTENT_MAIN;
        }
        break;
    }
    }
}

void Home_Content::encDown()
{
    switch (contentMode)
    {
    case HOME_CONTENT_MAIN:
    {
        if (Network_Manager::getNumUnreadMessages() > 0)
        {
            contentMode = HOME_CONTENT_MESSAGES;
            msgIterator = Network_Manager::getUnreadBegin();
        }
        break;
    }
    case HOME_CONTENT_MESSAGES:
    {
        auto newIt = Network_Manager::incrementUnreadIterator(msgIterator);
        if (newIt != Network_Manager::getEndIterator())
        {
            msgIterator = newIt;
        }
        break;
    }
    }
}