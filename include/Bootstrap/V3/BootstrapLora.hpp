#pragma once

#include "Bootstrap/V3/BootstrapMicrocontroller.hpp"
#include "LoraManager.hpp"
#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"
#include "HelperClasses/PingMessage.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"

namespace
{
    const uint8_t V3_LORA_CS   = 39;
    const uint8_t V3_LORA_RST  = 38;
    const uint8_t V3_LORA_DIO0 = 48;
    const uint8_t V3_LORA_TX   = 23;
}

class BootstrapLora
{
public:
    static void Initialize()
    {
        auto& mgr = Manager();
        if (!mgr.Init())
        {
            ESP_LOGE("BootstrapLora", "Failed to initialize LoRa module");
            return;
        }

        Driver().SetSpreadingFactor(7);
        Driver().SetSignalBandwidth(500E3);
        Driver().SetTXPower(V3_LORA_TX);

        LoraModule::Utilities::RegisterMessageType(PingMessage::GUID, PingMessage::Create);

        WayfinderLoraState::Init();
    }

    static ArduinoLoRaDriver& Driver()
    {
        static ArduinoLoRaDriver driver(&BootstrapMicrocontroller::SpiBus(), V3_LORA_CS, V3_LORA_RST, V3_LORA_DIO0, 915E6);
        return driver;
    }

    static LoraModule::Manager& Manager()
    {
        static LoraModule::Manager manager(&Driver());
        return manager;
    }

    static void RadioTaskRunner(void* pvParameters)
    {
        Manager().RadioTask();
    }

    static void SendQueueTaskRunner(void* pvParameters)
    {
        Manager().SendQueueTask();
    }
};
