#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "EventDeclarations.h"
#include "CompassUtils.h"
#include "globalDefines.h"

#define SDA_PIN 21
#define SCL_PIN 22

#define BUZZER_PIN 4

class BootstrapMicrocontroller
{
public:

    constexpr static uint8_t CPU_CORE_LORA = 1;
    constexpr static uint8_t CPU_CORE_APP = 0;

    constexpr static uint8_t BATT_SENSE_PIN = 39;
    constexpr static uint8_t KEEP_ALIVE_PIN = 5;

    static void Initialize()
    {
        pinMode(ENC_A, INPUT_PULLUP);
        pinMode(ENC_B, INPUT_PULLUP);
        // TODO: Rename this one
        pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
        pinMode(BUTTON_1_PIN, INPUT_PULLUP);
        pinMode(BUTTON_2_PIN, INPUT_PULLUP);
        pinMode(BUTTON_3_PIN, INPUT_PULLUP);
        pinMode(BUTTON_4_PIN, INPUT_PULLUP);
        // TODO: Add encoder button

        pinMode(BUZZER_PIN, OUTPUT);
        
        pinMode(BATT_SENSE_PIN, INPUT);
        pinMode(KEEP_ALIVE_PIN, OUTPUT);  
        digitalWrite(KEEP_ALIVE_PIN, HIGH);

        Encoder().attachFullQuad(ENC_A, ENC_B);
        Encoder().setFilter(1023);
        Encoder().setCount(0);
        
        inputEncoder = &Encoder();

        ESP_LOGI("BootstrapMicro", "Initializing I2C bus...");
        auto wireSuccess = I2cBus().begin(SDA_PIN, SCL_PIN);
        if (wireSuccess)
        {
            ESP_LOGI(TAG, "Successfully initialized I2C");
        }
        else
        {
            ESP_LOGW(TAG, "Failed to init I2C");
        }

        ScannedDevices() = CompassUtils::ScanI2cAddresses(I2cBus());

        auto healthTimerID = System_Utils::registerTimer("System Health Monitor", 60000, _MonitorSystemHealth, _HealthTimerBuffer());
        System_Utils::startTimer(healthTimerID);
        _MonitorSystemHealth(nullptr);
    }

    static TwoWire &I2cBus()
    {
        return Wire;
    }

    static ESP32Encoder &Encoder()
    {
        static ESP32Encoder encoder(true, enc_cb);
        return encoder;
    }

    static SPIClass& SpiBus()
    {
        static SPIClass spi(HSPI);
        return spi;
    }

    static std::unordered_set<uint8_t>& ScannedDevices()
    {
        static std::unordered_set<uint8_t> scannedDevices;
        return scannedDevices;
    }

private:
    // TODO: BATERY CURVES
    static void _MonitorSystemHealth(TimerHandle_t _)
    {
        uint16_t voltage = analogRead(BATT_SENSE_PIN);

        if (voltage < 1750)
        {
            // Battery is low. Shut down.

            // Show message and flash leds before turning off
            auto KEEP_ALIVE_PIN = 5;

            digitalWrite(KEEP_ALIVE_PIN, LOW);
        }
    }

    static StaticTimer_t &_HealthTimerBuffer()
    {
        static StaticTimer_t healthTimerBuffer;
        return healthTimerBuffer;
    }
};