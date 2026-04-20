#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "EventDeclarations.h"
#include "CompassUtils.h"
#include "globalDefines.h"

#define BUZZER_PIN 7

#define SDA_PIN 18
#define SCL_PIN 17

class BootstrapMicrocontroller
{
public:

    static void Initialize()
    {
        ESP_LOGI("BootstrapMicro", "Initializing IO...");
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

        ESP_LOGI("BootstrapMicro", "Configuring encoder...");
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

        ESP_LOGI("BootstrapMicro", "Scanning I2C addresses...");
        ScannedDevices() = CompassUtils::ScanI2cAddresses(I2cBus());
        ESP_LOGI("BootstrapMicro", "Found %d I2C devices", ScannedDevices().size());
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

    static std::unordered_set<uint8_t> &ScannedDevices()
    {
        static std::unordered_set<uint8_t> scannedDevices;
        return scannedDevices;
    }

private:
    
};