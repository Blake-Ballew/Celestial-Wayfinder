#pragma once

#include <memory>
#include <string>
#include <vector>
#include "WindowState.hpp"
#include "TextDrawCommand.hpp"
#include "LoraUtils.h"
#include "NavigationUtils.h"
#include "MessagePing.hpp"
#include "LED_Utils.h"
#include "SolidRing.hpp"

namespace DisplayModule
{
    class UnreadMessageState : public WindowState
    {
    public:
        UnreadMessageState()
        {
            bindInput(InputID::BUTTON_3, "Open", [this](const InputContext &) {
                LoraUtils::MarkMessageOpened(
                    LoraUtils::GetCurrentUnreadMessageSenderID());
                if (LoraUtils::IsUnreadMessageIteratorAtEnd() &&
                    LoraUtils::GetNumUnreadMessages() > 0)
                {
                    LoraUtils::DecrementUnreadMessageIterator();
                }
                _currentMsg = LoraUtils::GetCurrentUnreadMessage();
                _configureLed();
                _rebuildDrawCommands();
            });
            bindInput(InputID::BUTTON_2, "Reply");
            bindInput(InputID::BUTTON_4, "Track");
            bindInput(InputID::ENC_UP, "", [this](const InputContext &) {
                if (!LoraUtils::IsUnreadMessageIteratorAtBeginning())
                {
                    LoraUtils::DecrementUnreadMessageIterator();
                    _currentMsg = LoraUtils::GetCurrentUnreadMessage();
                }
                _configureLed();
                _rebuildDrawCommands();
            });
            bindInput(InputID::ENC_DOWN, "", [this](const InputContext &) {
                LoraUtils::IncrementUnreadMessageIterator();
                if (LoraUtils::IsUnreadMessageIteratorAtEnd())
                    LoraUtils::DecrementUnreadMessageIterator();
                else
                    _currentMsg = LoraUtils::GetCurrentUnreadMessage();
                _configureLed();
                _rebuildDrawCommands();
            });
        }

        void onEnter(const StateTransferData &) override
        {
            LoraUtils::ResetUnreadMessageIterator();

            _solidRingID = SolidRing::RegisteredPatternID();

            _currentMsg = LoraUtils::GetCurrentUnreadMessage();
            _configureLed();
            LED_Utils::enablePattern(_solidRingID);

            _rebuildDrawCommands();
        }

        void onExit() override
        {
            _currentMsg = nullptr;
            LED_Utils::disablePattern(_solidRingID);
            LED_Utils::clearPattern(_solidRingID);
            WindowState::onExit();
        }

        bool isAtBeginning() const
        {
            return LoraUtils::IsUnreadMessageIteratorAtBeginning();
        }

        bool hasNoUnread() const
        {
            return LoraUtils::GetNumUnreadMessages() == 0;
        }

        std::shared_ptr<ArduinoJson::JsonDocument> buildTrackPayload() const
        {
            if (!_currentMsg) { return nullptr; }

            auto ping = std::static_pointer_cast<MessagePing>(_currentMsg);
            if (!ping) { return nullptr; }

            auto doc = std::make_shared<ArduinoJson::JsonDocument>();
            (*doc)["lat"]     = ping->lat;
            (*doc)["lon"]     = ping->lng;
            (*doc)["color_R"] = ping->color_R;
            (*doc)["color_G"] = ping->color_G;
            (*doc)["color_B"] = ping->color_B;

            std::vector<MessagePrintInformation> info;
            ping->GetPrintableInformation(info);
            auto arr = (*doc)["displayTxt"].to<ArduinoJson::JsonArray>();
            for (auto &pi : info) { arr.add(pi.txt); }

            return doc;
        }

        std::shared_ptr<ArduinoJson::JsonDocument> buildReplyPayload() const
        {
            if (!_currentMsg) { return nullptr; }

            auto doc = std::make_shared<ArduinoJson::JsonDocument>();
            (*doc)["recipientID"] = _currentMsg->sender;
            return doc;
        }

    private:
        std::shared_ptr<LoraModule::LoraMessageInterface> _currentMsg;
        int _solidRingID = -1;

        void _configureLed()
        {
            if (_solidRingID < 0 || !_currentMsg) { return; }

            auto ping = std::static_pointer_cast<MessagePing>(_currentMsg);
            if (!ping) { return; }

            ArduinoJson::JsonDocument cfg;
            cfg["rOverride"] = ping->color_R;
            cfg["gOverride"] = ping->color_G;
            cfg["bOverride"] = ping->color_B;
            cfg["beginIdx"]  = 0;
            cfg["endIdx"]    = 15;
            LED_Utils::configurePattern(_solidRingID, cfg);
            LED_Utils::iteratePattern(_solidRingID);
        }

        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            if (!_currentMsg)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "No Messages",
                    TextFormat{ TextAlignH::CENTER, TextAlignV::CENTER }
                ));
                return;
            }

            std::vector<MessagePrintInformation> info;
            _currentMsg->GetPrintableInformation(info);

            for (size_t i = 0; i < info.size(); ++i)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    info[i].txt,
                    TextFormat{ TextAlignH::LEFT, TextAlignV::LINE,
                                static_cast<uint8_t>(i + 2) }
                ));
            }

            auto ageVal = NavigationUtils::GetTimeDifference(
                _currentMsg->time, _currentMsg->date);
            std::string ageStr = _currentMsg->GetMessageAge(ageVal);
            if (!ageStr.empty())
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    ageStr,
                    TextFormat{ TextAlignH::RIGHT, TextAlignV::LINE, 2 }
                ));
            }
        }
    };

} // namespace DisplayModule
