#include "GPS_Window.h"

GPS_Window *GPS_Window::thisInstance = nullptr;
StaticTimer_t GPS_Window::updateTimerBuffer;
TimerHandle_t GPS_Window::updateTimer = xTimerCreateStatic("GPSUpdateTimer", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, GPS_Window::updateGPS, &updateTimerBuffer);

GPS_Window::GPS_Window(OLED_Window *parent) : OLED_Window(parent)
{
    assignButton(ACTION_BACK, BUTTON_3, "Back", 4);
    btnCallback = handleBtnInterrupt;
    content = new GPS_Content(display);
    thisInstance = this;
    xTimerStart(this->updateTimer, 0);
}

GPS_Window::~GPS_Window()
{
#if DEBUG == 1
    Serial.println("GPS_Window::~GPS_Window()");
#endif
    if (xTimerIsTimerActive(updateTimer) == pdTRUE)
    {
        thisInstance = nullptr;
        xTimerStop(updateTimer, 0);
    }
    delete content;
    content = nullptr;
}

void GPS_Window::execBtnCallback(uint8_t buttonNumber, void *arg)
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
        xTimerDelete(this->updateTimer, 0);
        break;
    default:
        break;
    }
}

void GPS_Window::updateGPS(TimerHandle_t xTimer)
{
#if DEBUG == 1
    Serial.println("void GPS_Window::updateGPS(TimerHandle_t xTimer)");
#endif
    if (thisInstance == nullptr)
    {
        xTimerStop(updateTimer, 0);
    }
    thisInstance->content->printContent();
}

void GPS_Window::handleBtnInterrupt(uint8_t buttonNumber, void *arg, OLED_Window *window)
{
#if DEBUG == 1
    Serial.print("void Compass_Window::handleBtnInterrupt(uint8_t buttonNumber, void *arg): Button number: ");
    Serial.println(buttonNumber);
#endif

    GPS_Window *thisWindow = (GPS_Window *)window;
    GPS_Content *content = (GPS_Content *)thisWindow->content;
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
        xTimerStop(thisWindow->updateTimer, 0);
        thisInstance = nullptr;
        break;
    default:
        break;
    }
}
