#include "Compass_Content.h"

Compass_Content *Compass_Content::thisInstance = nullptr;
StaticTimer_t Compass_Content::updateTimerBuffer;
TimerHandle_t Compass_Content::updateTimer = xTimerCreateStatic("CompassUpdate", pdMS_TO_TICKS(25), pdTRUE, (void *)0, updateCompass, &updateTimerBuffer);

Compass_Content::Compass_Content(Adafruit_SSD1306 *disp)
{
    display = disp;
    thisInstance = this;
    xTimerStart(this->updateTimer, 0);
}

Compass_Content::~Compass_Content()
{
#if DEBUG == 1
    Serial.println("Compass_Content destructor");
#endif
    if (xTimerIsTimerActive(updateTimer) == pdTRUE)
    {
#if DEBUG == 1
        Serial.println("Compass_Content destructor: Timer is active");
#endif
        if (xTimerStop(updateTimer, 0) == pdFALSE)
        {
#if DEBUG == 1
            Serial.println("Compass_Content destructor: Could not stop timer");
#endif
        }
        else
        {
#if DEBUG == 1
            Serial.println("Compass_Content destructor: Timer stopped");
#endif
        }
    }
    /*         if (xTimerDelete(updateTimer, 500) == pdFALSE)
            {
    #if DEBUG == 1
                Serial.println("Compass_Content destructor: Could not delete timer");
    #endif
            } */
#if DEBUG == 1
    Serial.println("Compass_Content destructor: Timer deleted");
#endif
    thisInstance = nullptr;
}

void Compass_Content::printContent()
{

    display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT - 8, BLACK);

    display->setCursor(0, 0);
    display->print("X:");
    display->print(Navigation_Manager::getX());

    display->setCursor(0, 8);
    display->print("Y:");
    display->print(Navigation_Manager::getY());

    display->setCursor(0, 16);
    display->print("Z:");
    display->print(Navigation_Manager::getZ());

    display->setCursor(40, 0);
    display->print("  Azimuth:");
    display->print(Navigation_Manager::InvertXAzimuth(Navigation_Manager::getAzimuth()));

    char direction[4];
    memset(direction, 0, 4);
    Navigation_Manager::getDirection(direction);
    display->setCursor(40, 16);
    display->print("Direction:");
    display->print(direction);

    display->display();

    LED_Manager::pointNorth(Navigation_Manager::InvertYAzimuth(Navigation_Manager::InvertXAzimuth(Navigation_Manager::getAzimuth())));
}

void Compass_Content::encUp()
{
}

void Compass_Content::encDown()
{
}

void Compass_Content::Pause()
{
    xTimerStop(this->updateTimer, 0);
}

void Compass_Content::Resume()
{
    xTimerStart(this->updateTimer, 0);
}

void Compass_Content::updateCompass(TimerHandle_t xTimer)
{
    if (thisInstance == nullptr)
    {
        xTimerStop(updateTimer, 0);
        return;
    }

    Navigation_Manager::read();
    thisInstance->printContent();
}
