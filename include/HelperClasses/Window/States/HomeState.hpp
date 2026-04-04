#pragma once

#include <cstdio>
#include <string>
#include "WindowState.hpp"
#include "TextDrawCommand.hpp"
#include "FnDrawCommand.hpp"
#include "BatteryIconDrawCommand.hpp"
#include "BellIconDrawCommand.hpp"
#include "MessageIconDrawCommand.hpp"
#include "NavigationUtils.h"
#include "LoraUtils.h"
#include "System_Utils.h"
#include "FilesystemUtils.h"
#include <ArduinoJson.h>

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // HomeState
    // -------------------------------------------------------------------------
    // The primary home screen.  Refreshes every 60 s (GPS / time update).
    //
    // Input layout (wired by owning Window):
    //   BUTTON_1 — "Actions"   → SelectKeyValueState quick-action menu
    //   BUTTON_2 — "Broadcast" → SelectMessageState
    //   BUTTON_3 — "Lock"      → LockState
    //   BUTTON_4 — "Main Menu" → push MenuWindow (via onMainMenuRequested())

    class HomeState : public WindowState
    {
    public:
        static constexpr uint32_t REFRESH_INTERVAL_MS = 60000;

        HomeState() = default;

        // ------------------------------------------------------------------
        // Lifecycle
        // ------------------------------------------------------------------

        void onEnter(const StateTransferData &) override
        {
            ESP_LOGI(TAG, "Entering HomeState");
            _rebuildDrawCommands();
            refreshIntervalMs = REFRESH_INTERVAL_MS;
        }

        void onResume() override
        {
            ESP_LOGI(TAG, "Resuming HomeState");
            _rebuildDrawCommands();
        }

        // ------------------------------------------------------------------
        // Tick — rebuild on each 60-second refresh
        // ------------------------------------------------------------------

        void onTick() override { _rebuildDrawCommands(); }

        // ------------------------------------------------------------------
        // Payload builder for BUTTON_2 (Broadcast)
        // ------------------------------------------------------------------

        void onMessageReceived()
        {
            _rebuildDrawCommands();
        }

        static std::shared_ptr<ArduinoJson::DynamicJsonDocument>
        buildBroadcastPayload()
        {
            auto doc = std::make_shared<ArduinoJson::DynamicJsonDocument>(64);
            (*doc)["sendDirect"] = false;
            return doc;
        }

    private:
        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            NavigationUtils::UpdateGPS();

            // ── Time / GPS (right-aligned, line 2) ──────────────────────────
            {
                char   buf[12];
                TinyGPSTime t = NavigationUtils::GetTime();

                if (t.isValid())
                {
                    int hour = t.hour();
                    hour = (hour < 4) ? hour + 20 : hour - 4; // UTC-4 adjust

                    if (FilesystemModule::Utilities::FetchBoolSetting("24H Time", false))
                    {
                        snprintf(buf, sizeof(buf), "%02d:%02d",
                                 hour, t.minute());
                    }
                    else
                    {
                        int h12 = hour % 12;
                        if (h12 == 0) h12 = 12;
                        snprintf(buf, sizeof(buf), "%02d:%02d %s",
                                 h12, t.minute(), hour < 12 ? "AM" : "PM");
                    }
                }
                else
                {
                    snprintf(buf, sizeof(buf), "No GPS");
                }

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    std::string(buf),
                    TextFormat{ TextAlignH::RIGHT, TextAlignV::LINE, 2 }
                ));
            }

            // ── Battery icon (leftmost, line 2) ─────────────────────────────
            addDrawCommand(std::make_shared<BatteryIconDrawCommand>(
                static_cast<int16_t>(DisplayModule::Utilities::alignTextLeft(0)),
                static_cast<int16_t>(DisplayModule::Utilities::selectTextLine(2)),
                System_Utils::getBatteryPercentage()
            ));

            // ── Bell / sound indicator (line 2, 3 chars from left) ──────────
            addDrawCommand(std::make_shared<BellIconDrawCommand>(
                static_cast<int16_t>(DisplayModule::Utilities::alignTextLeft(3)),
                static_cast<int16_t>(DisplayModule::Utilities::selectTextLine(2)),
                System_Utils::silentMode
            ));

            // ── Last-broadcast scroll hint (ENC_UP → WindowLayer draws "^") ──
            if (LoraUtils::MyLastBroacastExists())
                bindInput(InputID::ENC_UP, "^");
            else
                bindInput(InputID::ENC_UP, "");

            // ── Unread messages indicator ────────────────────────────────────
            size_t unread = LoraUtils::GetNumUnreadMessages();
            if (unread > 0)
            {
                bindInput(InputID::ENC_DOWN, "v");

                uint8_t bottomLine = DisplayModule::Utilities::selectBottomTextLine();

                // Message icon and count on the line above the bottom edge.
                addDrawCommand(std::make_shared<MessageIconDrawCommand>(
                    static_cast<int16_t>(DisplayModule::Utilities::centerTextHorizontal(2, -1)),
                    static_cast<int16_t>(DisplayModule::Utilities::selectTextLine(bottomLine - 1))
                ));

                std::string countStr = ":" + std::to_string(unread);
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    countStr, 
                    TextFormat(TextAlignH::CENTER, TextAlignV::LINE, (uint8_t)(bottomLine - 1), 1)

                ));
            }
            else
            {
                bindInput(InputID::ENC_DOWN, "");
            }
        }
    };

} // namespace DisplayModule
