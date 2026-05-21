#pragma once

#include "Bootstrap/V2/BootstrapMicrocontroller.hpp"
#include "LoraManager.hpp"
#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"
#include "HelperClasses/PingMessage.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"

namespace
{
    const uint8_t V2_LORA_CS   = 15;
    const int     V2_LORA_RST  = -1;
    const uint8_t V2_LORA_DIO0 = 18;
    const uint8_t V2_LORA_TX   = 23;
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
        Driver().SetTXPower(V2_LORA_TX);

        LoraModule::Utilities::RegisterMessageType(PingMessage::GUID, PingMessage::Create);

        WayfinderLoraState::Init();
    }

    static ArduinoLoRaDriver& Driver()
    {
        static ArduinoLoRaDriver driver(&BootstrapMicrocontroller::SpiBus(), V2_LORA_CS, V2_LORA_RST, V2_LORA_DIO0, 915E6);
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
