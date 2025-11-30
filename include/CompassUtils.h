#pragma once

#include "Display_Utils.h"
#include "LoraManager.h"
#include "FilesystemUtils.h"
#include "RpcUtils.h"
#include "LED_Utils.h"
#include "Settings_Manager.h"
#include "EspNowManager.h"
#include "Display_Manager.h"
#include "RpcManager.h"
#include "NetworkUtils.h"

#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"

#include "ArduinoJson.h"
#include "globalDefines.h"

namespace
{
    const char *SETTINGS_FILENAME PROGMEM = "/Settings.msgpk";
    const char *OLD_SETTINGS_FILENAME PROGMEM = "/settings.json";
    static RpcModule::Manager RpcManagerInstance;
    static ConnectivityModule::EspNowManager EspNowManagerInstance;
    static AsyncWebServer WebServerInstance(80);
    static AsyncCorsMiddleware cors;
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
        // DynamicJsonDocument settings(2048);
        // FilesystemModule::Utilities::SettingsFileName() = SETTINGS_FILENAME;
        auto returncode = FilesystemModule::Utilities::LoadSettingsFile(FilesystemModule::Utilities::SettingsFile());

#if DEBUG == 1
        Serial.print("CompassUtils::InitializeSettings: LoadSettingsFile returned ");
        Serial.println(returncode);
#endif

        // CheckSettingsFile(FilesystemModule::Utilities::SettingsFile());

        FilesystemModule::Utilities::SettingsUpdated() += CheckSettingsFile;
        FilesystemModule::Utilities::SettingsUpdated() += ProcessSettingsFile;
        FilesystemModule::Utilities::SettingsUpdated() += ConnectivityModule::Utilities::ProcessSettings;
        FilesystemModule::Utilities::SettingsUpdated() += System_Utils::UpdateSettings;

        FilesystemModule::Utilities::SettingsUpdated().Invoke(FilesystemModule::Utilities::SettingsFile());

