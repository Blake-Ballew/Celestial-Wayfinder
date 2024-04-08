#include "EventDeclarations.h"
#include "globalDefines.h"
#include "LED_Manager.h"
#include "OLED_Manager.h"

TaskHandle_t inputTaskHandle;
TaskHandle_t radioReadTaskHandle;

ESP32Encoder *inputEncoder;

void IRAM_ATTR
button1ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint32_t)EVENT_BUTTON_1);
    xTaskNotifyFromISR(inputTaskHandle, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button2ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint32_t)EVENT_BUTTON_2);
    xTaskNotifyFromISR(inputTaskHandle, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button3ISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint32_t)EVENT_BUTTON_3);
    xTaskNotifyFromISR(inputTaskHandle, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button4ISR()
{
#if DEBUG == 1
    // Serial.println("button4ISR");
#endif
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint32_t)EVENT_BUTTON_4);
    xTaskNotifyFromISR(inputTaskHandle, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR buttonSOSISR()
{
#if DEBUG == 1
    // Serial.println("button4ISR");
#endif
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint32_t)EVENT_BUTTON_SOS);
    xTaskNotifyFromISR(inputTaskHandle, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

/*void IRAM_ATTR encoderUpISR(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint8_t)EVENT_ENCODER_UP);
    xTaskNotifyFromISR(NULL, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR encoderDownISR(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t notification = BIT_SHIFT((uint8_t)EVENT_ENCODER_DOWN);
    xTaskNotifyFromISR(NULL, notification, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}*/

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
        uint32_t notification = 0;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (currCount > prevCount)
        {
#if DEBUG == 1
            Serial.println("enc_cb: down");
#endif
            notification = BIT_SHIFT((uint32_t)EVENT_ENCODER_DOWN);
        }
        else if (currCount < prevCount)
        {
#if DEBUG == 1
            Serial.println("enc_cb: up");
#endif
            notification = BIT_SHIFT((uint32_t)EVENT_ENCODER_UP);
        }
        prevCount = currCount;
        xTaskNotifyFromISR(inputTaskHandle, notification, eSetBits, &xHigherPriorityTaskWoken);
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