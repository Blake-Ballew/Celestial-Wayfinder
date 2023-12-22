#include "LoRa_Test_Window.h"

LoRa_Test_Window::LoRa_Test_Window(OLED_Window *parent)
    : OLED_Window(parent)
{
    assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
    assignButton(ACTION_DEFER_CALLBACK_TO_WINDOW, BUTTON_4, "SendMsg", 7);
    content = new LoRa_Test_Content(display);
}

void LoRa_Test_Window::execBtnCallback(uint8_t buttonNumber, void *arg)
{
    uint8_t callbackID;
    switch (buttonNumber)
    {
    case BUTTON_1:
        callbackID = btn1CallbackID;
        break;
    case BUTTON_2:
    {
        callbackID = btn2CallbackID;
        LoRa_Test_Content *c = (LoRa_Test_Content *)content;
        c->sendBroadcast();
        break;
    }
    case BUTTON_3:
        callbackID = btn3CallbackID;
        break;
    case BUTTON_4:
        callbackID = btn4CallbackID;
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