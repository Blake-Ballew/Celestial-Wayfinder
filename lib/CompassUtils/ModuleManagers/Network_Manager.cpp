#include "Network_Manager.h"

RHHardwareSPI Network_Manager::rf_spi;
RH_DRIVER Network_Manager::driver(RFM95_CS, RFM95_Int, rf_spi);
RHMesh *Network_Manager::manager;

StaticQueue_t Network_Manager::messageQueueBuffer;
QueueHandle_t Network_Manager::messageQueue;
uint8_t Network_Manager::messageDataBuffer[MESSAGE_QUEUE_MAX * sizeof(MessageQueueItem)];

std::map<uint8_t, uint64_t> Network_Manager::nodeIDs;
std::map<uint64_t, Message_Base *> Network_Manager::messages;
std::map<uint64_t, Message_Base *> Network_Manager::messagesSent;

Message_Base *Network_Manager::lastBroadcast;

uint8_t Network_Manager::buffer[RH_MESH_MAX_MESSAGE_LEN];

TaskHandle_t *Network_Manager::taskHandle = nullptr;

uint8_t Network_Manager::nodeID;
uint64_t Network_Manager::userID;
uint8_t Network_Manager::numRetries = 1;

ArduinoJson::DynamicJsonDocument Network_Manager::statusList(SIZE_STATUSES_OBJECT);

namespace
{
    const char *STATUSES PROGMEM = "Statuses";
}

bool Network_Manager::init(TaskHandle_t *taskHandle)
{
#if DEBUG == 1
    Serial.println("Network Manager init");
#endif
    Network_Manager::taskHandle = taskHandle;
    float freq;
    RH_RF95::ModemConfigChoice modemConfig = RF95_MODEM_CONFIG;

    if (Settings_Manager::settings != NULL)
    {
        JsonArray user = Settings_Manager::settings["User"]["UserID"].as<JsonArray>();
        userID = 0;
        for (uint8_t i = 0; i < USERID_SIZE_BYTES; i++)
        {
            userID |= (uint64_t)user[i] << (((USERID_SIZE_BYTES - 1) - i) * 8);
        }

#if DEBUG == 1
        Serial.print("UserID: 0x");

        Serial.println(userID, HEX);
#endif
        nodeID = Settings_Manager::settings["Radio"]["NodeID"]["cfgVal"].as<uint8_t>() | DEFAULT_NODE_ID;
        freq = Settings_Manager::settings["Radio"]["Frequency"]["cfgVal"] | RF95_FREQ;
        size_t cfgIdx = Settings_Manager::settings["Radio"]["Modem Config"]["cfgVal"].as<size_t>();
        modemConfig = (RH_RF95::ModemConfigChoice)Settings_Manager::settings["Radio"]["Modem Config"]["vals"].as<JsonArray>()[cfgIdx].as<uint32_t>();
        numRetries = Settings_Manager::settings["Radio"]["Broadcast Retries"]["cfgVal"].as<uint8_t>();
    }
    else
    {
        nodeID = DEFAULT_NODE_ID;
        freq = RF95_FREQ;
    }
    // rf_spi.setPins(RF95_SPI_MISO, RF95_SPI_MOSI, RF95_SPI_SCK);
    manager = new RHMesh(driver, nodeID);
    manager->setTimeout(10000);
    if (!manager->init())
    {
        Serial.println("Mesh Manager init failed");
        return false;
    }

    // Initialize message queue
    messageQueue = xQueueCreateStatic(MESSAGE_QUEUE_MAX, sizeof(MessageQueueItem), messageDataBuffer, &messageQueueBuffer);

    // Set frequency from settings
    if (!driver.setFrequency(freq))
    {
        Serial.println("Incompatible frequency selected");
        return false;
    }
    //  driver.setTxPower(RF95_TX_PWR, false);
    driver.setModemConfig(RF95_MODEM_CONFIG);

// Load statuses
#if WRITE_STATUSES_TO_EEPROM == 1
    loadStatusList();
#else
    Settings_Manager::readStatusesFromEEPROM(statusList);
#endif

#if DEBUG == 1
    Serial.println("Network Manager init complete");
#endif
    return true;
}

