#pragma once

#include <cstdio>
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "TextDrawCommand.hpp"
#include "NavigationUtils.h"
#include "RingPoint.hpp"
#include "LED_Utils.h"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // CompassDebugState
    // -------------------------------------------------------------------------
    // Live compass readout — shows the current azimuth value and, optionally,
    // raw magnetometer values from NavigationUtils.
    //
    // Behaviour:
    //   - refreshIntervalMs() returns REFRESH_RATE_MS so the display auto-ticks.
    //   - Each tick: calls onTick() to read the current azimuth, updates the
    //     draw command text, and re-renders.
    //
    // LED ring feedback (e.g. a compass ring LED pattern) is intentionally
    // omitted here.  Subclass CompassDebugState and override onEnter/onExit/
    // onTick to add LED logic, or wire it in the owning Window.
    //
    // Wiring example (Window):
    //   win.addInputCommand(InputID::BUTTON_3, [&win](auto &) {
    //       win.popState();
    //   });
    //
    // onTick() should be called by the application each time the refresh timer
    // fires (i.e. each time Utilities::render() is invoked while this state is
    // active).

    class CompassDebugState : public WindowState
    {
    public:
        static constexpr uint32_t REFRESH_RATE_MS = 100;

        CompassDebugState()
        {
            bindInput(InputID::BUTTON_3, "Back");
            refreshIntervalMs = REFRESH_RATE_MS;
            _ringPointPatternId = RingPoint::RegisteredPatternID();
        }

        // ------------------------------------------------------------------
        // Lifecycle
        // ------------------------------------------------------------------

        void onEnter(const StateTransferData &) override
        {
            _rebuildDrawCommands();
            LED_Utils::enablePattern(_ringPointPatternId);
        }

        void onExit() override
        {
            LED_Utils::clearPattern(_ringPointPatternId);
            LED_Utils::disablePattern(_ringPointPatternId);
        }

        void onTick()
        {
            NavigationUtils::PrintRawValues(); // logs to serial/ESP_LOG
            auto heading = NavigationUtils::GetAzimuth();
            _bearing = 360.0f - heading;
            ESP_LOGD(TAG, "Current heading: %d Current bearing: %d", heading, static_cast<int>(_bearing));
            _rebuildDrawCommands();
            _configureLeds();
        }

        // Returns the most recently read azimuth (degrees, 0–360).
        // TODO: Figure out if this is needed
        // float currentAzimuth() const { return _bearing; }

    private:
        float _bearing = 0.0f;
        int _ringPointPatternId = -1;

        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            char buf[24];
            snprintf(buf, sizeof(buf), "North: %.1f", static_cast<double>(_bearing));

            addDrawCommand(std::make_shared<TextDrawCommand>(
                std::string(buf),
                TextFormat{ TextAlignH::CENTER, TextAlignV::CENTER }
            ));
        }

        void _configureLeds()
        {
            StaticJsonDocument<64> doc;

            doc["fadeDegrees"] = 20;
            doc["directionDegrees"] = _bearing;

            LED_Utils::configurePattern(_ringPointPatternId, doc);
            LED_Utils::iteratePattern(_ringPointPatternId);
        }
    };

} // namespace DisplayModule
