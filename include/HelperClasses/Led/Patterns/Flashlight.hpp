#pragma once

#include "LedPatternInterface.hpp"


// Turns a segment of LEDs on (white) or off
class Flashlight : public LedPatternInterface
{
public:
    Flashlight(LedSegment segment)
        : LedPatternInterface(std::move(segment)), _on(false)
    {
        setAnimationLengthTicks(1);
    }

    void configurePattern(JsonDocument &config)
    {
        if (config["on"].is<bool>())
        {
            _on = config["on"].as<bool>();
        }

        if (config["toggle"].is<bool>())
        {
            _on = !_on;
        }
    }

    bool iterateFrame()
    {
        CRGB color = _on ? CRGB::White : CRGB::Black;
        for (size_t i = 0; i < _segment.length(); i++)
        {
            _segment[i] = color;
        }
        return true;
    }

    void toggle()
    {
        _on = !_on;
    }

    bool isOn() const { return _on; }

    void SetRegisteredPatternID(int patternID) { _RegisteredPatternId() = patternID; }
    static int RegisteredPatternID() { return _RegisteredPatternId(); }

protected:
    static int &_RegisteredPatternId()
    {
        static int id = -1;
        return id;
    }

    bool _on;
};
