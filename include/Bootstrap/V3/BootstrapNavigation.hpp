#pragma once

#include "BootstrapMicrocontroller.hpp"

#include "HelperClasses/Compass/CompassV3.hpp"
#include "TinyGPS++.h"

#include "NavigationManager.h"
#include "NavigationUtils.h"

class BootstrapNavigation
{
public:

    static void Initialize()
    {
        ESP_LOGI("NavBoostrap", "Initializing Navigation Module");

        Serial2.setPins(5, 4);
        Serial2.begin(9600);
        NavigationManagerInstance().InitializeUtils(&CompassInstance(), Serial2);
    }

    static NavigationManager &NavigationManagerInstance()
    {
        static NavigationManager navManager;
        return navManager;
    }

    // =================== Hardware =======================

    static CompassV3 &CompassInstance()
    {
        static CompassV3 compass(BootstrapMicrocontroller::ScannedDevices(), BootstrapMicrocontroller::I2cBus());
        return compass;
    }
};