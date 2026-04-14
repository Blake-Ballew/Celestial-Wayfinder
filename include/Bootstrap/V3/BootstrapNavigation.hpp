#pragma once

#include "HelperClasses/Compass/LSM303AGR.h"
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

    static LSM303AGR &CompassInstance()
    {
        static LSM303AGR compass;
        return compass;
    }
};