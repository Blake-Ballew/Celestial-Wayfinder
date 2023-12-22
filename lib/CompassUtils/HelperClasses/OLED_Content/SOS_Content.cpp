#include "SOS_Content.h"

size_t SOS_Content::currentTick = 0;

SOS_Content *SOS_Content::thisInstance = nullptr;
StaticTimer_t SOS_Content::updateTimerBuffer;
TimerHandle_t SOS_Content::updateTimer = xTimerCreateStatic("SOSUpdate", pdMS_TO_TICKS(SOS_CONTENT_TIMER_TICK), pdTRUE, (void *)0, updateDisplay, &updateTimerBuffer);

SOS_Content::SOS_Content(Adafruit_SSD1306 *disp)
{
    confirmed = false;
    display = disp;
    type = ContentType::SOS;
}

SOS_Content::~SOS_Content()
{
    thisInstance = nullptr;
}

void SOS_Content::printContent()
{
#if DEBUG == 1
    // Serial.println("SOS_Content::printContent()");
    // Serial.printf("SOS_Content::printContent(): confirmed: %d\n", confirmed);
    // Serial.printf("SOS_Content::printContent(): currentTick: %d\n", currentTick);
#endif
    OLED_Content::clearContentArea();
    if (!confirmed)
    {
        OLED_Content::clearContentArea();
        display->setCursor(OLED_Content::centerTextHorizontal("Send SOS?"), OLED_Content::centerTextVertical());
        display->print("Send SOS?");
        display->display();
        return;
    }
    else
    {
        display->setCursor(OLED_Content::centerTextHorizontal(14), OLED_Content::centerTextVertical());
        display->print("Sending SOS");
        for (int i = 0; i < ((currentTick / 60) % 3) + 1; i++)
        {
            display->print(".");
        }
        LED_Manager::pulseCircle(255, 0, 0, currentTick);
        display->display();
    }
}

void SOS_Content::Pause()
{
    if (thisInstance == nullptr)
    {
        return;
    }
    xTimerStop(updateTimer, 0);
}

void SOS_Content::Resume()
{
    if (thisInstance == nullptr)
    {
        return;
    }
    xTimerStart(updateTimer, 0);
}

void SOS_Content::confirmSOS()
{
    confirmed = true;
    thisInstance = this;
    currentTick = SOS_CONTENT_TICKS_PER_MESSAGE;
    xTimerStart(updateTimer, 0);
    msgID = esp_random();
}

void SOS_Content::unconfirmSOS()
{
    confirmed = false;
    thisInstance = nullptr;
    xTimerStop(updateTimer, 1000);
    sendOkay();
}

void SOS_Content::updateDisplay(TimerHandle_t timer)
{
    if (thisInstance == nullptr)
    {
        xTimerStop(timer, 0);
        return;
    }

    thisInstance->printContent();
    currentTick++;
    if (currentTick >= SOS_CONTENT_TICKS_PER_MESSAGE)
    {
        thisInstance->sendSOS();
        currentTick = 0;
    }
}

void SOS_Content::sendSOS()
{
    if (!confirmed)
    {
        return;
    }

    msgID = esp_random();

    Navigation_Manager::read();

    uint32_t time, date;
    time = Navigation_Manager::getTime().value();
    date = Navigation_Manager::getDate().value();

    uint64_t sender = Network_Manager::userID;
    const char *senderName = Settings_Manager::settings["User"]["Name"]["cfgVal"].as<const char *>();

    uint8_t R = 255;
    uint8_t G = 0;
    uint8_t B = 0;

    TinyGPSLocation coords = Navigation_Manager::getLocation();

#if USE_FAKE_GPS_COORDS == 0
    double lat = coords.lat();
    double lng = coords.lng();
#else
    double lat = FAKE_GPS_LAT;
    double lng = FAKE_GPS_LON;
#endif

    const char *status = SOS_CONTENT_MESSAGE;
    Message_Ping *msgPing = new Message_Ping(time, date, sender, senderName, msgID, R, G, B, lat, lng, status);

    uint8_t returnCode = Network_Manager::queueBroadcastMessage(msgPing);
}

void SOS_Content::sendOkay()
{
    if (confirmed)
    {
        return;
    }

    msgID = esp_random();

    Navigation_Manager::read();

    uint32_t time, date;
    time = Navigation_Manager::getTime().value();
    date = Navigation_Manager::getDate().value();

    uint64_t sender = Network_Manager::userID;
    const char *senderName = Settings_Manager::settings["User"]["Name"]["cfgVal"].as<const char *>();

    uint8_t R = 0;
    uint8_t G = 255;
    uint8_t B = 0;

    TinyGPSLocation coords = Navigation_Manager::getLocation();

#if USE_FAKE_GPS_COORDS == 0
    double lat = coords.lat();
    double lng = coords.lng();
#else
    double lat = FAKE_GPS_LAT;
    double lng = FAKE_GPS_LON;
#endif

    const char *status = OK_CONTENT_MESSAGE;
    Message_Ping *msgPing = new Message_Ping(time, date, sender, senderName, msgID, R, G, B, lat, lng, status);

    uint8_t returnCode = Network_Manager::queueBroadcastMessage(msgPing);
}