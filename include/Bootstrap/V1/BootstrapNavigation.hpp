#pragma once

#include "HelperClasses/Compass/CompassV1.hpp"
#include "TinyGPS++.h"

#include "NavigationManager.h"
#include "GpsTimeSource.hpp"
#include "EzTimeSource.hpp"
#include "GpsGeolocationSource.hpp"
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

        auto gpsTime = new GpsTimeSource(NavigationUtils::GetGPS());
        auto ezTime = new EzTimeSource();
        System_Utils::TimeSources().push_back(gpsTime);
        System_Utils::TimeSources().push_back(ezTime);
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

    static NavigationModule::GpsGeolocationSource &GpsLocator()
    {
        static NavigationModule::GpsGeolocationSource gpsLocator(NavigationModule::Utilities::GetGPS());
        return gpsLocator;
    }
};