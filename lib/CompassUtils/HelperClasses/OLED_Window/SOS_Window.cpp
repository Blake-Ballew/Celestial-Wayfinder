#include "SOS_Window.h"

SOS_Window::SOS_Window(OLED_Window *parent) : OLED_Window(parent)
{
    assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Confirm", 7);
    sosContent = new SOS_Content(display);
    content = (OLED_Content *)sosContent;
}

SOS_Window::~SOS_Window()
{
#if DEBUG == 1
    Serial.println("SOS_Window::~SOS_Window()");
#endif
}

void SOS_Window::execBtnCallback(uint8_t buttonNumber, void *arg)
{
#if DEBUG == 1
    Serial.println("SOS_Window::execBtnCallback");
    Serial.printf("buttonNumber: %d\n", buttonNumber);
#endif
    uint32_t callbackID;
    switch (buttonNumber)
    {
    case BUTTON_1:
        callbackID = btn1CallbackID;
        break;
    case BUTTON_2:
        callbackID = btn2CallbackID;
        break;
    case BUTTON_3:
    {
#if DEBUG == 1
        Serial.println("SOS_Window::execBtnCallback: BUTTON_3");
        Serial.printf("Button 3 callbackID: %u\n", btn3CallbackID);
#endif
        callbackID = btn3CallbackID;
        if (callbackID == ACTION_DEFER_CALLBACK_TO_WINDOW)
        {
#if DEBUG == 1
            Serial.println("SOS_Window::execBtnCallback: ACTION_DEFER_CALLBACK_TO_WINDOW");
#endif
            assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "Confirm", 7);
            sosContent->unconfirmSOS();
            LED_Manager::clearRing();
        }
        else
        {
#if DEBUG == 1
            Serial.print("Button 3 callbackID: ");
            Serial.println(callbackID, HEX);
#endif
        }
    }
    break;
    case BUTTON_4:
    {
        callbackID = btn4CallbackID;
        if (sosContent->confirmed == false)
        {
            sosContent->confirmSOS();
            assignButton(ACTION_NONE, BUTTON_4, "\0", 1);
            assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_3, "Back", 4);
        }
        else
        {
        }
    }
    break;
    default:
        break;
    }

    switch (callbackID)
    {
    case ACTION_BACK:
    {
        LED_Manager::clearRing();
    }
    break;
    default:
        break;
    }
}

void SOS_Window::Pause()
{
    sosContent->Pause();
}

void SOS_Window::Resume()
{
    sosContent->Pause();
}
