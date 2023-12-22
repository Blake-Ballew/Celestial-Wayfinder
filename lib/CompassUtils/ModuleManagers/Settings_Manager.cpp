#include "Settings_Manager.h"

ArduinoJson::DynamicJsonDocument Settings_Manager::settings = ArduinoJson::DynamicJsonDocument(SIZE_SETTINGS_OBJECT);
// EepromStream Settings_Manager::eepromStream = EepromStream(EEPROM_SETTINGS_ADDR, EEPROM_SETTINGS_SIZE);

void Settings_Manager::init()
{
    EEPROM.begin(SIZE_SETTINGS_OBJECT);
    // #if UPLOAD_SETTINGS != 1
    readSettingsFromEEPROM();
    // #endif
}

bool Settings_Manager::readSettingsFromEEPROM()
{
#if DEBUG == 1
    Serial.println();
    Serial.println("Reading settings from EEPROM");
#endif
    EepromStream eepromStream = EepromStream(EEPROM_SETTINGS_ADDR, SIZE_SETTINGS_OBJECT);
    deserializeMsgPack(settings, eepromStream);
    settings.shrinkToFit();
    return true;
}

bool Settings_Manager::writeSettingsToEEPROM()
{
    EepromStream eepromStream = EepromStream(EEPROM_SETTINGS_ADDR, SIZE_SETTINGS_OBJECT);
    serializeMsgPack(settings, eepromStream);
    eepromStream.flush();
    return true;
}

bool Settings_Manager::readSettingsFromSerial()
{
    if (Serial.available() == 0)
    {
        return false;
    }
    while (Serial.available() > 0)
    {
        deserializeJson(settings, Serial);
    }
    settings.shrinkToFit();
    return true;
}

bool Settings_Manager::writeSettingsToSerial()
{
    if (Serial.availableForWrite() == 0)
    {
        return false;
    }
    serializeJson(settings, Serial);
    Serial.println();
    return true;
}

bool Settings_Manager::readStatusesFromEEPROM(ArduinoJson::JsonDocument &doc)
{
    EepromStream statusStream = EepromStream(EEPROM_STATUSES_ADDR, SIZE_STATUSES_OBJECT);
    deserializeMsgPack(doc, statusStream);
    return true;
}

bool Settings_Manager::writeStatusesToEEPROM(ArduinoJson::JsonDocument &doc)
{
    EepromStream statusStream = EepromStream(EEPROM_STATUSES_ADDR, SIZE_STATUSES_OBJECT);
    serializeMsgPack(doc, statusStream);
    statusStream.flush();
    return true;
}

