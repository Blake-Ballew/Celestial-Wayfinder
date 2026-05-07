#pragma once

#include <string>
#include <vector>
#include <memory>
#include "MessagePing.h"
#include "WindowState.hpp"
#include "TextDrawCommand.hpp"
#include "LoraUtils.h"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // DisplaySentMessageState
    // -------------------------------------------------------------------------
    // Displays the last message this device broadcast via LoRa.
    // Fetches the message from LoraUtils::MyLastBroacast() on entry.
    //
    // Input layout:
    //   BUTTON_4 — "Retransmit" (only shown when a message exists)
    //
    // Payload out (on BUTTON_4 — retransmit):
    //   Serialized copy of the last broadcast message (via MessageBase::serialize)
    //
    // The Window wires BUTTON_4 to retransmit by reading exitPayload() on
    // input.  BUTTON_3 (back) is wired by the owning Window.
    //
    // The message pointer is owned by this state; the state frees it on exit.

    class DisplaySentMessageState : public WindowState
    {
    public:
        DisplaySentMessageState()
        {
        }

        ~DisplaySentMessageState()
        {
            _deleteMessage();
        }

        // ------------------------------------------------------------------
        // Lifecycle
        // ------------------------------------------------------------------

        void onEnter(const StateTransferData &) override
        {
            _deleteMessage();
            _message    = LoraUtils::MyLastBroacast();
            _hasMessage = (_message != nullptr);

            _rebuildDrawCommands();
        }

        void onExit() override
        {
            _deleteMessage();
            WindowState::onExit();
        }

        // ------------------------------------------------------------------
        // Result payload for retransmit (call before the state exits)
        // ------------------------------------------------------------------

        std::shared_ptr<ArduinoJson::DynamicJsonDocument> buildRetransmitPayload() const
        {
            if (!_message) return nullptr;
            auto doc = std::make_shared<ArduinoJson::DynamicJsonDocument>(512);
            _message->serialize(*doc);
            return doc;
        }

        bool hasMessage() const { return _hasMessage; }

    private:
        MessageBase *_message = nullptr;
        bool        _hasMessage = false;

        const uint8_t _LARGE_DISPLAY_MIN_LINES = 16;
        const uint8_t _MED_DISPLAY_MIN_LINES = 8;
        const uint8_t _MIN_DISPLAY_MIN_LINES = 4;

        void _deleteMessage()
        {
            delete _message;
            _message  = nullptr;
            _hasMessage = false;
        }

        void _rebuildDrawCommands()
        {
            clearDrawCommands();
            auto pingMsg = static_cast<MessagePing *>(_message);

            if (!_message)
            {
                ESP_LOGW(TAG, "DisplaySentMessageState: No message to display");
                return;
            }

            std::string ageInfo = std::string("Sent ") + _displayMessageAge(pingMsg) + " ago";

            auto distance = NavigationUtils::GetDistanceTo(pingMsg->lat, pingMsg->lng);
            std::string distanceInfo;

            if (distance == -1)
            {
                distanceInfo = "Distance unknown";
            }
            else if (distance > 1000)
            {
                distanceInfo = std::to_string(distance / 1000) + "km away";
            }
            else
            {
                distanceInfo = std::to_string(distance) + "m away";
            }
            

            auto displayLines = Utilities::selectBottomTextLine();

            if (displayLines >= _LARGE_DISPLAY_MIN_LINES)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Current Status",
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 1 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _getLineDivider(),
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 2 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    pingMsg->status,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    ageInfo,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 4 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    distanceInfo,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 5 }
                ));
            } 
            else if (displayLines >= _MED_DISPLAY_MIN_LINES)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Current Status",
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 1 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _getLineDivider(),
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 2 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    pingMsg->status,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    ageInfo,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 4 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    distanceInfo,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 5 }
                ));
            } 
            else if (displayLines >= _MIN_DISPLAY_MIN_LINES)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Current Status",
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 1 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    pingMsg->status,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 2 }
                ));

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    ageInfo,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 }
                ));
            }
            else
            {
                ESP_LOGE(TAG, "DisplaySentMessageState: Not enough lines to display message info");
            }
        }

        std::string _getLineDivider()
        {
            return "=====================";
        }

        std::string _displayMessageAge(MessagePing *ping)
        {
            if (!ping) return "";

            time_t now = 0;
            time_t msgTime = NavigationModule::Utilities::PackedToTimeT(ping->time, ping->date);

            if (msgTime <= 0 || now <= msgTime)
            {
                return "<1m";
            }  

            time_t diffSec = now - msgTime;

            ESP_LOGV(TAG, "Time diff: %lld seconds", (long long)diffSec);

            if (diffSec >= 86400)
            {
                return ">1d";
            }

            uint8_t diffHours   = diffSec / 3600;
            uint8_t diffMinutes = (diffSec % 3600) / 60;

            if (diffHours > 0)
            {
                return std::to_string(diffHours) + "h";
            }

            if (diffMinutes > 0)
            {
                return std::to_string(diffMinutes) + "m";
            }

            return "<1m";
        }
    };

} // namespace DisplayModule
