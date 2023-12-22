#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <map>
#include "EventDeclarations.h"
#include "OLED_Window.h"
#include "OLED_Settings_Window.h"
#include "Compass_Window.h"
#include "GPS_Window.h"
#include "LoRa_Test_Window.h"
#include "Statuses_Window.h"
#include "Ping_Window.h"
#include "Home_Window.h"
#include "SOS_Window.h"
#include "globalDefines.h"
// #include "esp_event_base.h"

#define DEBOUNCE_DELAY 100

using fp = void (*)(void *);

class OLED_Manager
{
public:
    static OLED_Manager *instance;

    static Adafruit_SSD1306 display;

    static OLED_Window *currentWindow;
    static OLED_Window *rootWindow;

    static std::map<uint32_t, fp> callbackMap;

    static void init();

    static OLED_Window *attachNewWindow();
    static void attachNewWindow(OLED_Window *window);

    // Input handler using FreeRTOS task notifications.
    static void processButtonPressEvent(void *taskParams);
    static void initializeCallbacks();
    static void processEventCallback(uint32_t resourceID, void *event_args);
    static void registerCallback(uint32_t resourceID, fp callback);
    static void displayLowBatteryShutdownNotice();

    // Menu functions

    static void goBack(void *arg);
    static void select(void *arg);
    static void generateHomeWindow(void *arg);
    static void generateSettingsWindow(void *arg);
    static void generatePingWindow(void *arg);
    static void generateStatusesWindow(void *arg);
    static void generateMenuWindow(void *arg);
    static void generateCompassWindow(void *arg);
    static void generateGPSWindow(void *arg);
    static void generateLoRaTestWindow(void *arg);
    static void flashDefaultSettings(void *arg);
    static void rebootDevice(void *arg);
    static void toggleFlashlight(void *arg);
    static void shutdownDevice(void *arg);
    static void toggleSilentMode(void *arg);
    static void quickActionMenu(void *arg);
    static void openSOS(void *arg);

private:
    static TickType_t lastButtonPressTick;
};