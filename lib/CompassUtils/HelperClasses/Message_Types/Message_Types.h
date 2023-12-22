#pragma once

#include "globalDefines.h"
#include <ArduinoJson.h>
#include "LED_Manager.h"
#include "Navigation_Manager.h"
#include "Adafruit_SSD1306.h"

#define MSG_TYPE_OFFSET 0

#define MSG_BASE_SIZE 512

#define NAME_LENGTH 12
#define STATUS_LENGTH 32

namespace
{
    const char *MESSAGE_TYPE_KEY PROGMEM = "t";
    const char *MESSAGE_TYPE_ID PROGMEM = "i";
    const char *MESSAGE_TYPE_FROM PROGMEM = "f";
    const char *MESSAGE_TYPE_FROM_NAME PROGMEM = "n";
    const char *MESSAGE_TYPE_TIME PROGMEM = "T";
    const char *MESSAGE_TYPE_DATE PROGMEM = "D";
    const char *MESSAGE_TYPE_COLOR_R PROGMEM = "r";
    const char *MESSAGE_TYPE_COLOR_G PROGMEM = "g";
    const char *MESSAGE_TYPE_COLOR_B PROGMEM = "b";
    const char *MESSAGE_TYPE_LAT PROGMEM = "a";
    const char *MESSAGE_TYPE_LNG PROGMEM = "o";
    const char *MESSAGE_TYPE_STATUS PROGMEM = "s";
}

enum MessageType
{
    MESSAGE_INVALID = 0,
    MESSAGE_BASE = 1,
    MESSAGE_PING = 2
};

class Message_Base
{
public:
    bool messageOpened;

    MessageType msgType;
    uint32_t msgID;

    uint64_t sender;
    char senderName[NAME_LENGTH + 1];

    uint32_t time;
    uint32_t date;

    Message_Base()
    {
        this->msgType = MESSAGE_BASE;
    }
    // Constructor for message created by this node
    Message_Base(uint32_t time, uint32_t date, uint64_t sender, const char *senderName, uint32_t msgID)
    {
        messageOpened = false;

        this->time = time;
        this->date = date;
        this->sender = sender;
        this->msgType = MESSAGE_BASE;

        memcpy(this->senderName, senderName, NAME_LENGTH);
        if (msgID != 0)
        {
            this->msgID = msgID;
        }
        else
        {
            this->msgID = esp_random();
        }
    }
    // Constructor for message received from another node
    Message_Base(uint8_t *buffer)
    {
        messageOpened = false;
        deserialize(buffer);
    }

    virtual ~Message_Base()
    {
    }

    static MessageType getMessageType(const uint8_t *buffer)
    {
        if (buffer == nullptr)
            return (MessageType)0;
        ArduinoJson::StaticJsonDocument<MSG_BASE_SIZE> doc;
        deserializeMsgPack(doc, buffer, MSG_BASE_SIZE);
        if (doc.containsKey(MESSAGE_TYPE_KEY))
        {
            return (MessageType)doc[MESSAGE_TYPE_KEY];
        }
        else
        {
            return (MessageType)0;
        }
    }

    virtual uint8_t *serialize(size_t &len)
    {
        ArduinoJson::StaticJsonDocument<MSG_BASE_SIZE> doc;
        doc[MESSAGE_TYPE_KEY] = msgType;
        doc[MESSAGE_TYPE_ID] = msgID;
        doc[MESSAGE_TYPE_FROM] = sender;
        doc[MESSAGE_TYPE_FROM_NAME] = senderName;
        doc[MESSAGE_TYPE_TIME] = time;
        doc[MESSAGE_TYPE_DATE] = date;

        len = measureMsgPack(doc) + 1;
        uint8_t *buffer = new uint8_t[len];
        serializeMsgPack(doc, buffer, len);
        buffer[len - 1] = '\0';
        return buffer;
    }

