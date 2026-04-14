#pragma once

#include "HelperClasses/Compass/QMC5883L.h"
#include "TinyGPS++.h"

#include "NavigationManager.h"
#include "NavigationUtils.h"

class BootstrapNavigation
{
public:

    static void Initialize()
    {
        ESP_LOGI("NavBoostrap", "Initializing Navigation Module");
        CompassInstance().SetInvertX(true);

        Serial2.begin(9600);
        NavigationManagerInstance().InitializeUtils(&CompassInstance(), Serial2);
    }

    static NavigationManager &NavigationManagerInstance()
    {
        static NavigationManager navManager;
        return navManager;
    }

    // =================== Hardware =======================

    static QMC5883L &CompassInstance()
    {
        static QMC5883L compass;
        return compass;
    }
};