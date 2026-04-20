#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "EventDeclarations.h"
#include "CompassUtils.h"
#include "globalDefines.h"

#define BUZZER_PIN 4
#define BATT_SENSE_PIN 39

class BootstrapMicrocontroller
{
public:

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

        Encoder().attachFullQuad(ENC_A, ENC_B);
        Encoder().setFilter(1023);
        Encoder().setCount(0);
        
        inputEncoder = &Encoder();

        ScannedDevices() = CompassUtils::ScanI2cAddresses(I2cBus());
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