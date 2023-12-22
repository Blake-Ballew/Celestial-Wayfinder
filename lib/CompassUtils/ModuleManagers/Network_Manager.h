#pragma once

#include <Message_Types.h>
// #include <UserID.h>
#include <Settings_Manager.h>
#include <globalDefines.h>
#include <RH_RF95.h>
#include <RHMesh.h>
#include <RHSoftwareSPI.h>
#include <RHHardwareSPI.h>
#include <Map>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "EventDeclarations.h"

#define RH_DRIVER RH_RF95
#define RFM95_CS 15
#define RFM95_Int 18
#define RF95_FREQ 915.0
#define RF95_MODEM_CONFIG RH_RF95::Bw125Cr45Sf128
#define RF95_TX_PWR 23

#define RF95_SPI_SCK 14
#define RF95_SPI_MISO 12
#define RF95_SPI_MOSI 13

#define DEFAULT_NODE_ID 1

#define MESSAGE_QUEUE_MAX 8
#define RETURN_CODE_UNABLE_TO_QUEUE 255
#define MESSAGE_QUEUE_TIMEOUT 15000

struct MessageQueueItem
{
    Message_Base *msg;
    bool isBroadcast;
    uint64_t user;
    uint8_t *returnCode;
};

class Network_Manager
{
public:
    static RHMesh *manager;
    static RH_DRIVER driver;
    static RHHardwareSPI rf_spi;

    static std::map<uint8_t, uint64_t> nodeIDs;
    static std::map<uint64_t, Message_Base *> messages;
    static std::map<uint64_t, Message_Base *> messagesSent;

    static QueueHandle_t messageQueue;
    static StaticQueue_t messageQueueBuffer;

    static Message_Base *lastBroadcast;

    static uint8_t buffer[RH_MESH_MAX_MESSAGE_LEN];

    static TaskHandle_t *taskHandle;

    // Implement later. Software defined channel hopping. Only store mesh nodes on the same channel
    // static uint8_t channel;

    // Essentially an IP address
    static uint8_t nodeID;

    // User ID. Will eventually be used as a public key
    static uint64_t userID;

    static bool init(TaskHandle_t *taskHandle);

    static uint8_t sendBroadcastMessage(Message_Base *msg);
    static uint8_t sendMessageToUser(uint64_t user, Message_Base *msg);

    static uint8_t queueMessageToUser(uint64_t user, Message_Base *msg);
    static uint8_t queueBroadcastMessage(Message_Base *msg);

    static void listenForMessages(void *taskParams);

    static uint8_t findFreeNodeID();
    static Message_Base *findMessageByIdx(uint16_t idx);

    static ArduinoJson::JsonArray getStatusList();
    static size_t getNumUnreadMessages();

    static std::map<uint64_t, Message_Base *>::iterator getUnreadBegin();
    static std::map<uint64_t, Message_Base *>::iterator getUnreadEnd();
    static std::map<uint64_t, Message_Base *>::iterator incrementUnreadIterator(std::map<uint64_t, Message_Base *>::iterator it);
    static std::map<uint64_t, Message_Base *>::iterator decrementUnreadIterator(std::map<uint64_t, Message_Base *>::iterator it);
    static std::map<uint64_t, Message_Base *>::iterator getBeginIterator();
    static std::map<uint64_t, Message_Base *>::iterator getEndIterator();

    static void markMessageAsRead(uint64_t user);
    static void loadStatusList();

private:
    static uint8_t findNodeIDofUser(uint64_t user);

    static ArduinoJson::DynamicJsonDocument statusList;
    static uint8_t messageDataBuffer[];
    static uint8_t numRetries;
};
