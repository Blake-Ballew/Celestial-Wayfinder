#include "LoRa_Test_Content.h"

LoRa_Test_Content::LoRa_Test_Content(Adafruit_SSD1306 *disp)
{
    display = disp;
    type = ContentType::LORA_TEST;
    msgIdx = 0;
}

LoRa_Test_Content::~LoRa_Test_Content()
{
}

void LoRa_Test_Content::encDown()
{
    if (Network_Manager::messages.size() == 0)
    {
#if DEBUG == 1
        Serial.println("No messages");
#endif
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

void LoRa_Test_Content::encUp()
{
    if (Network_Manager::messages.size() == 0)
    {
#if DEBUG == 1
        Serial.println("No messages");
#endif
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

void LoRa_Test_Content::printContent()
{
    display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

#if DEBUG == 1
    Serial.print("Network_Manager::messages.size(): ");
    Serial.println(Network_Manager::messages.size());
#endif

    if (Network_Manager::messages.size() == 0)
    {
#if DEBUG == 1
        Serial.println("printContent(): No messages");
#endif
        display->setCursor(20, 12);
        display->print("No messages");
        display->display();
        return;
    }
#if DEBUG == 1
    Serial.print("msgIdx: ");
    Serial.println(msgIdx);
#endif
    Message_Base *msg = Network_Manager::findMessageByIdx(msgIdx);

#if DEBUG == 1
    Serial.print("SenderName: ");
    Serial.println(msg->senderName);
    Serial.print("MsgID: 0x");
    Serial.println(msg->msgID, HEX);
#endif

    display->setCursor(0, 8);
    display->print("Name: ");
    display->print(msg->senderName);

    display->setCursor(0, 16);
    display->print("MsgID: 0x");
    display->print(msg->msgID, HEX);

    display->display();
}

uint8_t LoRa_Test_Content::sendBroadcast()
{
    Navigation_Manager::read();
    uint32_t time = Navigation_Manager::getTime().value();
    uint32_t date = Navigation_Manager::getDate().value();
    const char *senderName = Settings_Manager::settings["User"]["Name"]["cfgVal"].as<const char *>();
    Message_Base msg = Message_Base(time, date, Network_Manager::userID, senderName, esp_random());

    return Network_Manager::sendBroadcastMessage(&msg);
}

void LoRa_Test_Content::updateMessages()
{
#if DEBUG == 1
    Serial.println("Updating messages");
#endif
    printContent();
}

Message_Base *LoRa_Test_Content::getCurrentMessage()
{
    return Network_Manager::findMessageByIdx(msgIdx);
}