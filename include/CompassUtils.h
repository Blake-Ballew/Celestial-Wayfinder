#pragma once

#include "Display_Utils.h"
#include "LoraUtils.h"
#include "FilesystemUtils.h"
#include "LED_Utils.h"
#include "Settings_Manager.h"
#include "Display_Manager.h"
#include "RH_RF95.h"
#include "ArduinoJson.h"
#include "globalDefines.h"

namespace
{
    const char *SETTINGS_FILENAME PROGMEM = "/Settings.msgpk";
}

// Static class to help interface with esp32 utils compass functionality
// Eventually, all windows pertaining to the compass specific functionality will reference this class
class CompassUtils
{
public:

    static uint8_t MessageReceivedInputID;
    static RH_RF95 driver;

    static void PassMessageReceivedToDisplay(uint32_t sendingUserID, bool isNew)
    {
        if (isNew) 
        {
            #if DEBUG == 1
            Serial.println("CompassUtils::PassMessageReceivedToDisplay: New message received");
            #endif
            Display_Utils::sendInputCommand(MessageReceivedInputID);
        }
        #if DEBUG == 1
        else
        {
            Serial.println("CompassUtils::PassMessageReceivedToDisplay: Old message received");
        }
        #endif

        // TODO: Maybe add refresh display command if not new
    }

    static void InitializeSettings()
    {   
        auto returncode = FilesystemUtils::LoadSettingsFile(SETTINGS_FILENAME);

        if (returncode == FilesystemReturnCode::FILE_NOT_FOUND)
        {
            Settings_Manager::readSettingsFromEEPROM();
            FlashSettings(0);
        }

        ProcessSettingsFile();

        FilesystemUtils::SettingsUpdated() += ProcessSettingsFile;
    }

    static void ProcessSettingsFile()
    {
        JsonDocument &doc = FilesystemUtils::SettingsFile();

        if (!doc.isNull())
        {
            // LED Module
            int colorTheme = doc["Color Theme"]["cfgVal"].as<int>();

            uint8_t red = doc["Theme Red"]["cfgVal"].as<uint8_t>();
            uint8_t green = doc["Theme Green"]["cfgVal"].as<uint8_t>();
            uint8_t blue = doc["Theme Blue"]["cfgVal"].as<uint8_t>();

            std::unordered_map<int, CRGB> presetThemeColors = 
            {
                /* Custom */ {0, CRGB(red, green, blue)},
                /* Red */ {1, CRGB(255, 0, 0)},
                /* Green */ {2, CRGB(0, 255, 0)},
                /* Blue */ {3, CRGB(0, 0, 255)},
                /* Purple */ {4, CRGB(255, 0, 255)},
                /* Yellow */ {5, CRGB(255, 255, 0)},
                /* Cyan */ {6, CRGB(0, 255, 255)},
                /* White */ {7, CRGB(255, 255, 255)},
                /* Orange */ {8, CRGB(255, 165, 0)}
            };

            CRGB color;

            if (presetThemeColors.find(colorTheme) != presetThemeColors.end())
            {
                color = presetThemeColors[colorTheme];
            }
            else
            {
                color = CRGB(red, green, blue);
            }

            LED_Utils::setThemeColor(color);

            // Lora Module
            LoraUtils::SetUserID(doc["UserID"]["cfgVal"].as<uint32_t>());
            LoraUtils::SetUserName(doc["User Name"]["cfgVal"].as<std::string>());
            LoraUtils::SetDefaultSendAttempts(doc["Broadcast Attempts"]["cfgVal"].as<uint8_t>());

            float frequency = doc["Frequency"]["cfgVal"].as<float>();
            driver.setFrequency(frequency);

            auto modemConfigIndex = doc["Modem Config"]["cfgVal"].as<size_t>();
            RH_RF95::ModemConfigChoice modemConfig = (RH_RF95::ModemConfigChoice)doc["Modem Config"]["vals"][modemConfigIndex].as<int>();
            driver.setModemConfig(modemConfig);

            // System
            System_Utils::silentMode = doc["Silent Mode"].as<bool>();
            System_Utils::time24Hour = doc["24H Time"].as<bool>();
        }
    }

    static void RegisterCallbacksDisplayManager(Display_Manager *unused)
    {
        Display_Manager::registerCallback(ACTION_FLASH_DEFAULT_SETTINGS, FlashSettings);
    }