        #if DEBUG == 1
        Serial.println("Settings File: ");
        serializeJsonPretty(FilesystemModule::Utilities::SettingsFile(), Serial);
        Serial.println();
        #endif
    }

    static void CheckSettingsFile(JsonDocument &doc)
    {
        bool updateSettings = false;

        if (!doc.containsKey("UserID"))
        {
            doc["UserID"] = esp_random();
            updateSettings = true;
        }

        if (!doc.containsKey("User Name"))
        {
            char usernamebuffer[10];
            sprintf(usernamebuffer, "User_%04X", doc["UserID"].as<uint32_t>() & 0xFFFF);
            std::string username = usernamebuffer;

            JsonObject User_Name = doc.createNestedObject("User Name");
            User_Name["cfgType"] = 10;
            User_Name["cfgVal"] = usernamebuffer;
            User_Name["dftVal"] = usernamebuffer;
            User_Name["maxLen"] = 12;
            doc["Silent Mode"] = false;

            updateSettings = true;
        }

        if (!doc.containsKey("Device Name"))
        {
            char devicenamebuffer[21];
            sprintf(devicenamebuffer, "Beacon_%04X", doc["UserID"].as<uint32_t>() & 0xFFFF);
            std::string deviceName = devicenamebuffer;

            JsonObject Device_Name = doc.createNestedObject("Device Name");
            Device_Name["cfgType"] = 10;
            Device_Name["cfgVal"] = deviceName;
            Device_Name["dftVal"] = deviceName;
            Device_Name["maxLen"] = 20;
            doc["Silent Mode"] = false;

            updateSettings = true;
        }

        if (!doc.containsKey("Color Theme"))
        {
            JsonObject Color_Theme = doc.createNestedObject("Color Theme");
            Color_Theme["cfgType"] = 11;
            Color_Theme["cfgVal"] = 2;
            Color_Theme["dftVal"] = 2;

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

            updateSettings = true;
        }

        if (!doc.containsKey("Theme Red"))
        {
            JsonObject Theme_Red = doc.createNestedObject("Theme Red");
            Theme_Red["cfgType"] = 8;
            Theme_Red["cfgVal"] = 0;
            Theme_Red["dftVal"] = 0;
            Theme_Red["maxVal"] = 255;
            Theme_Red["minVal"] = 0;
            Theme_Red["incVal"] = 1;
            Theme_Red["signed"] = false;
            updateSettings = true;
        }

        if (!doc.containsKey("Theme Green"))
        {
            JsonObject Theme_Green = doc.createNestedObject("Theme Green");
            Theme_Green["cfgType"] = 8;
            Theme_Green["cfgVal"] = 255;
            Theme_Green["dftVal"] = 255;
            Theme_Green["maxVal"] = 255;
            Theme_Green["minVal"] = 0;
            Theme_Green["incVal"] = 1;
            Theme_Green["signed"] = false;
            updateSettings = true;
        } 

        if (!doc.containsKey("Theme Blue"))
        {
            JsonObject Theme_Blue = doc.createNestedObject("Theme Blue");
            Theme_Blue["cfgType"] = 8;
            Theme_Blue["cfgVal"] = 0;
            Theme_Blue["dftVal"] = 0;
            Theme_Blue["maxVal"] = 255;
            Theme_Blue["minVal"] = 0;
            Theme_Blue["incVal"] = 1;
            Theme_Blue["signed"] = false;
            updateSettings = true;
        }

        if (!doc.containsKey("Frequency"))
        {
            JsonObject Frequency = doc.createNestedObject("Frequency");
            Frequency["cfgType"] = 9;
            Frequency["cfgVal"] = 914.9;
            Frequency["dftVal"] = 914.9;
            Frequency["maxVal"] = 914.9;
            Frequency["minVal"] = 902.3;
            Frequency["incVal"] = 0.2;

            updateSettings = true;
        }
        
        if (!doc.containsKey("Modem Config"))
        {
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

            updateSettings = true;
        }

        if (!doc.containsKey("Broadcast Attempts"))
        {
            JsonObject Broadcast_Attempts = doc.createNestedObject("Broadcast Attempts");
            Broadcast_Attempts["cfgType"] = 8;
            Broadcast_Attempts["cfgVal"] = 3;
            Broadcast_Attempts["dftVal"] = 3;
            Broadcast_Attempts["maxVal"] = 5;
            Broadcast_Attempts["minVal"] = 1;
            Broadcast_Attempts["incVal"] = 1;
            Broadcast_Attempts["signed"] = false;

            updateSettings = true;
        }

        if (!doc.containsKey("Silent Mode"))
        {
            doc["Silent Mode"] = false;
            updateSettings = true;
        }

        if (!doc.containsKey("24H Time"))
        {
            doc["24H Time"] = false;    
            updateSettings = true;
        }

        if (!doc.containsKey("WiFi Provisioning"))
        {
            JsonObject provisioningMode = doc.createNestedObject("WiFi Provisioning");
            provisioningMode["cfgType"] = 11;
            provisioningMode["cfgVal"] = 1;
            provisioningMode["dftVal"] = 1 ;

            JsonArray provisioningMode_vals = provisioningMode.createNestedArray("vals");
            provisioningMode_vals.add(0);
            provisioningMode_vals.add(1);
            // provisioningMode_vals.add(2);

            JsonArray provisioningMode_valTxt = provisioningMode.createNestedArray("valTxt");
            provisioningMode_valTxt.add("None");
            provisioningMode_valTxt.add("ESP-NOW Dongle");
            // provisioningMode_valTxt.add("Access Point");

            updateSettings = true;
        }

        if (!doc.containsKey("Firmware Version") || doc["Firmware Version"].as<std::string>() != std::string(FIRMWARE_VERSION_STRING))
        {
            doc["Firmware Version"] = FIRMWARE_VERSION_STRING;
            updateSettings = true;
        }

        if (!doc.containsKey("Hardware Version") || doc["Hardware Version"].as<int>() != HARDWARE_VERSION)
        {
            doc["Hardware Version"] = HARDWARE_VERSION;
            updateSettings = true;
        }

        if (doc.overflowed())
        {
            #if DEBUG == 1
            Serial.println("CompassUtils::CheckSettingsFile: Settings file overflowed.");
            #endif
        }

        if (updateSettings)
        {
            #if DEBUG == 1
            Serial.println("CompassUtils::FlashSettings: Updating settings file.");
            #endif
            FilesystemModule::Utilities::WriteFile(FilesystemModule::Utilities::SettingsFileName(), doc);
        }

        
    }

    static void ProcessSettingsFile(JsonDocument &doc)
    {
        // JsonDocument &doc = FilesystemModule::Utilities::SettingsFile();
        #if DEBUG == 1
        // Serial.println("CompassUtils::ProcessSettingsFile");
        // serializeJson(doc, Serial);
        // Serial.println();
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
        // Display_Manager::registerCallback(ACTION_FLASH_DEFAULT_SETTINGS, FlashSettings);
        // Display_Manager::registerCallback(ACTION_FLASH_LOCATIONS, FlashCampLocations);
        // Display_Manager::registerCallback(ACTION_FLASH_MESSAGES, FlashMessages);
        // Display_Manager::registerCallback(ACTION_CLEAR_LOCATIONS, ClearLocations);
        // Display_Manager::registerCallback(ACTION_CLEAR_MESSAGES, ClearMessages);

        Display_Utils::UpdateDisplay() += UpdateDisplay;
    }

    static void InitializeRpc(size_t rpcTaskPriority, size_t rpcTaskCore)
    {
        // Allow CORS requests for RPC server.
        WebServerInstance.addMiddleware(&cors);

        RpcManagerInstance.Init(rpcTaskPriority, rpcTaskCore);
        RpcManagerInstance.RegisterWebServerRpc(WebServerInstance); 

        WiFi.onEvent(EnableServerOnWiFiConnected);

        RegisterRpcFunctions();
    }

    static void WireFunctions()
    {
        ConnectivityModule::Utilities::InitializeEspNow() += [](esp_now_recv_cb_t receiveFunction, esp_now_send_cb_t sendFunction) 
        { 
            if (ConnectivityModule::Utilities::RpcChannelID() == -1)
            {
                ConnectivityModule::Utilities::RpcChannelID() = RpcModule::Utilities::AddRpcChannel(
                    512, 
                    std::bind(
                        &ConnectivityModule::EspNowManager::
                        ReceiveRpcQueue, 
                        &EspNowManagerInstance, 
                        std::placeholders::_1, 
                        std::placeholders::_2), 
                        RpcModule::Utilities::RpcResponseNullDestination);
            }

            RpcModule::Utilities::EnableRpcChannel(ConnectivityModule::Utilities::RpcChannelID());
            
            EspNowManagerInstance.Initialize(receiveFunction, sendFunction); 
        };
        ConnectivityModule::Utilities::DeinitializeEspNow() += [](bool disableRadio) 
        { 
            RpcModule::Utilities::DisableRpcChannel(ConnectivityModule::Utilities::RpcChannelID());
            EspNowManagerInstance.Deinitialize(disableRadio); 
        };
    }

    static void RegisterRpcFunctions()
    {
        // Saved Locations
        RpcModule::Utilities::RegisterRpc("AddSavedLocation", NavigationUtils::RpcAddSavedLocation);
        RpcModule::Utilities::RegisterRpc("AddSavedLocations", NavigationUtils::RpcAddSavedLocations);
        RpcModule::Utilities::RegisterRpc("DeleteSavedLocation", NavigationUtils::RpcRemoveSavedLocation);
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

        // Settings
        RpcModule::Utilities::RegisterRpc("GetSettings", FilesystemModule::Utilities::RpcGetSettingsFile);
        RpcModule::Utilities::RegisterRpc("UpdateSetting", FilesystemModule::Utilities::RpcUpdateSetting);
        RpcModule::Utilities::RegisterRpc("UpdateSettings", FilesystemModule::Utilities::RpcUpdateSettings);

        // OTA
        RpcModule::Utilities::RegisterRpc("BeginOTA", System_Utils::StartOtaRpc);
        RpcModule::Utilities::RegisterRpc("UploadOTAChunk", System_Utils::UploadOtaChunkRpc);
        RpcModule::Utilities::RegisterRpc("EndOTA", System_Utils::EndOtaRpc);

        // System
        RpcModule::Utilities::RegisterRpc("RestartSystem", [](JsonDocument &_) { ESP.restart();  vTaskDelay(1000 / portTICK_PERIOD_MS); });
        RpcModule::Utilities::RegisterRpc("GetSystemInfo", System_Utils::GetSystemInfoRpc);

        // Receive WiFi Credentials
        RpcModule::Utilities::RegisterRpc("BroadcastWifiCredentials", [](JsonDocument &doc) 
        { 
            if (doc.containsKey("SSID") && doc.containsKey("Password"))
            {
                auto result = ConnectivityModule::RadioUtils::ConnectToAccessPoint(doc["SSID"].as<std::string>(), doc["Password"].as<std::string>());

                if (result)
                {
                    ConnectivityModule::Utilities::DeinitializeEspNow().Invoke(false);
                }
            }
        });
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

        auto returncode = FilesystemModule::Utilities::WriteSettingsFile(SETTINGS_FILENAME, doc);
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

    static void FlashMessages(uint8_t inputID)
    {
        Display_Utils::clearDisplay();
        Display_Utils::printCenteredText("Flashing Messages...");
        Display_Utils::UpdateDisplay().Invoke();


        LoraUtils::AddSavedMessage("Ping", false);


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

    private:
    static void EnableServerOnWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        if((int)event == (int)SYSTEM_EVENT_STA_GOT_IP) 
        {
            // Start server now that Wi-Fi is ready
            WebServerInstance.begin();
        } 
        else if((int)event == (int)SYSTEM_EVENT_STA_DISCONNECTED) 
        {
            // Optionally, you could stop the server if you like:
            // server.end();
        }
    }
};
