#pragma once

#include "RpcManager.h"
#include "CompassUtils.h"

class BootstrapRpc
{
public:

    static void Initialize()
    {
        ESP_LOGI("BootstrapRpc", "Initializing RPC...");

        CompassUtils::InitializeRpc(1, BootstrapMicrocontroller::CPU_CORE_LORA);
    }
};