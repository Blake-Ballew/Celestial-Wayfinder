#pragma once

#include <cstdint>
#include <cstdio>
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "TextDrawCommand.hpp"
#include "NavigationUtils.h"
#include "FilesystemUtils.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // CompassCalibrateState
    // -------------------------------------------------------------------------
    // Runs a timed compass calibration routine.
    //
    // Behaviour:
    //   - onEnter: starts NavigationUtils calibration, resets the countdown.
    //   - refreshIntervalMs: returns REFRESH_RATE_MS so the display ticks.
    //   - Each tick (via ContentLayer redraw): iterates calibration,
    //     decrements the timer, redraws the countdown.  When the timer
    //     expires the state calls win.popState() to return automatically.
    //   - onExit: ends calibration, saves data to SPIFFS via FilesystemModule.
    //
    // The owning Window must wire at least BUTTON_3 (Back) if early exit is
    // desired.  The state exits automatically when the timer expires.
    //
    // Draw update strategy:
    //   Because refreshIntervalMs() returns a non-zero value, Utilities will
    //   call Utilities::render() on a timer rather than only on input.
    //   Each call to draw() on ContentLayer re-executes the stored draw
    //   commands — but we need to iterate the calibration *and* update the
    //   countdown text each frame.  We therefore use a single dynamic
    //   TextDrawCommand whose text is updated each tick via onTick().
    //
    //   The owning Window (or Manager) should call onTick() each time the
    //   display refresh fires.  Alternatively, override this class and update
    //   within a custom draw command.
    //
    // Note: NavigationUtils and FilesystemModule are both part of this
    // library.  LED feedback is intentionally omitted here — subclasses or
    // the owning application can add patterns via onEnter/onExit hooks.

    class CompassCalibrateState : public WindowState
    {
    public:
        static constexpr uint32_t REFRESH_RATE_MS   = 30;
        static constexpr uint32_t TOTAL_TIME_MS     = 10000;

        CompassCalibrateState()
        {
            bindInput(InputID::BUTTON_3, "Cancel");
            refreshIntervalMs = REFRESH_RATE_MS;
        }

        // ------------------------------------------------------------------
        // Lifecycle
        // ------------------------------------------------------------------

        void onEnter(const StateTransferData &) override
        {
            NavigationUtils::BeginCalibration();

            _timerMs    = static_cast<int32_t>(TOTAL_TIME_MS);
            _lastTickMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
            _done       = false;

            _rebuildDrawCommands();
        }

        void onExit() override
        {
            NavigationUtils::EndCalibration();

            // Persist calibration
            ArduinoJson::StaticJsonDocument<128> calibDoc;
            NavigationUtils::GetCalibrationData(calibDoc);
            FilesystemModule::Utilities::WriteFile(
                NavigationUtils::GetCalibrationFilename(), calibDoc);

            WindowState::onExit(); // clears draw commands
        }

        bool isDone() const { return _done; }

        // ------------------------------------------------------------------
        // Per-tick update — called automatically by Window::onTick()
        // ------------------------------------------------------------------

        void onTick() override
        {
            NavigationUtils::IterateCalibration();

            uint32_t nowMs   = xTaskGetTickCount() * portTICK_PERIOD_MS;
            uint32_t deltaMs = nowMs - _lastTickMs;
            _lastTickMs      = nowMs;

            _timerMs -= static_cast<int32_t>(deltaMs);

            if (_timerMs <= 0)
            {
                _done = true;
                return;
            }

            _rebuildDrawCommands();
        }

    private:
        int32_t  _timerMs    = 0;
        uint32_t _lastTickMs = 0;
        bool     _done       = false;

        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            // "Calibrating..." label — line 2
            addDrawCommand(std::make_shared<TextDrawCommand>(
                "Calibrating...",
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 2 }
            ));

            // Countdown in whole seconds — line 3
            char buf[8];
            int  secsRemaining = (_timerMs / 1000) + 1;
            snprintf(buf, sizeof(buf), "%d", secsRemaining);

            addDrawCommand(std::make_shared<TextDrawCommand>(
                std::string(buf),
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 }
            ));
        }
    };

} // namespace DisplayModule
