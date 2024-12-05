#pragma once

#include "Display_Utils.h"
#include "LoraManager.h"
#include "FilesystemUtils.h"
#include "RpcUtils.h"
#include "LED_Utils.h"
#include "Settings_Manager.h"
#include "Display_Manager.h"
#include "RpcManager.h"

#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"

#include "ArduinoJson.h"
#include "globalDefines.h"

namespace
{
    const char *SETTINGS_FILENAME PROGMEM = "/Settings.msgpk";
    const char *OLD_SETTINGS_FILENAME PROGMEM = "/settings.json";
    static RpcModule::Manager RpcManagerInstance;
}

// Static class to help interface with esp32 utils compass functionality
// Eventually, all windows pertaining to the compass specific functionality will reference this class
class CompassUtils
{
public:

    static uint8_t MessageReceivedInputID;
    static ArduinoLoRaDriver ArduinoLora;

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

        #if DEBUG == 1
        Serial.print("CompassUtils::InitializeSettings: LoadSettingsFile returned ");
        Serial.println(returncode);
        #endif

        if (returncode == FilesystemReturnCode::FILE_NOT_FOUND)
        {
            #if DEBUG == 1
            Serial.println("CompassUtils::InitializeSettings: Settings file not found. Creating new settings file.");
            #endif
            FlashSettings(0);

            // Load old settings file and try to import to new settings
            DynamicJsonDocument oldSettings(2048);

            returncode = FilesystemUtils::ReadFile(OLD_SETTINGS_FILENAME, oldSettings);
            #if DEBUG == 1
            Serial.print("CompassUtils::InitializeSettings: ReadFile returned ");
            Serial.println(returncode);
            #endif

            if (returncode == FilesystemReturnCode::FILESYSTEM_OK)
            {
                JsonDocument &doc = FilesystemUtils::SettingsFile();

                if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("UserID"))
                {
                    doc["UserID"] = oldSettings["User"]["UserID"].as<uint32_t>();
                }

                if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Name"))
                {
                    doc["User Name"]["cfgVal"] = oldSettings["User"]["Name"]["cfgVal"].as<std::string>();
                }

                if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Theme Red"))
                {
                    doc["Theme Red"]["cfgVal"] = oldSettings["User"]["Theme Red"]["cfgVal"].as<uint8_t>();
                }

                if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Theme Green"))
                {
                    doc["Theme Green"]["cfgVal"] = oldSettings["User"]["Theme Green"]["cfgVal"].as<uint8_t>();
                }

                if (oldSettings.containsKey("User") && oldSettings["User"].containsKey("Theme Blue"))
                {
                    doc["Theme Blue"]["cfgVal"] = oldSettings["User"]["Theme Blue"]["cfgVal"].as<uint8_t>();
                }
          
                if (oldSettings.containsKey("Radio") && oldSettings["Radio"].containsKey("Frequency"))
                {
                    doc["Frequency"]["cfgVal"] = oldSettings["Radio"]["Frequency"]["cfgVal"].as<float>();
                }

                if (oldSettings.containsKey("Radio") && oldSettings["Radio"].containsKey("Modem Config"))
                {
                    doc["Modem Config"]["cfgVal"] = oldSettings["Radio"]["Modem Config"]["cfgVal"].as<uint8_t>();
                }

                if (oldSettings.containsKey("Radio") && oldSettings["Radio"].containsKey("Broadcast Retries"))
                {
                    doc["Broadcast Attempts"]["cfgVal"] = oldSettings["Radio"]["Broadcast Retries"]["cfgVal"].as<uint8_t>();
                }

                auto writeReturnCode = FilesystemUtils::WriteSettingsFile(SETTINGS_FILENAME, doc);

                #if DEBUG == 1
                Serial.print("CompassUtils::InitializeSettings: WriteSettingsFile returned ");
                Serial.println(writeReturnCode);
                #endif
            }
        }

        #if DEBUG == 1
        Serial.println("CompassUtils::InitializeSettings: Done");
        #endif

        ProcessSettingsFile();

        FilesystemUtils::SettingsUpdated() += ProcessSettingsFile;
    }

    static void ProcessSettingsFile()
    {
        JsonDocument &doc = FilesystemUtils::SettingsFile();
        #if DEBUG == 1
        Serial.println("CompassUtils::ProcessSettingsFile");
        serializeJson(doc, Serial);
        Serial.println();
        #endif

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
                /* Red */ {1, CRGB(CRGB::HTMLColorCode::Red)},
                /* Green */ {2, CRGB(CRGB::HTMLColorCode::Green)},
                /* Blue */ {3, CRGB(CRGB::HTMLColorCode::Blue)},
                /* Purple */ {4, CRGB(CRGB::HTMLColorCode::Purple)},
                /* Yellow */ {5, CRGB(CRGB::HTMLColorCode::Yellow)},
                /* Cyan */ {6, CRGB(CRGB::HTMLColorCode::Cyan)},
                /* White */ {7, CRGB(255, 255, 255)},
                /* Orange */ {8, CRGB(CRGB::HTMLColorCode::Orange)},
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
            #if DEBUG == 1
            auto interfaceColor = LED_Pattern_Interface::ThemeColor();
            Serial.print("LED Interface::ThemeColor: ");
            Serial.print(interfaceColor.r);
            Serial.print(", ");
            Serial.print(interfaceColor.g);
            Serial.print(", ");
            Serial.println(interfaceColor.b);
            #endif

            // Lora Module
            LoraUtils::SetUserID(doc["UserID"].as<uint32_t>());
            LoraUtils::SetUserName(doc["User Name"]["cfgVal"].as<std::string>());
            LoraUtils::SetDefaultSendAttempts(doc["Broadcast Attempts"]["cfgVal"].as<uint8_t>());

            float frequency = doc["Frequency"]["cfgVal"].as<float>();
            // ArduinoLora.SetFrequency(frequency);
            ArduinoLora.SetSpreadingFactor(7);
            ArduinoLora.SetSignalBandwidth(125E3);

            #if HARDWARE_VERSION == 1
            ArduinoLora.SetTXPower(20);
            #endif

            #if HARDWARE_VERSION == 2
            ArduinoLora.SetTXPower(23);
            #endif

            // System
            System_Utils::silentMode = doc["Silent Mode"].as<bool>();
            System_Utils::time24Hour = doc["24H Time"].as<bool>();

            #if DEBUG == 1
            Serial.println("CompassUtils::ProcessSettingsFile: Done");
            #endif
        }
    }

    static void RegisterCallbacksDisplayManager(Display_Manager *unused)
    {
        Display_Manager::registerCallback(ACTION_FLASH_DEFAULT_SETTINGS, FlashSettings);
        Display_Manager::registerCallback(ACTION_FLASH_LOCATIONS, FlashCampLocations);
        Display_Manager::registerCallback(ACTION_FLASH_MESSAGES, FlashMessages);
        Display_Manager::registerCallback(ACTION_CLEAR_LOCATIONS, ClearLocations);
        Display_Manager::registerCallback(ACTION_CLEAR_MESSAGES, ClearMessages);

        Display_Utils::UpdateDisplay() += UpdateDisplay;
    }

    static void InitializeRpc(size_t rpcTaskPriority, size_t rpcTaskCore)
    {
        RpcManagerInstance.Init(rpcTaskPriority, rpcTaskCore);

        RegisterRpcFunctions();
    }

    static void RegisterRpcFunctions()
    {
        // Saved Locations
        RpcModule::Utilities::RegisterRpc("AddSavedLocation", NavigationUtils::RpcAddSavedLocation);
        RpcModule::Utilities::RegisterRpc("AddSavedLocations", NavigationUtils::RpcAddSavedLocations);
        RpcModule::Utilities::RegisterRpc("RemoveSavedLocation", NavigationUtils::RpcRemoveSavedLocation);
        RpcModule::Utilities::RegisterRpc("ClearSavedLocations", NavigationUtils::RpcClearSavedLocations);
        RpcModule::Utilities::RegisterRpc("UpdateSavedLocation", NavigationUtils::RpcUpdateSavedLocation);
        RpcModule::Utilities::RegisterRpc("GetSavedLocation", NavigationUtils::RpcGetSavedLocation);
        RpcModule::Utilities::RegisterRpc("GetSavedLocations", NavigationUtils::RpcGetSavedLocations);

        // Saved Messages
        RpcModule::Utilities::RegisterRpc("AddSavedMessage", LoraUtils::RpcAddSavedMessage);
        RpcModule::Utilities::RegisterRpc("AddSavedMessages", LoraUtils::RpcAddSavedMessages);
        RpcModule::Utilities::RegisterRpc("DeleteSavedMessage", LoraUtils::RpcDeleteSavedMessage);
        RpcModule::Utilities::RegisterRpc("DeleteSavedMessages", LoraUtils::RpcDeleteSavedMessages);
        RpcModule::Utilities::RegisterRpc("GetSavedMessage", LoraUtils::RpcGetSavedMessage);
        RpcModule::Utilities::RegisterRpc("GetSavedMessages", LoraUtils::RpcGetSavedMessages);
        RpcModule::Utilities::RegisterRpc("UpdateSavedMessage", LoraUtils::RpcUpdateSavedMessage);

        
    }

    static void UpdateDisplay()
    {
        Display_Manager::display.display();
    }

    // Callbacks
    static void FlashSettings(uint8_t inputID)
    {
        DynamicJsonDocument doc(2048);
        JsonDocument &oldSettings = Settings_Manager::settings;

        uint32_t userID = esp_random();
        doc["UserID"] = userID;

        // Default username is "User_xxxx" where xxxx is the last 2 bytes of the user ID in hex
        char usernamebuffer[10];
        sprintf(usernamebuffer, "User_%04X", userID & 0xFFFF);
        std::string username = usernamebuffer;

        JsonObject User_Name = doc.createNestedObject("User Name");
        User_Name["cfgType"] = 10;
        User_Name["cfgVal"] = usernamebuffer;
        User_Name["dftVal"] = usernamebuffer;
        User_Name["maxLen"] = 12;
        doc["Silent Mode"] = false;

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

        JsonObject Theme_Green = doc.createNestedObject("Theme Green");
        Theme_Green["cfgType"] = 8;
        Theme_Green["cfgVal"] = 255;
        Theme_Green["dftVal"] = 255;
        Theme_Green["maxVal"] = 255;
        Theme_Green["minVal"] = 0;
        Theme_Green["incVal"] = 1;
        Theme_Green["signed"] = false;

        JsonObject Theme_Blue = doc.createNestedObject("Theme Blue");
        Theme_Blue["cfgType"] = 8;
        Theme_Blue["cfgVal"] = 0;
        Theme_Blue["dftVal"] = 0;
        Theme_Blue["maxVal"] = 255;
        Theme_Blue["minVal"] = 0;
        Theme_Blue["incVal"] = 1;
        Theme_Blue["signed"] = false;

        JsonObject Frequency = doc.createNestedObject("Frequency");
        Frequency["cfgType"] = 9;
        Frequency["cfgVal"] = 914.9;
        Frequency["dftVal"] = 914.9;
        Frequency["maxVal"] = 914.9;
        Frequency["minVal"] = 902.3;
        Frequency["incVal"] = 0.2;

        JsonObject Modem_Config = doc.createNestedObject("Modem Config");
        Modem_Config["cfgType"] = 11;
        Modem_Config["cfgVal"] = 1;
        Modem_Config["dftVal"] = 1 ;

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

        doc["24H Time"] = false;

        auto returncode = FilesystemUtils::WriteSettingsFile(SETTINGS_FILENAME, doc);
        #if DEBUG == 1
        Serial.print("CompassUtils::FlashSettings: ");
        Serial.println(returncode);
        #endif  

        if (inputID != 0)
        {
            Display_Utils::clearDisplay();
            Display_Utils::printCenteredText("Settings Flashed!");
            Display_Utils::UpdateDisplay().Invoke();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    static void FlashCampLocations(uint8_t inputID)
    {
        Display_Utils::clearDisplay();
        Display_Utils::printCenteredText("Flashing Locations...");
        Display_Utils::UpdateDisplay().Invoke();

        // No defaut locations exist currently

        Display_Utils::clearDisplay();
        Display_Utils::printCenteredText("Locations Flashed!");
        Display_Utils::UpdateDisplay().Invoke();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    static void FlashImportantLocations(uint8_t inputID)
    {

    }

    static void FlashMessages(uint8_t inputID)
    {
        Display_Utils::clearDisplay();
        Display_Utils::printCenteredText("Flashing Messages...");
        Display_Utils::UpdateDisplay().Invoke();


        LoraUtils::AddSavedMessage("Meet here", false);
        LoraUtils::AddSavedMessage("Point of Interest", false);
        LoraUtils::AddSavedMessage("Avoid here", true);


        Display_Utils::clearDisplay();
        Display_Utils:: printCenteredText("Messages Flashed!");
        Display_Utils::UpdateDisplay().Invoke();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    static void ClearLocations(uint8_t inputID)
    {
        NavigationUtils::ClearSavedLocations();
    }

    static void ClearMessages(uint8_t inputID)
    {
        LoraUtils::ClearSavedMessages();
    }

    static void BoundRadioTask(void *pvParameters)
    {
        LoraManager *manager = (LoraManager *)pvParameters;
        manager->RadioTask();
    }

    static void BoundSendQueueTask(void *pvParameters)
    {
        LoraManager *manager = (LoraManager *)pvParameters;
        manager->SendQueueTask();
    }
};