    // Callbacks
    static void FlashSettings(uint8_t inputID)
    {
        DynamicJsonDocument doc(2048);
        JsonDocument &oldSettings = Settings_Manager::settings;

        doc["UserID"] = esp_random();

        JsonObject User_Name = doc.createNestedObject("User Name");
        User_Name["cfgType"] = 10;
        User_Name["cfgVal"] = "User";
        User_Name["dftVal"] = "User";
        User_Name["maxLen"] = 12;
        doc["Silent Mode"] = true;

        if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Name"))
        {
            User_Name["cfgVal"] = oldSettings["User"]["Name"]["cfgVal"].as<std::string>();
        }

        JsonObject Color_Theme = doc.createNestedObject("Color Theme");
        Color_Theme["cfgType"] = 11;
        Color_Theme["cfgVal"] = 0;
        Color_Theme["dftVal"] = 0;

        JsonArray Color_Theme_vals = Color_Theme.createNestedArray("vals");
        Color_Theme_vals.add(0);
        Color_Theme_vals.add(1);
        Color_Theme_vals.add(2);
        Color_Theme_vals.add(3);
        Color_Theme_vals.add(4);
        Color_Theme_vals.add(5);
        Color_Theme_vals.add(6);
        Color_Theme_vals.add(7);
        Color_Theme_vals.add(8);

        JsonArray Color_Theme_valTxt = Color_Theme.createNestedArray("valTxt");
        Color_Theme_valTxt.add("Custom");
        Color_Theme_valTxt.add("Red");
        Color_Theme_valTxt.add("Green");
        Color_Theme_valTxt.add("Blue");
        Color_Theme_valTxt.add("Purple");
        Color_Theme_valTxt.add("Yellow");
        Color_Theme_valTxt.add("Cyan");
        Color_Theme_valTxt.add("White");
        Color_Theme_valTxt.add("Orange");

        JsonObject Theme_Red = doc.createNestedObject("Theme Red");
        Theme_Red["cfgType"] = 8;
        Theme_Red["cfgVal"] = 0;
        Theme_Red["dftVal"] = 0;
        Theme_Red["maxVal"] = 255;
        Theme_Red["minVal"] = 0;
        Theme_Red["incVal"] = 1;
        Theme_Red["signed"] = false;

        if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Theme Red"))
        {
            Theme_Red["cfgVal"] = oldSettings["User"]["Theme Red"]["cfgVal"].as<uint8_t>();
        }

        JsonObject Theme_Green = doc.createNestedObject("Theme Green");
        Theme_Green["cfgType"] = 8;
        Theme_Green["cfgVal"] = 255;
        Theme_Green["dftVal"] = 255;
        Theme_Green["maxVal"] = 255;
        Theme_Green["minVal"] = 0;
        Theme_Green["incVal"] = 1;
        Theme_Green["signed"] = false;

        if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Theme Green"))
        {
            Theme_Green["cfgVal"] = oldSettings["User"]["Theme Green"]["cfgVal"].as<uint8_t>();
        }

        JsonObject Theme_Blue = doc.createNestedObject("Theme Blue");
        Theme_Blue["cfgType"] = 8;
        Theme_Blue["cfgVal"] = 0;
        Theme_Blue["dftVal"] = 0;
        Theme_Blue["maxVal"] = 255;
        Theme_Blue["minVal"] = 0;
        Theme_Blue["incVal"] = 1;
        Theme_Blue["signed"] = false;

        if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Theme Blue"))
        {
            Theme_Blue["cfgVal"] = oldSettings["User"]["Theme Blue"]["cfgVal"].as<uint8_t>();
        }   

        JsonObject Frequency = doc.createNestedObject("Frequency");
        Frequency["cfgType"] = 9;
        Frequency["cfgVal"] = 914.9;
        Frequency["dftVal"] = 914.9;
        Frequency["maxVal"] = 914.9;
        Frequency["minVal"] = 902.3;
        Frequency["incVal"] = 0.2;

        if (oldSettings.containsKey("Radio") && oldSettings["Radio"].containsKey("Frequency"))
        {
            Frequency["cfgVal"] = oldSettings["Radio"]["Frequency"]["cfgVal"].as<float>();
        }

        JsonObject Modem_Config = doc.createNestedObject("Modem Config");
        Modem_Config["cfgType"] = 11;
        Modem_Config["cfgVal"] = 4;
        Modem_Config["dftVal"] = 4;

        if (oldSettings.containsKey("Radio") && oldSettings["Radio"].containsKey("Modem Config"))
        {
            Modem_Config["cfgVal"] = oldSettings["Radio"]["Modem Config"]["cfgVal"].as<uint8_t>();
        }

        JsonArray Modem_Config_vals = Modem_Config.createNestedArray("vals");
        Modem_Config_vals.add(0);
        Modem_Config_vals.add(1);
        Modem_Config_vals.add(2);
        Modem_Config_vals.add(3);
        Modem_Config_vals.add(4);

        JsonArray Modem_Config_valTxt = Modem_Config.createNestedArray("valTxt");
        Modem_Config_valTxt.add("125 kHz, 4/5, 128");
        Modem_Config_valTxt.add("500 kHz, 4/5, 128");
        Modem_Config_valTxt.add("31.25 kHz, 4/8, 512");
        Modem_Config_valTxt.add("125 kHz, 4/8, 4096");
        Modem_Config_valTxt.add("125 khz, 4/5, 2048");

        JsonObject Broadcast_Attempts = doc.createNestedObject("Broadcast Attempts");
        Broadcast_Attempts["cfgType"] = 8;
        Broadcast_Attempts["cfgVal"] = 3;
        Broadcast_Attempts["dftVal"] = 3;
        Broadcast_Attempts["maxVal"] = 5;
        Broadcast_Attempts["minVal"] = 1;
        Broadcast_Attempts["incVal"] = 1;
        Broadcast_Attempts["signed"] = false;

        if (oldSettings.containsKey("Radio") && oldSettings["Radio"].containsKey("Broadcast Retries"))
        {
            Broadcast_Attempts["cfgVal"] = oldSettings["Radio"]["Broadcast Retries"]["cfgVal"].as<uint8_t>();
        }

        doc["24H Time"] = false;

        FilesystemUtils::WriteSettingsFile(SETTINGS_FILENAME, doc);
    }
};