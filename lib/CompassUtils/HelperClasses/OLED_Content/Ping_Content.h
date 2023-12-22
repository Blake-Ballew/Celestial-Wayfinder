#pragma once

#include "OLED_Content.h"
#include "Network_Manager.h"
#include "Navigation_Manager.h"
#include "Settings_Manager.h"
#include "Message_Types.h"

namespace
{
    const char *MESSAGE_SENT PROGMEM = "Message sent";
    const char *NO_ROUTE PROGMEM = "No route";
    const char *DELIVERY_FAILED PROGMEM = "Delivery failed";
    const char *UNABLE_TO_QUEUE PROGMEM = "Unable to queue";
}

class Ping_Content : public OLED_Content
{
public:
    Ping_Content(Adafruit_SSD1306 *disp, Message_Base *msg)
    {
        display = disp;
        type = ContentType::PING;
        this->msg = msg;
        statusList = Network_Manager::getStatusList();
        statusIdx = 0;
    }

    ~Ping_Content() {}

    void printContent()
    {
#ifdef DEBUG
        Serial.println("Ping_Content::printContent()");
#endif

        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

        display->setCursor(0, 0);
        display->print("To: ");
        if (msg == nullptr)
        {
            display->print("Broadcast");
        }
        else
        {
            display->print(msg->senderName);
        }

        if (statusList.size() == 0)
        {
            display->setCursor(0, 16);
            display->print("No statuses");
            display->display();
            return;
        }
#if DEBUG == 1
        Serial.print("Status idx: ");
        Serial.println(statusIdx);
        Serial.print("Status list size: ");
        Serial.println(statusList.size());
#endif
        const char *statusDesc = statusList[statusIdx].as<const char *>();
#if DEBUG == 1
        Serial.print("Status desc: ");
        Serial.println(statusDesc);
#endif
        display->setCursor(OLED_Content::centerTextHorizontal(statusDesc), OLED_Content::centerTextVertical());
        display->print(statusDesc);
        display->display();

        uint8_t R = Settings_Manager::settings["User"]["Theme Red"]["cfgVal"].as<uint8_t>();
        uint8_t G = Settings_Manager::settings["User"]["Theme Green"]["cfgVal"].as<uint8_t>();
        uint8_t B = Settings_Manager::settings["User"]["Theme Blue"]["cfgVal"].as<uint8_t>();

        LED_Manager::lightRing(R, G, B);
    }
    void encUp()
    {
        if (statusIdx == 0)
        {
            statusIdx = statusList.size();
        }
        statusIdx--;
        printContent();
    }

    void encDown()
    {
        statusIdx++;
        if (statusIdx >= statusList.size())
        {
            statusIdx = 0;
        }
        printContent();
    }

    void assignMsg(Message_Base *msg)
    {
        this->msg = msg;
    }

    void unassignMsg()
    {
        this->msg = nullptr;
    }

    void sendPing()
    {
        Navigation_Manager::read();
        Navigation_Manager::updateGPS();

        uint32_t time, date;
        time = Navigation_Manager::getTime().value();
        date = Navigation_Manager::getDate().value();

        uint64_t sender = Network_Manager::userID;
        const char *senderName = Settings_Manager::settings["User"]["Name"]["cfgVal"].as<const char *>();

        uint32_t msgID = esp_random();

        uint8_t R = Settings_Manager::settings["User"]["Theme Red"]["cfgVal"].as<uint8_t>();
        uint8_t G = Settings_Manager::settings["User"]["Theme Green"]["cfgVal"].as<uint8_t>();
        uint8_t B = Settings_Manager::settings["User"]["Theme Blue"]["cfgVal"].as<uint8_t>();

        TinyGPSLocation coords = Navigation_Manager::getLocation();

#if USE_FAKE_GPS_COORDS == 0
        double lat = coords.lat();
        double lng = coords.lng();
#else
        double lat = FAKE_GPS_LAT;
        double lng = FAKE_GPS_LON;
#endif

        const char *status = statusList[statusIdx].as<const char *>();
        Message_Ping *msgPing = new Message_Ping(time, date, sender, senderName, msgID, R, G, B, lat, lng, status);

        uint8_t returnCode;
        bool isBroadcast = msg == nullptr;

        display->clearDisplay();
        display->setCursor(OLED_Content::centerTextHorizontal(10), OLED_Content::centerTextVertical());
        display->print("Sending...");
        display->display();
        LED_Manager::clearRing();

        if (isBroadcast)
        {
#ifdef DEBUG
            Serial.println("Sending broadcast");
#endif
            returnCode = Network_Manager::queueBroadcastMessage(msgPing);
        }
        else
        {
#ifdef DEBUG
            Serial.print("Sending direct message to: ");
            Serial.println(msg->senderName);
#endif
            returnCode = Network_Manager::queueMessageToUser(msg->sender, msgPing);
        }

        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

        switch (returnCode)
        {
        case RH_ROUTER_ERROR_NONE:
            display->setCursor(OLED_Content::centerTextHorizontal(MESSAGE_SENT), OLED_Content::centerTextVertical());
            display->print(MESSAGE_SENT);

            if (isBroadcast)
            {
                delete Network_Manager::lastBroadcast;
                Network_Manager::lastBroadcast = msgPing;
            }
            else
            {
                uint64_t receiver = msg->sender;
                if (Network_Manager::messagesSent.find(receiver) != Network_Manager::messagesSent.end())
                {
                    delete Network_Manager::messagesSent[receiver];
                }
                Network_Manager::messagesSent[receiver] = msgPing;
            }
            break;
        case RH_ROUTER_ERROR_NO_ROUTE:
            display->setCursor(OLED_Content::centerTextHorizontal(NO_ROUTE), OLED_Content::centerTextVertical());
            display->print(NO_ROUTE);
            break;
        case RH_ROUTER_ERROR_UNABLE_TO_DELIVER:
            display->setCursor(OLED_Content::centerTextHorizontal(DELIVERY_FAILED), OLED_Content::centerTextVertical());
            display->print(DELIVERY_FAILED);
            break;
        case RETURN_CODE_UNABLE_TO_QUEUE:
            display->setCursor(OLED_Content::centerTextHorizontal(UNABLE_TO_QUEUE), OLED_Content::centerTextVertical());
            display->print(UNABLE_TO_QUEUE);
            break;
        default:
            break;
        }

        display->fillRect(OLED_WIDTH, OLED_HEIGHT, OLED_WIDTH - 24, OLED_HEIGHT - 8, BLACK);
        display->display();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    uint8_t statusIdx;

private:
    Message_Base *msg;

    JsonArray statusList;
};