void Settings_Manager::flashSettings()
{

    DynamicJsonDocument doc(SIZE_SETTINGS_OBJECT);
    EepromStream eepromStream = EepromStream(EEPROM_SETTINGS_ADDR, SIZE_SETTINGS_OBJECT);
    JsonObject User = doc.createNestedObject("User");

    JsonArray User_UserID = User.createNestedArray("UserID");
    for (int i = 0; i < 8; i++)
    {
        // Fill with random bytes with esp random function
        User_UserID.add(esp_random() % 256);
    }

    JsonObject User_Name = User.createNestedObject("Name");
    User_Name["cfgType"] = 10;
    User_Name["cfgVal"] = "User";
    User_Name["dftVal"] = "User";
    User_Name["maxLen"] = 12;
    doc["Device"]["24HTime"] = false;

    JsonObject User_Theme_Red = User.createNestedObject("Theme Red");
    User_Theme_Red["cfgType"] = 8;
    User_Theme_Red["cfgVal"] = 0;
    User_Theme_Red["dftVal"] = 0;
    User_Theme_Red["maxVal"] = 255;
    User_Theme_Red["minVal"] = 0;
    User_Theme_Red["incVal"] = 5;
    User_Theme_Red["signed"] = false;

    JsonObject User_Theme_Green = User.createNestedObject("Theme Green");
    User_Theme_Green["cfgType"] = 8;
    User_Theme_Green["cfgVal"] = 255;
    User_Theme_Green["dftVal"] = 255;
    User_Theme_Green["maxVal"] = 255;
    User_Theme_Green["minVal"] = 0;
    User_Theme_Green["incVal"] = 5;
    User_Theme_Green["signed"] = false;

    JsonObject User_Theme_Blue = User.createNestedObject("Theme Blue");
    User_Theme_Blue["cfgType"] = 8;
    User_Theme_Blue["cfgVal"] = 0;
    User_Theme_Blue["dftVal"] = 0;
    User_Theme_Blue["maxVal"] = 255;
    User_Theme_Blue["minVal"] = 0;
    User_Theme_Blue["incVal"] = 5;
    User_Theme_Blue["signed"] = false;
    doc["Device"]["24HTime"] = false;

    JsonObject Radio = doc.createNestedObject("Radio");

    JsonObject Radio_Frequency = Radio.createNestedObject("Frequency");
    Radio_Frequency["cfgType"] = 9;
    Radio_Frequency["cfgVal"] = 914.9;
    Radio_Frequency["dftVal"] = 914.9;
    Radio_Frequency["maxVal"] = 914.9;
    Radio_Frequency["minVal"] = 902.3;
    Radio_Frequency["incVal"] = 0.2;

    JsonObject Radio_NodeID = Radio.createNestedObject("NodeID");
    Radio_NodeID["cfgType"] = 8;
    Radio_NodeID["cfgVal"] = (ESP.getEfuseMac() & 0xFF) == 255 ? 254 : (ESP.getEfuseMac() & 0xFF);
    Radio_NodeID["dftVal"] = (ESP.getEfuseMac() & 0xFF) == 255 ? 254 : (ESP.getEfuseMac() & 0xFF);
    Radio_NodeID["maxVal"] = 254;
    Radio_NodeID["minVal"] = 0;
    Radio_NodeID["incVal"] = 1;
    Radio_NodeID["signed"] = false;

    JsonObject Radio_Modem_Config = Radio.createNestedObject("Modem Config");
    Radio_Modem_Config["cfgType"] = 11;
    Radio_Modem_Config["cfgVal"] = 4;
    Radio_Modem_Config["dftVal"] = 4;

    JsonArray Radio_Modem_Config_vals = Radio_Modem_Config.createNestedArray("vals");
    Radio_Modem_Config_vals.add(0);
    Radio_Modem_Config_vals.add(1);
    Radio_Modem_Config_vals.add(2);
    Radio_Modem_Config_vals.add(3);
    Radio_Modem_Config_vals.add(4);

    JsonArray Radio_Modem_Config_valTxt = Radio_Modem_Config.createNestedArray("valTxt");
    Radio_Modem_Config_valTxt.add("125 kHz, 4/5, 128");
    Radio_Modem_Config_valTxt.add("500 kHz, 4/5, 128");
    Radio_Modem_Config_valTxt.add("31.25 kHz, 4/8, 512");
    Radio_Modem_Config_valTxt.add("125 kHz, 4/8, 4096");
    Radio_Modem_Config_valTxt.add("125 khz, 4/5, 2048");
    Radio["Router"] = false;

    JsonObject Radio_Broadcast_Retries = Radio.createNestedObject("Broadcast Retries");
    Radio_Broadcast_Retries["cfgType"] = 8;
    Radio_Broadcast_Retries["cfgVal"] = 3;
    Radio_Broadcast_Retries["dftVal"] = 3;
    Radio_Broadcast_Retries["maxVal"] = 5;
    Radio_Broadcast_Retries["minVal"] = 1;
    Radio_Broadcast_Retries["incVal"] = 1;
    Radio_Broadcast_Retries["signed"] = false;
    Radio["Router"] = false;

    JsonObject Compass = doc.createNestedObject("Compass");
    Compass["useCalibration"] = false;
    Compass["X_MIN"] = 0;
    Compass["X_MAX"] = 0;
    Compass["Y_MIN"] = 0;
    Compass["Y_MAX"] = 0;
    Compass["Z_MIN"] = 0;
    Compass["Z_MAX"] = 0;
    doc["GPS"]["refreshDelayms"] = 5000;

    serializeMsgPack(doc, Serial);
    serializeMsgPack(doc, eepromStream);

    // ========================== End Generated Code ==========================

    eepromStream.flush();
    doc.clear();
}
