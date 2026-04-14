#pragma once

#if HARDWARE_VERSION == 1
    #include "Bootstrap/V1/BootstrapLeds.hpp"
#elif HARDWARE_VERSION == 2
    #include "Bootstrap/V2/BootstrapLeds.hpp"
#elif HARDWARE_VERSION == 3
    #include "Bootstrap/V3/BootstrapLeds.hpp"
#else
    #error "Unknown HARDWARE_VERSION. Must be 1, 2, or 3."
#endif

#ifdef USE_V3_OLED
#include "Adafruit_SSD1327.h"
#endif

#include <FastLED.h>

#include "LoraManager.h"
#include "FilesystemUtils.h"
#include "RpcUtils.h"
#include "LED_Utils.h"
#include "FilesystemManager.h"
#include "EspNowManager.h"
#include "Display_Manager.h"
#include "DisplayManager.hpp"
#include "WindowFactories.hpp"

#include "RpcManager.h"
#include "SettingsInterface.hpp"
#include <map>
#include "Bluetooth_Utils.h"

#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"

#include "ArduinoJson.h"
#include "globalDefines.h"
#include "esp_log.h"

// Window Includes

#include "HelperClasses/Window/HomeWindow.hpp"
#include "CompassWindow.hpp"
#include "GpsWindow.hpp"
#include "DiagnosticsWindow.hpp"
#include "EditSavedLocationsWindow.hpp"
#include "EditStatusMessagesWindow.hpp"
#include "WiFiRpcWindow.hpp"
#include "PairBluetoothWindow.hpp"

static const char *TAG_COMPASS = "COMPASS";

namespace
{
    static RpcModule::Manager RpcManagerInstance;
    static DisplayModule::Manager DisplayManagerInstance;
    static ConnectivityModule::EspNowManager EspNowManagerInstance;
    FilesystemModule::Manager filesystemManagerInstance;
    static AsyncWebServer WebServerInstance(80);
    static AsyncCorsMiddleware cors;
    static std::shared_ptr<DisplayModule::HomeWindow> _homeWindowInstance;

    #if HARDWARE_VERSION < 3
    Adafruit_SSD1306 display = Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire);
    #elif HARDWARE_VERSION == 3
    #ifdef USE_V3_OLED   
    Adafruit_SSD1327 display = Adafruit_SSD1327(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
    #else
    GFXcanvas1 display = GFXcanvas1(OLED_WIDTH, OLED_HEIGHT);
    #endif
    #endif
}

// Static class to help interface with esp32 utils compass functionality
// Eventually, all windows pertaining to the compass specific functionality will reference this class
class CompassUtils
{
public:

    static uint8_t MessageReceivedInputID;
    static ArduinoLoRaDriver ArduinoLora;

    static void Bootstrap()
    {
        BootstrapLeds::Initialize();
    }

    static void PassMessageReceivedToDisplay(uint32_t sendingUserID, bool isNew)
    {
        if (isNew)
        {
            #if DEBUG == 1
            ESP_LOGI(TAG_COMPASS, "PassMessageReceivedToDisplay: New message received");
            #endif
            
            // Check if we're on home window
            if (_homeWindowInstance == DisplayModule::Utilities::activeWindow())
            {
                ESP_LOGI(TAG, "Refreshing home window");
                _homeWindowInstance->onMessageReceived();
                DisplayModule::Utilities::render();
            }

            if (System_Utils::silentMode == false)
            {
                LED_Manager::buzzerNotification();
            }
        }
        #if DEBUG == 1
        else
        {
            ESP_LOGI(TAG_COMPASS, "PassMessageReceivedToDisplay: Old message received");
        }
        #endif

        // TODO: Maybe add refresh display command if not new
    }


    static void InitializeSettings()
    {   
        filesystemManagerInstance.InitializeFilesystem();

        CheckDeviceInfo();
        auto settings = GenerateSettings();
        FilesystemModule::Utilities::DeviceSettings() = std::move(settings);

        FilesystemModule::Utilities::SettingsUpdated() += ProcessSettingsFile;
        FilesystemModule::Utilities::SettingsUpdated() += ConnectivityModule::Utilities::ProcessSettings;
        FilesystemModule::Utilities::SettingsUpdated() += System_Utils::UpdateSettings;
        FilesystemModule::Utilities::SettingsUpdated() += Bluetooth_Utils::SettingsUpdated;

        FilesystemModule::Utilities::RequestSettingsRefresh() += []()
        {
            FilesystemModule::Utilities::InvokeSettingsUpdated(FilesystemModule::Utilities::DeviceSettings());
        };

        FilesystemModule::Utilities::RequestSettingsRefresh().Invoke();
    }