    virtual void deserialize(const uint8_t *buffer)
    {
        ArduinoJson::StaticJsonDocument<MSG_BASE_SIZE> doc;

        deserializeMsgPack(doc, buffer, MSG_BASE_SIZE);

        msgType = doc[MESSAGE_TYPE_KEY];
        if (msgType != MESSAGE_BASE)
        {
            msgType = MESSAGE_INVALID;
            return;
        }

        msgID = doc[MESSAGE_TYPE_ID];
        sender = doc[MESSAGE_TYPE_FROM];
        strcpy(senderName, doc[MESSAGE_TYPE_FROM_NAME].as<const char *>());
        time = doc[MESSAGE_TYPE_TIME];
        date = doc[MESSAGE_TYPE_DATE];
    }

    virtual void toString(char *buffer, size_t bufferLen)
    {
        snprintf(buffer, bufferLen, "MsgID: %X", msgID);
    }

    static void printFromBuffer(uint8_t *buffer)
    {
        ArduinoJson::StaticJsonDocument<MSG_BASE_SIZE> doc;
        deserializeMsgPack(doc, buffer, MSG_BASE_SIZE);
        serializeJson(doc, Serial);
    }

    virtual void displayMessage(Adafruit_SSD1306 *display)
    {
        display->setCursor(0, 8);
        display->print(this->senderName);

        display->setCursor(0, 16);
        display->print(F("MsgID: "));
        display->print(this->msgID, HEX);

        display->setCursor(110, 8);
        printMessageAge(Navigation_Manager::getTimeDifference(this->time, this->date), display);
    }

    static void printMessageAge(uint64_t timeDiff, Adafruit_SSD1306 *display)
    {
        uint8_t diffHours = (timeDiff & 0xFF000000) >> 24;
        uint8_t diffMinutes = (timeDiff & 0xFF0000) >> 16;

        #if DEBUG == 1
        Serial.print("Time diff: ");
        Serial.println(timeDiff);
        Serial.print("Hours: ");
        Serial.println(diffHours);
        Serial.print("Minutes: ");
        Serial.println(diffMinutes);
        #endif

        if (timeDiff > 0xFFFFFFFF)
        {
            display->print(">1d");
        }
        else if (diffHours > 0)
        {
            display->print(diffHours);
            display->print(F("h"));
        }
        else if (diffMinutes > 0)
        {
            display->print(diffMinutes);
            display->print(F("m"));
        }
        else
        {
            display->print(F("<1m"));
        }
    }
};

class Message_Ping : public Message_Base
{
public:
    Message_Ping()
    {
        this->msgType = MESSAGE_PING;
    }

    Message_Ping(uint32_t time, uint32_t date, uint64_t sender, const char *senderName, uint32_t msgID, uint8_t color_R, uint8_t color_G, uint8_t color_B, double lat, double lng, const char *status)
        : Message_Base(time, date, sender, senderName, msgID)
    {
        this->msgType = MESSAGE_PING;
        messageOpened = false;

        this->color_R = color_R;
        this->color_G = color_G;
        this->color_B = color_B;

        this->lat = lat;
        this->lng = lng;

        size_t strLen = strlen(status);
        if (strLen > STATUS_LENGTH)
            strLen = STATUS_LENGTH;

        memcpy(this->status, status, strLen);
        this->status[strLen] = '\0';
    }

    Message_Ping(uint8_t *buffer)
    {
        messageOpened = false;
        deserialize(buffer);
    }

    ~Message_Ping()
    {
    }

