#include "Home_Window.h"

Home_Window::Home_Window(OLED_Window *parent) : OLED_Window(parent)
{
    this->content = new Home_Content(display);
    homeContent = (Home_Content *)this->content;
    pingContent = new Ping_Content(display, nullptr);
    trackingContent = new Tracking_Content(display);

    assignButton(ACTION_GENERATE_QUICK_ACTION_MENU, BUTTON_1, "Actions", 7);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
    assignButton(ACTION_NONE, BUTTON_3, "\0", 1);
    assignButton(ACTION_GENERATE_MENU_WINDOW, BUTTON_4, "Main Menu", 9);
}

Home_Window::Home_Window() : OLED_Window()
{
    this->content = new Home_Content(display);
    homeContent = (Home_Content *)this->content;
    pingContent = new Ping_Content(display, nullptr);
    trackingContent = new Tracking_Content(display);

    assignButton(ACTION_GENERATE_QUICK_ACTION_MENU, BUTTON_1, "Actions", 7);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
    assignButton(ACTION_NONE, BUTTON_3, "\0", 1);
    assignButton(ACTION_GENERATE_MENU_WINDOW, BUTTON_4, "Main Menu", 9);

    System_Utils::monitorSystemHealth(nullptr);
}

void Home_Window::Pause()
{
    homeContent->Pause();
}

void Home_Window::Resume()
{
    homeContent->Resume();
}

void Home_Window::encUp()
{
    content->encUp();

    if (content->type == ContentType::HOME)
    {
        // Change button actions based on content mode
        if (homeContent->contentMode == 1)
        {
            homeContent1();
        }
        else if (homeContent->contentMode == 2)
        {
            homeContent2();
        }
    }
}

void Home_Window::encDown()
{
    content->encDown();

    if (content->type == ContentType::HOME)
    {
        // Change button actions based on content mode
        if (homeContent->contentMode == 1)
        {
            homeContent1();
        }
        else if (homeContent->contentMode == 2)
        {
            homeContent2();
        }
    }
}

void Home_Window::drawWindow()
{
#if DEBUG == 1
    Serial.println("Home_Window::drawWindow()");
#endif
    OLED_Window::drawWindow();
    // homeContent->printContent();
}

void Home_Window::execBtnCallback(uint8_t buttonNumber, void *arg)
{
    switch (buttonNumber)
    {
    case BUTTON_1:
    {
        if (content->type == ContentType::HOME)
        {
            if (homeContent->contentMode == HOME_CONTENT_MESSAGES)
            {
                swapToTracking(homeContent->getCurrentMessage());
            }
        }
    }
    break;
    case BUTTON_2:
    {
        if (content->type == ContentType::HOME)
        {
            if (homeContent->contentMode == HOME_CONTENT_MAIN)
            {
                swapToPing(nullptr);
            }
            else if (homeContent->contentMode == HOME_CONTENT_MESSAGES)
            {
                // Mark message read

                Message_Base *msg = homeContent->getCurrentMessage();
                if (msg != nullptr)
                {
                    msg->messageOpened = true;
                    encUp();
                }
            }
        }
    }
    break;
    case BUTTON_3:
    {
        if (content->type == ContentType::PING)
        {
            swapToHome();
        }

        if (content->type == ContentType::TRACKING)
        {
            swapToHome();
        }
    }
    break;
    case BUTTON_4:
    {
        if (content->type == ContentType::PING)
        {
#if DEBUG == 1
            Serial.println("Sending ping");
#endif
            pingContent->sendPing();
#if DEBUG == 1
            Serial.println("Ping sent");
#endif
            pingContent->unassignMsg();
            swapToHome();
        }

        if (content->type == ContentType::HOME)
        {
            if (homeContent->contentMode == HOME_CONTENT_MESSAGES)
            {
                Message_Base *msg = homeContent->getCurrentMessage();
                if (msg != nullptr)
                {
                    swapToPing(msg);
                }
            }
        }
    }
    break;
    default:
        break;
    }
}

void Home_Window::swapToTracking(Message_Base *msg)
{
    if (content->type == ContentType::TRACKING)
    {
        return;
    }

    if (content->type == ContentType::HOME)
    {
        homeContent->Pause();
    }
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "\0", 1);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "\0", 1);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "Back", 4);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "\0", 1);

    LED_Manager::clearRing();

    content = trackingContent;
    trackingContent->assignMsg(msg);
    trackingContent->start();
}

void Home_Window::swapToPing(Message_Base *msg)
{
#if DEBUG == 1
    Serial.println("Swapping to ping content");
#endif
    if (content->type == ContentType::PING)
    {
        return;
    }

    if (content->type == ContentType::HOME)
    {
#if DEBUG == 1
        Serial.println("Pausing home content");
#endif
        homeContent->Pause();
    }

    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "\0", 1);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "\0", 1);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "Back", 4);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Send", 4);

    LED_Manager::clearRing();

    content = pingContent;
#if DEBUG == 1
    Serial.println("Assigning message");
#endif
    pingContent->assignMsg(msg);
}

void Home_Window::swapToHome()
{
    if (content->type == ContentType::HOME)
    {
        return;
    }

    if (content->type == ContentType::PING)
    {
        LED_Manager::clearRing();
        pingContent->unassignMsg();
    }
    else if (content->type == ContentType::TRACKING)
    {
        LED_Manager::clearRing();
        trackingContent->unassignMsg();
        trackingContent->stop();
    }

    assignButton(ACTION_GENERATE_QUICK_ACTION_MENU, BUTTON_1, "Actions", 7);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
    assignButton(ACTION_NONE, BUTTON_3, "\0", 1);
    assignButton(ACTION_GENERATE_MENU_WINDOW, BUTTON_4, "Main Menu", 9);

    LED_Manager::clearRing();

    content = homeContent;
    homeContent->contentMode = HOME_CONTENT_MAIN;
    homeContent->Resume();
    homeContent->printContent();
}

void Home_Window::homeContent1()
{
    assignButton(ACTION_GENERATE_QUICK_ACTION_MENU, BUTTON_1, "Actions", 7);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Broadcast", 9);
    assignButton(ACTION_NONE, BUTTON_3, "\0", 1);
    assignButton(ACTION_GENERATE_MENU_WINDOW, BUTTON_4, "Main Menu", 9);
}

void Home_Window::homeContent2()
{
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_1, "Track", 5);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_2, "Mark Read", 9);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "\0", 1);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Reply", 5);
}