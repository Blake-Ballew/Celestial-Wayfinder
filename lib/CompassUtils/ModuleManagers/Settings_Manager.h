#pragma once

#include "globalDefines.h"
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <StreamUtils.h>

class Settings_Manager
{
public:
    static ArduinoJson::DynamicJsonDocument settings;
    // static EepromStream eepromStream;

    static void init();

    static bool readSettingsFromEEPROM();
    static bool writeSettingsToEEPROM();

    static bool readSettingsFromSerial();
    static bool writeSettingsToSerial();

    static bool readStatusesFromEEPROM(ArduinoJson::JsonDocument &doc);
    static bool writeStatusesToEEPROM(ArduinoJson::JsonDocument &doc);

    static void flashSettings();

};