uint8_t Network_Manager::sendBroadcastMessage(Message_Base *message)
{
#if DEBUG == 1
    Serial.print("Sending broadcast message from nodeID: ");
    Serial.println(manager->thisAddress());
#endif
    size_t msgLen = 0;
    uint8_t *msg = message->serialize(msgLen);
    uint8_t ret;

    for (uint8_t sendLoop = 0; sendLoop < numRetries; sendLoop++)
    {
        // not too concerned with return code since broadcast shouldn't fail
        ret = manager->sendtoWait(msg, msgLen, RH_BROADCAST_ADDRESS);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

#if DEBUG == 1
    Serial.print("Sent broadcast message of length: ");
    Serial.println(msgLen);
    Serial.print("RHMesh.sendtoWait to broadcast returned with code: ");
    Serial.println(ret);
    Serial.println("Deserialized to: ");
    Message_Base::printFromBuffer(msg);
    Serial.println();
    Serial.print("Raw bytes: ");
    for (uint8_t i = 0; i < msgLen; i++)
    {
        Serial.print(msg[i]);
        Serial.print(" ");
    }
    Serial.println();
#endif
    delete msg;
    return ret;
}

uint8_t Network_Manager::sendMessageToUser(uint64_t user, Message_Base *message)
{
    size_t msgLen = 0;
    uint8_t *msg = message->serialize(msgLen);
    uint8_t nodeID = findNodeIDofUser(user);
#if DEBUG == 1
    Serial.print("Sending message to nodeID: ");
    Serial.println(nodeID);
#endif
    if (nodeID == 255)
    {
        return RH_ROUTER_ERROR_NO_ROUTE;
    }
    uint8_t ret;
    for (uint8_t sendLoop = 0; sendLoop < numRetries; sendLoop++)
    {
        ret = manager->sendtoWait(msg, msgLen, nodeID);
        if (ret == RH_ROUTER_ERROR_NONE)
        {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

#if DEBUG == 1
    Serial.print("Sent message of length: ");
    Serial.println(msgLen);
    Serial.print("RHMesh.sendtoWait to user 0x");
    Serial.print(user, HEX);
    Serial.print(" returned with code: ");
    Serial.println(ret);
#endif
    delete msg;
    return ret;
}

uint8_t Network_Manager::queueBroadcastMessage(Message_Base *msg)
{
    uint8_t returnCode = RETURN_CODE_UNABLE_TO_QUEUE;
    MessageQueueItem item;
    item.isBroadcast = true;
    item.user = 0;
    item.msg = msg;
    item.returnCode = &returnCode;
    xQueueSend(messageQueue, &item, 0);
    unsigned long beginTimer = millis();
    while (millis() - beginTimer < MESSAGE_QUEUE_TIMEOUT)
    {
        if (returnCode != RETURN_CODE_UNABLE_TO_QUEUE)
        {
            return returnCode;
        }
    }

    return returnCode;
}

uint8_t Network_Manager::queueMessageToUser(uint64_t user, Message_Base *msg)
{
    uint8_t returnCode = RETURN_CODE_UNABLE_TO_QUEUE;
    MessageQueueItem item;
    item.isBroadcast = false;
    item.user = user;
    item.msg = msg;
    item.returnCode = &returnCode;
    xQueueSend(messageQueue, &item, 0);
    unsigned long beginTimer = millis();
    while (millis() - beginTimer < MESSAGE_QUEUE_TIMEOUT)
    {
        if (returnCode != RETURN_CODE_UNABLE_TO_QUEUE)
        {
            return returnCode;
        }
    }

    return returnCode;
}

uint8_t Network_Manager::findFreeNodeID()
{
#if DEBUG == 1
    Serial.print("IDs currently mapped: ");
    std::map<uint8_t, uint64_t>::iterator it;
    for (it = nodeIDs.begin(); it != nodeIDs.end(); it++)
    {
        Serial.print(it->first);
        Serial.print(": 0x");
        Serial.print(it->second, HEX);
        Serial.print(", ");
    }
    Serial.println();
#endif

    uint8_t id = 1;
    while (nodeIDs.find(id) != nodeIDs.end())
    {
        id++;
    }
    return id;
}

void Network_Manager::listenForMessages(void *taskParams)
{
    while (true)
    {
        // Check if there are any messages to send
        MessageQueueItem item;
        if (xQueueReceive(messageQueue, &item, 0) == pdTRUE)
        {
            if (item.isBroadcast)
            {
                *item.returnCode = sendBroadcastMessage(item.msg);
            }
            else
            {
                *item.returnCode = sendMessageToUser(item.user, item.msg);
            }
        }
        if (manager->available())
        {
            uint8_t from;
            uint8_t to;
            uint8_t id;
            uint8_t flags;
            uint8_t len = sizeof(buffer);
            memset(buffer, 0, len);
            if (manager->recvfromAck(buffer, &len, &from, &to, &id, &flags))
            {
                MessageType msgType = Message_Base::getMessageType(buffer);
                if (msgType == MessageType::MESSAGE_INVALID)
                {
#if DEBUG == 1
                    Serial.println("Invalid message received");
#endif
                    return;
                }

                Message_Base *msg;
                switch (msgType)
                {
                case MessageType::MESSAGE_BASE:
                {
#if DEBUG == 1
                    Serial.print("Base message received from nodeID: ");
                    Serial.println(from);
#endif
                    msg = new Message_Base(buffer);
                    break;
                }
                case MessageType::MESSAGE_PING:
                {
#if DEBUG == 1
                    Serial.print("Ping message received from nodeID: ");
                    Serial.println(from);
                    Serial.print("Message length: ");
                    Serial.println(len);
                    Serial.print("Raw bytes: ");
                    for (uint8_t i = 0; i < len; i++)
                    {
                        Serial.print(buffer[i], HEX);
                        Serial.print(" ");
                    }
                    Serial.println();
#endif
                    msg = new Message_Ping(buffer);
                    break;
                }
                }

                // Update nodeID map
                uint8_t savedNodeID = findNodeIDofUser(msg->sender);
#if DEBUG == 1
                if (savedNodeID != 255)
                {
                    Serial.print("Saved nodeID for UserID 0x");
                    Serial.print(msg->sender, HEX);
                    Serial.print(": ");
                    Serial.println(savedNodeID);
                }
                else
                {
                    Serial.print("UserID 0x");
                    Serial.print(msg->sender, HEX);
                    Serial.println(" not found in nodeID map");
                }
#endif
                if (savedNodeID == 255)
                {
                    nodeIDs[from] = msg->sender;
                }
                else if (savedNodeID != from)
                {
                    nodeIDs.erase(savedNodeID);
                    nodeIDs[from] = msg->sender;
                }

                // Check if sender has same nodeID as this node
                if (from == nodeID)
                {
#if DEBUG == 1
                    Serial.println("This node's ID is currently in use. Finding new ID");
#endif
                    nodeID = findFreeNodeID();
                    manager->setThisAddress(nodeID);
                }

                bool sendNotification = true;

                // Update message map
                if (messages.find(msg->sender) != messages.end())
                {
                    if (messages.find(msg->sender)->second->msgID == msg->msgID)
                    {
                        sendNotification = false;
                    }
                    delete messages[msg->sender];
                }
                messages[msg->sender] = msg;

                // TODO: send interrupt to OLED_Manager to update screen
                if (taskHandle != nullptr && sendNotification)
                {
                    uint32_t notification = BIT_SHIFT((uint8_t)EVENT_MESSAGE_RECEIVED);
                    xTaskNotify(*taskHandle, notification, eSetBits);
                }
            }
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

uint8_t Network_Manager::findNodeIDofUser(uint64_t user)
{
    std::map<uint8_t, uint64_t>::iterator it;
    for (it = nodeIDs.begin(); it != nodeIDs.end(); it++)
    {
        if (it->second == user)
        {
            return it->first;
        }
    }
    // 255 is broadcast address, so it is returned if the user is not found
    return 255;
}

Message_Base *Network_Manager::findMessageByIdx(uint16_t idx)
{
    std::map<uint64_t, Message_Base *>::iterator it;
    idx = idx % messages.size();
    it = messages.begin();
    for (uint16_t i = 0; i < idx; i++)
    {
        it++;
    }
    return it->second;
}

ArduinoJson::JsonArray Network_Manager::getStatusList()
{
    if (Network_Manager::statusList.containsKey(STATUSES))
    {
        return Network_Manager::statusList[STATUSES].as<ArduinoJson::JsonArray>();
    }
    else
    {
        return ArduinoJson::JsonArray();
    }
}

size_t Network_Manager::getNumUnreadMessages()
{
    size_t numUnread = 0;
    for (auto it = messages.begin(); it != messages.end(); it++)
    {
        /* #if DEBUG == 1
                Serial.print("Message from 0x");
                Serial.print(it->first, HEX);
                Serial.print(" is ");
                Serial.println(it->second->messageOpened ? "read" : "unread");
        #endif */
        if (!it->second->messageOpened)
        {
            numUnread++;
        }
    }

    return numUnread;
}

std::map<uint64_t, Message_Base *>::iterator Network_Manager::getUnreadBegin()
{
    std::map<uint64_t, Message_Base *>::iterator it = messages.begin();
    for (; it != messages.end(); it++)
    {
        if (!it->second->messageOpened)
        {
            return it;
        }
    }
    return messages.end();
}

std::map<uint64_t, Message_Base *>::iterator Network_Manager::getUnreadEnd()
{
    std::map<uint64_t, Message_Base *>::iterator it = messages.end();
    for (; it != messages.begin(); it--)
    {
        if (!it->second->messageOpened)
        {
            return it;
        }
    }
    return messages.end();
}

std::map<uint64_t, Message_Base *>::iterator Network_Manager::incrementUnreadIterator(std::map<uint64_t, Message_Base *>::iterator it)
{
    it++;
    for (; it != messages.end(); it++)
    {
        if (!it->second->messageOpened)
        {
            return it;
        }
    }
    return messages.end();
}

std::map<uint64_t, Message_Base *>::iterator Network_Manager::decrementUnreadIterator(std::map<uint64_t, Message_Base *>::iterator it)
{
#if DEBUG == 1
    Serial.print("Decrementing iterator from 0x");
    Serial.println(it->first, HEX);
#endif
    if (it == messages.begin())
    {
#if DEBUG == 1
        Serial.println("Iterator is already at the beginning");
#endif
        return messages.end();
    }

    it--;
    for (; it != messages.begin(); it--)
    {
#if DEBUG == 1
        Serial.print("Message from 0x");
        Serial.print(it->first, HEX);
        Serial.print(" is ");
        Serial.println(it->second->messageOpened ? "read" : "unread");
#endif
        if (!it->second->messageOpened)
        {
            return it;
        }
    }

    if (!it->second->messageOpened)
    {
        return it;
    }
    return messages.end();
}

std::map<uint64_t, Message_Base *>::iterator Network_Manager::getBeginIterator()
{
    return messages.begin();
}

std::map<uint64_t, Message_Base *>::iterator Network_Manager::getEndIterator()
{
    return messages.end();
}

void Network_Manager::loadStatusList()
{
    JsonArray Statuses = Network_Manager::statusList.createNestedArray(STATUSES);

    Statuses.add("Meet here");
    Statuses.add("Follow me");
    Statuses.add("Point of Interest");

    // Settings_Manager::writeStatusesToEEPROM(statusList);
}