#pragma once

#include "Window.hpp"
#include "States/CompassDebugState.hpp"
#include "States/CompassCalibrateState.hpp"
#include "States/TextDisplayState.hpp"
#include "DisplayUtilities.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // CompassWindow
    // -------------------------------------------------------------------------
    // Three-state compass flow:
    //
    //   CompassDebugState (live azimuth, 100 ms refresh)
    //     ↓ BUTTON_4 ("Calibrate?")
    //   TextDisplayState (prompt — "Start calibration?")
    //     ↓ BUTTON_4 ("Yes")     → CompassCalibrateState (30 ms timed calibration)
    //     ↓ BUTTON_3 ("No/Back") → back to CompassDebugState
    //
    // BUTTON_3 in CompassDebugState pops the window entirely.
    //
    // Usage:
    //   Utilities::pushWindow(std::make_shared<CompassWindow>());

    class CompassWindow : public Window
    {
    public:
        CompassWindow()
        {
            // Allocate states
            _debugState     = std::make_shared<CompassDebugState>();
            _calibrateState = std::make_shared<CompassCalibrateState>();

            // Calibration-prompt text state
            _promptState = std::make_shared<TextDisplayState>();
            {
                auto promptPayload = std::make_shared<ArduinoJson::DynamicJsonDocument>(256);
                auto arr = (*promptPayload).createNestedArray("txtLines");
                auto obj = arr.createNestedObject();
                obj["text"]   = "Start compass calibration?";
                obj["hAlign"] = static_cast<int>(TextAlignH::CENTER);
                obj["vAlign"] = static_cast<int>(TextAlignV::CENTER);
                // TextDisplayState reads this payload in onEnter
                // We store it to be pushed with payload when entering prompt
                _promptPayload = promptPayload;
            }

            // Wire adjacent state references
            _debugState->setAdjacentState(InputID::BUTTON_4, _promptState);
            _promptState->setAdjacentState(InputID::BUTTON_4, _calibrateState);

            // BUTTON_3 in debug state — pop window
            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3,
                [this](const InputContext &ctx)
                {
                    if (_currentState == _debugState)
                        Utilities::popWindow();
                    else if (_currentState == _promptState)
                        switchState(_debugState);
                    else if (_currentState == _calibrateState)
                        switchState(_debugState);
                });

            // BUTTON_4 in debug state — enter calibration prompt
            registerInput(InputID::BUTTON_4, "Calibrate");
            addInputCommand(InputID::BUTTON_4,
                [this](const InputContext &ctx)
                {
                    if (_currentState == _debugState)
                    {
                        StateTransferData d;
                        d.payload = _promptPayload;
                        pushState(_promptState, d);
                    }
                    else if (_currentState == _promptState)
                    {
                        // "Yes" — enter calibration
                        switchState(_calibrateState);
                    }
                });

            setInitialState(_debugState);
        }

        void onTick() override
        {
            Window::onTick(); // dispatches to the current state

            if (_currentState == _calibrateState && _calibrateState->isDone())
            {
                switchState(_debugState);
            }
        }

    private:
        std::shared_ptr<CompassDebugState>     _debugState;
        std::shared_ptr<TextDisplayState>      _promptState;
        std::shared_ptr<CompassCalibrateState> _calibrateState;
        std::shared_ptr<ArduinoJson::DynamicJsonDocument> _promptPayload;
    };

} // namespace DisplayModule