    static void CheckDeviceInfo()
    {
        auto& deviceInfo = FilesystemModule::Utilities::DeviceInfo();
        if (!deviceInfo.isKey("UserID"))
        {
            uint32_t userID = esp_random();
            deviceInfo.putUInt("UserID", userID);
            ESP_LOGI(TAG_COMPASS, "Generated new UserID: %u", userID);
        }

        // Not sure these are necessary
        if (!deviceInfo.isKey("Firmware") || deviceInfo.getString("Firmware").c_str() != std::string(FIRMWARE_VERSION_STRING))
        {
            deviceInfo.putString("Firmware", FIRMWARE_VERSION_STRING);
            ESP_LOGI(TAG_COMPASS, "Set Firmware Version: %s", FIRMWARE_VERSION_STRING);
        }

        if (deviceInfo.getInt("Hardware", -1) != HARDWARE_VERSION)
        {
            deviceInfo.putInt("Hardware", HARDWARE_VERSION);
            ESP_LOGI(TAG_COMPASS, "Set Hardware Version: %d", HARDWARE_VERSION);
        }

        System_Utils::DeviceID = deviceInfo.getUInt("UserID");
    }

    static std::map<std::string, std::shared_ptr<FilesystemModule::SettingsInterface>> GenerateSettings()
    {
        std::map<std::string, std::shared_ptr<FilesystemModule::SettingsInterface>> settings;

        std::string defaultUserName;
        std::string defaultDeviceName;
        {
            char usernamebuffer[10];
            sprintf(usernamebuffer, "User_%04X", FilesystemModule::Utilities::DeviceInfo().getUInt("UserID") & 0xFFFF);
            defaultUserName = usernamebuffer;
        }
        {
            char devicenamebuffer[20];
            sprintf(devicenamebuffer, "Beacon_%04X", FilesystemModule::Utilities::DeviceInfo().getUInt("UserID") & 0xFFFF);
            defaultDeviceName = devicenamebuffer;
        }
        
        auto userName = std::make_shared<FilesystemModule::StringSetting>("User Name", defaultUserName, 12);
        settings[userName->key] = userName;

        auto deviceName = std::make_shared<FilesystemModule::StringSetting>("Device Name", defaultDeviceName, 20);
        settings[deviceName->key] = deviceName;

        std::vector<std::string> colorThemeOptions = {"Custom", "Red", "Green", "Blue", "Purple", "Yellow", "Cyan", "White", "Orange"};
        std::vector<int> colorThemeValues = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        auto colorTheme = std::make_shared<FilesystemModule::EnumSetting>("Theme Color", 2, colorThemeOptions, colorThemeValues);
        settings[colorTheme->key] = colorTheme;

        auto themeRed = std::make_shared<FilesystemModule::IntSetting>("Theme Color Red", 0, 0, 255, 1);
        settings[themeRed->key] = themeRed;

        auto themeGreen = std::make_shared<FilesystemModule::IntSetting>("Theme Color Green", 255, 0, 255, 1);
        settings[themeGreen->key] = themeGreen;

        auto themeBlue = std::make_shared<FilesystemModule::IntSetting>("Theme Color Blue", 0, 0, 255, 1);
        settings[themeBlue->key] = themeBlue;

        // TODO: dumb this down to walkie-talkie style channels
        auto frequency = std::make_shared<FilesystemModule::FloatSetting>("Frequency", 914.9, 902.3, 914.9, 0.2);
        settings[frequency->key] = frequency;

        auto broadcastAttempts = std::make_shared<FilesystemModule::IntSetting>("Num Broadcasts", 3, 1, 5, 1);
        settings[broadcastAttempts->key] = broadcastAttempts;

        auto silentMode = std::make_shared<FilesystemModule::BoolSetting>("Silent Mode", false);
        settings[silentMode->key] = silentMode;

        auto time24hr = std::make_shared<FilesystemModule::BoolSetting>("24H Time", false);
        settings[time24hr->key] = time24hr;

        std::vector<std::string> wifiOptions = {"Off", "AP Mode", "Station Mode"};
        std::vector<int> wifiValues = {0, 1, 2};
        auto wifiProvisioning = std::make_shared<FilesystemModule::EnumSetting>("WiFi Mode", 0, wifiOptions, wifiValues);
        settings[wifiProvisioning->key] = wifiProvisioning;

        auto wifiapPassword = std::make_shared<FilesystemModule::StringSetting>("WiFi AP Password", "esp-pass", 21);
        settings[wifiapPassword->key] = wifiapPassword;

        for (const auto& setting : settings)
        {
            setting.second->loadFromPreferences(FilesystemModule::Utilities::SettingsPreference());
        }

        return settings;
    }

