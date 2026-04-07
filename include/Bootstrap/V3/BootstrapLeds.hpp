#pragma once

#include "globalDefines.h"

#include "FastLED.h"

#include "LedPatternInterface.hpp"
#include "LedSegment.hpp"
#include "LED_Manager.h"
#include "DisplayUtilities.hpp"

// Patterns

#include "ButtonFlash.hpp"
#include "IlluminateButton.hpp"
#include "RingPoint.hpp"
#include "RingPulse.hpp"
#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "../../HelperClasses/Led/Patterns/Flashlight.hpp"



#define NUM_LEDS 61
#define LED_PIN 16
#define LED_ORDER GRB
#define LED_TYPE WS2812B
#define LED_TASK_CPU_CORE 0

#define LED_IDX_ENCODER_RING 41
#define NUM_ENCODER_LEDS 8

#define LED_IDX_COMPASS_RING 17
#define NUM_COMPASS_LEDS 32

#define LED_IDX_LEFT_TRACE 5
#define LED_IDX_RIGHT_TRACE 57
#define NUM_TRACE_LEDS 4

#define LED_IDX_POWER_BUTTON 0
#define LED_IDX_BUTTON_1 4
#define LED_IDX_BUTTON_2 3
#define LED_IDX_BUTTON_3 1
#define LED_IDX_BUTTON_4 2

/*
LED Mappings:
0:      Power Button
1:      Button 3
2:      Button 4
3:      Button 2
4:      Button 1
5-12:   Screen Light
13-16:  Left Trace
17-48:  Compass (Counter-clockwise)
49-56:  Knob ring (Counter-clockwise)
57-60:  Right Trace

LED Mappings Post Screen Light Removal:
TODO: Remap LEDs after bodging
0:      Power Button
1:      Button 3
2:      Button 4
3:      Button 2
4:      Button 1
5-8:  Left Trace
9-40:  Compass (Counter-clockwise)
41-48:  Knob ring (Counter-clockwise)
49-52:  Right Trace

*/

class BootstrapLeds
{
public:
    static void Initialize()
    {
        FastLED.addLeds<LED_TYPE, LED_PIN, LED_ORDER>(LEDBuffer(), NUM_LEDS);

        LED_Utils::registerPattern(&ButtonFlashPattern());
        LED_Utils::registerPattern(&IlluminateButtonPattern());
        LED_Utils::registerPattern(&RingPointPattern());
        LED_Utils::registerPattern(&RingPulsePattern());
        LED_Utils::registerPattern(&ScrollWheelPattern());

        LED_Manager::init(NUM_LEDS, LEDBuffer(), LED_TASK_CPU_CORE);

        // Initialize button flashing animation

        auto buttonFlashPatternID = ButtonFlash::RegisteredPatternID();
        LED_Utils::enablePattern(buttonFlashPatternID);
        LED_Utils::setAnimationLengthMS(buttonFlashPatternID, 300);

        DisplayModule::Utilities::getInputRaised() += [](const DisplayModule::InputContext &ctx) {
            ESP_LOGI(TAG, "Button flash input: %d", ctx.inputID);
            StaticJsonDocument<64> cfg;
            cfg["inputID"] = ctx.inputID;
            auto buttonFlashPatternID = ButtonFlash::RegisteredPatternID();
            LED_Utils::configurePattern(buttonFlashPatternID, cfg);
            LED_Utils::loopPattern(buttonFlashPatternID, 1);
        };
    }

    static CRGB *LEDBuffer() 
    {
        static CRGB leds[NUM_LEDS];
        return leds;
    }

#pragma region LED_Segments

    static std::vector<size_t> &CompassRingIndicies()
    {
        static std::vector<size_t> compassRingIndicies = 
        {
            17, 48, 47, 46, 45, 44, 43, 42, 
            41, 40, 39, 38, 37, 36, 35, 34, 
            33, 32, 31, 30, 29, 28, 27, 26, 
            25, 24, 23, 22, 21, 20, 19, 18
        };

        return compassRingIndicies;
    }

    static LedSegment &CompassRingSegment()
    {
        static LedSegment compassRing(LEDBuffer(), CompassRingIndicies());
        return compassRing;
    }

    static std::vector<size_t> EncoderRingIndicies()
    {
        static std::vector<size_t> encoderRingIndicies = 
        {
            49, 56, 55, 54,
            53, 52, 51, 50
        };

        return encoderRingIndicies;
    }

    static LedSegment &EncoderRingSegment()
    {
        static LedSegment encoderRing(LEDBuffer(), EncoderRingIndicies());
        return encoderRing;
    }

    static LedSegment &InputLedSegment()
    {
        // Initialize input LEDs in order of InputID
        static LedSegment inputLeds(
            LEDBuffer(),
            {
                LED_IDX_BUTTON_1, 
                LED_IDX_BUTTON_2, 
                LED_IDX_BUTTON_3, 
                LED_IDX_BUTTON_4, 
                LED_IDX_POWER_BUTTON
            });
        return inputLeds;
    }

    static LedSegment LeftTraceSegment()
    {
        static LedSegment leftTrace(LEDBuffer(), LED_IDX_LEFT_TRACE, NUM_TRACE_LEDS);    
        return leftTrace;
    }

    static LedSegment RightTraceSegment()
    {
        static LedSegment rightTrace(LEDBuffer(), LED_IDX_RIGHT_TRACE, NUM_TRACE_LEDS);    
        return rightTrace;
    }

#pragma endregion

#pragma region LED_Patterns

    static ButtonFlash &ButtonFlashPattern()
    {
        static ButtonFlash buttonFlash(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
                BUTTON_SOS
            });
        return buttonFlash;
    }

    static IlluminateButton &IlluminateButtonPattern()
    {
        static IlluminateButton illuminateButton(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
                BUTTON_SOS
            });
        return illuminateButton;
    }

    static RingPoint &RingPointPattern()
    {
        static RingPoint ringPoint(CompassRingSegment());
        return ringPoint;
    }

    static RingPoint &EncoderPointPattern()
    {
        static RingPoint encoderPoint(CompassRingSegment());
        return encoderPoint;
    }

    static RingPulse &RingPulsePattern()
    {
        static RingPulse ringPulse(CompassRingSegment());
        return ringPulse;
    }

    static RingPulse &EncoderPulsePattern()
    {
        static RingPulse encoderPulse(EncoderRingSegment());
        return encoderPulse;
    }

    static ScrollWheel &ScrollWheelPattern()
    {
        static ScrollWheel scrollWheel(EncoderRingSegment());
        return scrollWheel;
    }

#pragma endregion
};