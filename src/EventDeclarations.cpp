#include "EventDeclarations.h"
#include "globalDefines.h"
#include "LED_Manager.h"
#include "OLED_Manager.h"

TaskHandle_t inputTaskHandle;
TaskHandle_t radioReadTaskHandle;
QueueHandle_t displayCommandQueue;

ESP32Encoder *inputEncoder;

void IRAM_ATTR button1ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayCommandQueueItem command;
    command.commandType = INPUT_COMMAND;
    command.source = USER_INPUT;
    command.commandData.inputCommand.inputID = BUTTON_1;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button2ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayCommandQueueItem command;
    command.commandType = INPUT_COMMAND;
    command.source = USER_INPUT;
    command.commandData.inputCommand.inputID = BUTTON_2;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button3ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayCommandQueueItem command;
    command.commandType = INPUT_COMMAND;
    command.source = USER_INPUT;
    command.commandData.inputCommand.inputID = BUTTON_3;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button4ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayCommandQueueItem command;
    command.commandType = INPUT_COMMAND;
    command.source = USER_INPUT;
    command.commandData.inputCommand.inputID = BUTTON_4;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR buttonSOSISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayCommandQueueItem command;
    command.commandType = INPUT_COMMAND;
    command.source = USER_INPUT;
    command.commandData.inputCommand.inputID = BUTTON_SOS;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR enc_cb(void *arg)
{
    static int64_t prevCount = 0;
    ESP32Encoder *enc = ESP32Encoder::encoders[0];
    /*    if (enc == NULL)
        {
    #if DEBUG == 1
            Serial.println("enc_cb: enc is NULL");
    #endif
            return;
        }
    */
    int64_t currCount = enc->getCount();
    if (currCount % 4 == 0)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        DisplayCommandQueueItem command;
        command.commandType = INPUT_COMMAND;
        command.source = USER_INPUT;
        if (currCount > prevCount)
        {
#if DEBUG == 1
            Serial.println("enc_cb: down");
#endif
            command.commandData.inputCommand.inputID = ENC_DOWN;
        }
        else if (currCount < prevCount)
        {
#if DEBUG == 1
            Serial.println("enc_cb: up");
#endif
            command.commandData.inputCommand.inputID = ENC_UP;
        }
        prevCount = currCount;

        xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);

        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR();
        }
    }
}

void IRAM_ATTR CompassDRDYISR()
{
#if DEBUG == 1
    Serial.println("CompassDRDYISR");
#endif
    Navigation_Manager::read();
}

void enableInterrupts()
{
    attachInterrupt(BUTTON_SOS_PIN, buttonSOSISR, FALLING);
    attachInterrupt(BUTTON_1_PIN, button1ISR, FALLING);
    attachInterrupt(BUTTON_2_PIN, button2ISR, FALLING);
    attachInterrupt(BUTTON_3_PIN, button3ISR, FALLING);
    attachInterrupt(BUTTON_4_PIN, button4ISR, FALLING);

    inputEncoder->attachFullQuad(ENC_A, ENC_B);
    inputEncoder->setFilter(1023);
}

void disableInterrupts()
{
    detachInterrupt(BUTTON_1_PIN);
    detachInterrupt(BUTTON_2_PIN);
    detachInterrupt(BUTTON_3_PIN);
    detachInterrupt(BUTTON_4_PIN);
    detachInterrupt(BUTTON_SOS_PIN);

    inputEncoder->detatch();
}