    static void ProcessSettingsFile(JsonDocument &doc)
    {
        ESP_LOGI(TAG_COMPASS, "Processing settings file update");

        if (!doc.isNull())
        {
            std::string debugStr = doc.as<std::string>();
            ESP_LOGI(TAG_COMPASS, "Settings JSON: %s", debugStr.c_str());

            // LED Module
            int colorTheme = doc["Theme Color"].as<int>();

            uint8_t red = doc["Theme Color Red"].as<uint8_t>();
            uint8_t green = doc["Theme Color Green"].as<uint8_t>();
            uint8_t blue = doc["Theme Color Blue"].as<uint8_t>();

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
            auto interfaceColor = LedPatternInterface::ThemeColor();
            ESP_LOGI(TAG_COMPASS, "LED Interface::ThemeColor: %d, %d, %d", interfaceColor.r, interfaceColor.g, interfaceColor.b);

            // Lora Module
            LoraUtils::SetUserName(doc["User Name"].as<std::string>());
            LoraUtils::SetDefaultSendAttempts(doc["Broadcast Attempts"].as<uint8_t>());

            // ArduinoLora.SetFrequency(frequency);
            ArduinoLora.SetSpreadingFactor(7);
            ArduinoLora.SetSignalBandwidth(125E3);

            #if HARDWARE_VERSION == 1
            ArduinoLora.SetTXPower(20);
            #endif

            #if HARDWARE_VERSION == 2
            ArduinoLora.SetTXPower(23);
            #endif

            #if HARDWARE_VERSION == 3
            ArduinoLora.SetTXPower(23);
            #endif

            ESP_LOGD(TAG_COMPASS, "ProcessSettingsFile: Done");
        }
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
        RpcModule::Utilities::RegisterRpc("GetDisplayContents", GetDisplayContentsRpc);

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

    static void GetDisplayContentsRpc(JsonDocument &doc)
    {
        doc["width"] = display.width();
        doc["height"] = display.height();

        size_t bufferLength = (display.width() * display.height()) / 8;
        uint8_t* displayBuffer = display.getBuffer();

        // 128x128 resolution = 2048 bytes, b64 encoded = 2730 chars
        unsigned char contents[3000];
        size_t b64_len = 0;
        auto res = mbedtls_base64_encode(contents, sizeof(contents), &b64_len, displayBuffer, bufferLength);
        ESP_LOGI(TAG, "Encoded buffer of size %d into %d b64 bytes (res=%d)", bufferLength, b64_len, res);

        if (res == 0) {
            doc["buffer"] = String(contents, b64_len);
        }
    }

    // Display Manager

    static void InitializeDisplayManager()
    {
        ESP_LOGI(TAG, "Initializing display driver...");
        auto displayPtr = InitializeDisplayDriver();

        DisplayManagerInstance.init(displayPtr, OLED_WIDTH, OLED_HEIGHT); 

        DisplayModule::initDefaultLayers();

        // Wire up input draw commands
        auto windowLayer = std::static_pointer_cast<DisplayModule::WindowLayer>(DisplayModule::Utilities::getLayer(DisplayModule::LayerID::WINDOW));

#if HARDWARE_VERSION == 1
        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_1, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_2, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_3, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_4, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_UP, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd("^", fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_DOWN, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd("v", fmt);
            cmd.draw(drawCtx);
        });

#elif HARDWARE_VERSION == 2

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_1, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_2, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_3, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_4, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_UP, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd("^", fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_DOWN, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd("v", fmt);
            cmd.draw(drawCtx);
        });

#elif HARDWARE_VERSION == 3

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_1, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_2, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_3, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_4, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_UP, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd("^", fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_DOWN, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd("v", fmt);
            cmd.draw(drawCtx);
        });

#endif

#if HARDWARE_VERSION < 3 || defined(USE_V3_OLED)
        DisplayModule::Utilities::onRenderComplete += []()
        {
            display.display();
        };
