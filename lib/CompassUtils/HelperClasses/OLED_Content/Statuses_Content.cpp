#include "Statuses_Content.h"

Statuses_Content::Statuses_Content(Adafruit_SSD1306 *disp)
{
    display = disp;
    type = ContentType::STATUS;
    msgIdx = 0;
}

Statuses_Content::~Statuses_Content()
{
}

void Statuses_Content::encDown()
{
    if (Network_Manager::messages.size() == 0)
    {
        return;
    }
    msgIdx++;
    if (msgIdx >= Network_Manager::messages.size())
    {
        msgIdx = 0;
    }

#if DEBUG == 1
    Serial.print("msgIdx: ");
    Serial.println(msgIdx);
#endif

    printContent();
}

void Statuses_Content::encUp()
{
    if (Network_Manager::messages.size() == 0)
    {
        return;
    }

    if (msgIdx == 0)
    {
        msgIdx = Network_Manager::messages.size() - 1;
    }
    else
    {
        msgIdx--;
    }

#if DEBUG == 1
    Serial.print("msgIdx: ");
    Serial.println(msgIdx);
#endif

    printContent();
}

void Statuses_Content::printContent()
{
    display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

#if DEBUG == 1
    Serial.println("Statuses_Content::printContent()");
#endif

    if (Network_Manager::messages.size() == 0)
    {
        display->fillRect(0, 0, OLED_WIDTH / 2, 8, BLACK);
        display->fillRect(OLED_WIDTH / 2, OLED_HEIGHT - 8, OLED_WIDTH / 2, 8, BLACK);
#if DEBUG == 1
        Serial.println("No messages");
#endif
        display->setCursor(20, 12);
        display->print("No messages");
        display->display();
        return;
    }

    Message_Base *msg = Network_Manager::findMessageByIdx(msgIdx);
    switch (msg->msgType)
    {
    case MessageType::MESSAGE_BASE:
    {
        display->setCursor(0, 8);
        display->print(msg->senderName);

        display->setCursor(0, 16);
        display->print(F("MsgID: "));
        display->print(msg->msgID, HEX);

        display->setCursor(110, 8);
        printMessageAge(Navigation_Manager::getTimeDifference(msg->time, msg->date));
        break;
    }
    case MessageType::MESSAGE_PING:
    {
        Message_Ping *pingMsg = (Message_Ping *)msg;
        uint64_t timeDiff = Navigation_Manager::getTimeDifference(pingMsg->time, pingMsg->date);

        display->setCursor(110, 8);
        // 128display->print(F("Recv: "));

        // Greater than one day
        printMessageAge(timeDiff);

        display->setCursor(0, 8);
        display->print(pingMsg->senderName);

        display->setCursor(0, 16);
        display->print(pingMsg->status);

        LED_Manager::lightRing(pingMsg->color_R, pingMsg->color_G, pingMsg->color_B);
        break;
    }
    }

    display->display();
}

void Statuses_Content::printMessageAge(uint64_t timeDiff)
{
    if (timeDiff > 0xFFFFFFFF)
    {
        display->print(">1d");
    }
    else if ((timeDiff & 0xFF000000) >> 24)
    {
        uint8_t hours = (timeDiff & 0xFF000000) >> 24;
        display->print(hours);
        display->print(F("h"));
    }
    else if ((timeDiff && 0xFF0000) >> 16)
    {
        uint8_t minutes = (timeDiff && 0xFF0000) >> 16;
        display->print(minutes);
        display->print(F("m"));
    }
    else
    {
        display->print(F("<1m"));
    }
}
