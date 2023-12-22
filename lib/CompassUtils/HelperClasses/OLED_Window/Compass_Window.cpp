#include "Compass_Window.h"

Compass_Window::Compass_Window(OLED_Window *parent) : OLED_Window(parent)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
    // btnCallback = handleBtnInterrupt;
    content = new Compass_Content(display);
    compassContent = (Compass_Content *)content;
}

Compass_Window::~Compass_Window()
{
#if DEBUG == 1
    Serial.println("Compass_Window::~Compass_Window()");
#endif

    delete content;
    content = nullptr;
}

void Compass_Window::execBtnCallback(uint8_t buttonNumber, void *arg)
{
    uint8_t callbackID;
    switch (buttonNumber)
    {
    case BUTTON_1:
        callbackID = btn1CallbackID;
        break;
    case BUTTON_2:
        callbackID = btn2CallbackID;
        break;
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
        LED_Manager::clearRing();
        break;
    default:
        break;
    }
}

void Compass_Window::Pause()
{
    compassContent->Pause();
}

void Compass_Window::Resume()
{
    compassContent->Pause();
}

/* void Compass_Window::updateCompass(TimerHandle_t xTimer)
{
    if (thisInstance == nullptr || thisInstance->content == nullptr)
    {
        return;
    }

    Navigation_Manager::read();
    thisInstance->content->printContent();
} */
/*
void Compass_Window::handleBtnInterrupt(uint8_t buttonNumber, void *arg, OLED_Window *window)
{
#if DEBUG == 1
    Serial.print("void Compass_Window::handleBtnInterrupt(uint8_t buttonNumber, void *arg): Button number: ");
    Serial.println(buttonNumber);
#endif

    Compass_Window *thisWindow = (Compass_Window *)window;
    Compass_Content *content = (Compass_Content *)thisWindow->content;
    uint32_t callbackID;
    switch (buttonNumber)
    {
    case BUTTON_1:
        callbackID = thisWindow->btn1CallbackID;
        break;
    case BUTTON_2:
        callbackID = thisWindow->btn2CallbackID;
        break;
    case BUTTON_3:
        callbackID = thisWindow->btn3CallbackID;
        break;
    case BUTTON_4:
        callbackID = thisWindow->btn4CallbackID;
        break;
    default:
        break;
    }

    switch (callbackID)
    {
    case ACTION_BACK:
        xTimerDelete(thisWindow->updateTimer, 0);
        LED_Manager::clearRing();
        break;
    default:
        break;
    }
}
*/