#endif

        InitializeHomeWindow();
    }

    static Adafruit_GFX *InitializeDisplayDriver()
    {
#if HARDWARE_VERSION < 3
        display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
        display.clearDisplay();

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.display();

        return static_cast<Adafruit_GFX *>(&display);
#else
#ifdef USE_V3_OLED
        ESP_LOGI(TAG, "Initializing SSD1327...");
        auto result = display.begin(0x3C);

        if (result)
        {
            ESP_LOGI(TAG, "SSD1327 Initialized.");
            display.setRotation(2);
            display.clearDisplay();
            display.setContrast(0x7F);
            display.setTextColor(SSD1327_WHITE);
        }
        else
        {
            ESP_LOGW(TAG, "SSD1327 Failed to initialize.");
        }
#else
        display.setTextColor(WHITE);
#endif

        display.setTextSize(1);
        display.setCursor(0, 0);
        return static_cast<Adafruit_GFX *>(&display);
#endif
    }

    static void InitializeHomeWindow()
    {
        ESP_LOGI(TAG_COMPASS, "InitializeHomeWindow");
        _homeWindowInstance = DisplayModule::HomeWindow::create(MainMenuFactory);

        DisplayModule::Utilities::pushWindow(_homeWindowInstance);
    }

    static void MainMenuFactory(const DisplayModule::InputContext &ctx)
    {
        std::vector<DisplayModule::MenuItem> menuItems;

        // Register Main Menu Items
        menuItems.push_back(DisplayModule::MenuItem("Settings", SettingsWindowFactory));
        menuItems.push_back(DisplayModule::MenuItem("Configure via WiFi", []()
        {
            auto wifiRpcWindow = std::make_shared<DisplayModule::WiFiRpcWindow>(true);
            DisplayModule::Utilities::pushWindow(wifiRpcWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Configure via BT", []()
        {
            auto btRpcWindow = std::make_shared<DisplayModule::PairBluetoothWindow>();
            DisplayModule::Utilities::pushWindow(btRpcWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Edit Status Messages", []()
        {
            auto editStatusMessagesWindow = std::make_shared<DisplayModule::EditStatusMessagesWindow>();
            DisplayModule::Utilities::pushWindow(editStatusMessagesWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Edit Saved Locations", []()
        {
            auto editSavedLocationsWindow = std::make_shared<DisplayModule::EditSavedLocationsWindow>();
            DisplayModule::Utilities::pushWindow(editSavedLocationsWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Debug Compass", []()
        {
            auto compassWindow = std::make_shared<DisplayModule::CompassWindow>();
            DisplayModule::Utilities::pushWindow(compassWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Debug GPS", []()
        {
            auto gpsWindow = std::make_shared<DisplayModule::GpsWindow>();
            DisplayModule::Utilities::pushWindow(gpsWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Diagnostic Info", []()
        {
            auto diagnosticsWindow = std::make_shared<DisplayModule::DiagnosticsWindow>();
            DisplayModule::Utilities::pushWindow(diagnosticsWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Reboot", []()
        {
            auto drawCtx = DisplayModule::Utilities::drawContext();
            drawCtx.display->fillScreen(SSD1306_BLACK);
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::CENTER;
            DisplayModule::TextDrawCommand cmd("Rebooting...", fmt);
            cmd.draw(drawCtx);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            ESP.restart();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }));

        auto mainMenuWindowPtr = DisplayModule::makeMenuWindow(menuItems);

        DisplayModule::Utilities::pushWindow(mainMenuWindowPtr);
    }

    static void SettingsWindowFactory()
    {
        auto settingsWindowPtr = DisplayModule::makeSettingsWindow();   
        DisplayModule::Utilities::pushWindow(settingsWindowPtr);
    }

    static void InitializeLedManager(uint8_t cpuCore = 0)
    {
        auto leds = new CRGB[NUM_LEDS];

        #if HARDWARE_VERSION == 1
            FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        #elif HARDWARE_VERSION == 2
            FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        #elif HARDWARE_VERSION == 3
            FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        #endif

        LED_Manager::init(NUM_LEDS, leds, cpuCore);
    }

    private:

    const char * _TAG = "CompassUtils";

    static void EnableServerOnWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        if ((int)event == (int)SYSTEM_EVENT_STA_GOT_IP ||
            (int)event == (int)ARDUINO_EVENT_WIFI_AP_START)
        {
            ESP_LOGI(TAG, "WiFi event fired, waiting for stack to stabilize...");
            vTaskDelay(pdMS_TO_TICKS(500));  // Critical: let AP interface fully initialize
            ESP_LOGI(TAG, "WiFi connected, starting RPC server");
            WebServerInstance.begin();
        }
        else if ((int)event == (int)SYSTEM_EVENT_STA_DISCONNECTED)
        {
            // server.end();
        }
    }

    
};