    uint8_t *serialize(size_t &len)
    {
        ArduinoJson::StaticJsonDocument<MSG_BASE_SIZE> doc;
        doc[MESSAGE_TYPE_KEY] = msgType;
        doc[MESSAGE_TYPE_ID] = msgID;
        doc[MESSAGE_TYPE_FROM] = sender;
        doc[MESSAGE_TYPE_FROM_NAME] = senderName;
        doc[MESSAGE_TYPE_TIME] = time;
        doc[MESSAGE_TYPE_DATE] = date;

        doc[MESSAGE_TYPE_LAT] = lat;
        doc[MESSAGE_TYPE_LNG] = lng;

        doc[MESSAGE_TYPE_STATUS] = status;

        doc[MESSAGE_TYPE_COLOR_R] = color_R;
        doc[MESSAGE_TYPE_COLOR_G] = color_G;
        doc[MESSAGE_TYPE_COLOR_B] = color_B;

        len = measureMsgPack(doc);
        uint8_t *buffer = new uint8_t[len];
        serializeMsgPack(doc, buffer, len);

#if DEBUG == 1
        Serial.print("Ping msg length: ");
        Serial.println(len);
        serializeJson(doc, Serial);
        Serial.println();
#endif
        return buffer;
    }

    void deserialize(uint8_t *buffer)
    {
#if DEBUG == 1
        Serial.print("Deserializing ping message of length");
#endif
        ArduinoJson::StaticJsonDocument<MSG_BASE_SIZE> doc;

        deserializeMsgPack(doc, buffer, MSG_BASE_SIZE);
#if DEBUG == 1
        serializeJson(doc, Serial);
        Serial.println();
#endif

        msgType = doc[MESSAGE_TYPE_KEY] | (MessageType)0;
        if (msgType != MESSAGE_PING)
        {
#if DEBUG == 1
            Serial.println("Invalid message type");
#endif
            msgType = MESSAGE_INVALID;
            return;
        }

#if DEBUG == 1
        Serial.println("Valid message type");
#endif

        msgID = doc[MESSAGE_TYPE_ID];
        sender = doc[MESSAGE_TYPE_FROM];
        const char *sendName = doc[MESSAGE_TYPE_FROM_NAME].as<const char *>();
        size_t nameLen = strlen(sendName);
        if (nameLen > NAME_LENGTH)
            nameLen = NAME_LENGTH;
        strncpy(senderName, sendName, nameLen);
        senderName[nameLen] = '\0';
        time = doc[MESSAGE_TYPE_TIME];
        date = doc[MESSAGE_TYPE_DATE];

        lat = doc[MESSAGE_TYPE_LAT];
        lng = doc[MESSAGE_TYPE_LNG];

        const char *msgStatus = doc[MESSAGE_TYPE_STATUS].as<const char *>();
        size_t strLen = 0;
        if (msgStatus == nullptr)
        {
#if DEBUG == 1
            Serial.println("Status is null");
#endif
        }
        else
        {
            strLen = strlen(msgStatus);
            Serial.print("Status length: ");
            Serial.println(strLen);
        }

        if (strLen > STATUS_LENGTH)
        {
#if DEBUG == 1
            Serial.println("Status length is too long");
#endif
            strLen = STATUS_LENGTH;
        }

        if (msgStatus != nullptr)
        {
            strncpy(status, msgStatus, strLen);
            status[strLen] = '\0';
        }

        color_R = doc[MESSAGE_TYPE_COLOR_R];
        color_G = doc[MESSAGE_TYPE_COLOR_G];
        color_B = doc[MESSAGE_TYPE_COLOR_B];
    }

    void displayMessage(Adafruit_SSD1306 *display)
    {
        Navigation_Manager::updateGPS();

        uint64_t timeDiff = Navigation_Manager::getTimeDifference(this->time, this->date);

        display->setCursor(110, 8);
        // 128display->print(F("Recv: "));

        // Greater than one day
        printMessageAge(timeDiff, display);

        display->setCursor(0, 8);
        display->print(this->senderName);

        display->setCursor(0, 16);
        display->print(this->status);

        LED_Manager::lightRing(this->color_R, this->color_G, this->color_B);
    }

    void toString(char *buffer, size_t bufferLen)
    {
        snprintf(buffer, bufferLen, status);
    }

    uint8_t color_R;
    uint8_t color_G;
    uint8_t color_B;

    double lat;
    double lng;

    char status[STATUS_LENGTH + 1];
};