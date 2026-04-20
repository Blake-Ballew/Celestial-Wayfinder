#pragma once

#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "../../DrawCommands/LineCompassDrawCommand.hpp"

namespace DisplayModule
{
    class ViewMessageState : public WindowState
    {
    public:
        ViewMessageState()
        {
            refreshIntervalMs = 1000;
            
            bindInput(InputID::ENC_UP, "", [this](const InputContext &) {
                if (!LoraUtils::IsUnreadMessageIteratorAtBeginning())
                {
                    _currIndex--;
                    LoraUtils::DecrementUnreadMessageIterator();
                    _currentMsg = LoraUtils::GetCurrentUnreadMessage();
                }
                else if (_requestExitStateCallback)
                {
                    LED_Utils::disablePattern(_ringPointId);
                    _requestExitStateCallback();
                }

                _configureLed();
                _rebuildDrawCommands();
            });

            bindInput(InputID::ENC_DOWN, "v", [this](const InputContext &) {
                _currIndex++;
                LoraUtils::IncrementUnreadMessageIterator();
                if (LoraUtils::IsUnreadMessageIteratorAtEnd())
                {
                    LoraUtils::DecrementUnreadMessageIterator();
                    _currIndex--;
                }
                else
                    _currentMsg = LoraUtils::GetCurrentUnreadMessage();
                _configureLed();
                _rebuildDrawCommands();
            });
        }

        void onEnter(const StateTransferData &data) override
        {
            _currIndex = 1;
            LoraUtils::ResetUnreadMessageIterator();

            _ringPointId = RingPoint::RegisteredPatternID();
            ESP_LOGI(TAG, "Entering ViewMessageState with ring point ID %d", _ringPointId);

            _currentMsg = LoraUtils::GetCurrentUnreadMessage();
            _configureLed();

            LED_Utils::enablePattern(_ringPointId);

            _rebuildDrawCommands();
        }

        void assignExitCallback(std::function<void()> cb)
        {
            _requestExitStateCallback = std::move(cb);
        }

        void onTick() override
        {
            ESP_LOGI(TAG, "ViewMessageState tick: current index %d", _currIndex);

            _configureLed();
            _rebuildDrawCommands();
        }

        MessageBase *currentMessage() const { return _currentMsg; }

    private:
        uint32_t _currUserIdLookup = 0;
        size_t _currIndex = 1;

        const uint8_t _LARGE_DISPLAY_MIN_LINES = 16;
        const uint8_t _MED_DISPLAY_MIN_LINES = 8;
        const uint8_t _MIN_DISPLAY_MIN_LINES = 4;

        MessageBase *_currentMsg = nullptr;
        int _ringPointId = -1;

        std::function<void()> _requestExitStateCallback;

        void _configureLed()
        {
            StaticJsonDocument<256> cfg;
            auto pingMsg = static_cast<MessagePing *>(_currentMsg);

            if (!pingMsg)
            {
                return;
            }

            cfg["rOverride"] = pingMsg->color_R;
            cfg["gOverride"] = pingMsg->color_G;
            cfg["bOverride"] = pingMsg->color_B;

            double distance = NavigationUtils::GetDistanceTo(pingMsg->lat, pingMsg->lng);

            const size_t ledFxMin = 20;
            const size_t ledFxMax = 500;

            if (distance > ledFxMax)
            {
                distance = ledFxMax;
            } 
            else if (distance < ledFxMin)
            {
                distance = ledFxMin;
            }

            auto directionDegrees = _getMessageHeading();
            float fadeDegrees = -0.075f * distance + 61.5;

            cfg["fadeDegrees"] = fadeDegrees;
            cfg["directionDegrees"] = directionDegrees;

            LED_Utils::configurePattern(_ringPointId, cfg);
            LED_Utils::iteratePattern(_ringPointId);
        }

        void _rebuildDrawCommands()
        {
            if (LoraUtils::GetNumUnreadMessages() != _currIndex)
            {
                bindInput(InputID::ENC_DOWN, "v");
            }
            else
            {
                bindInput(InputID::ENC_DOWN, "");
            }

            clearDrawCommands();

            auto displayLines = Utilities::selectBottomTextLine();

            addDrawCommand(std::make_shared<LineCompassDrawCommand>(
                static_cast<int>(_getMessageHeading()),
                _getMessageDistance(), 
                1));

            if (displayLines >= _LARGE_DISPLAY_MIN_LINES)
            {
                TextFormat fmt(TextAlignH::CENTER, TextAlignV::LINE, 2);

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayIndex(),
                    fmt
                ));

                fmt.line = 3;
                fmt.hAlign = TextAlignH::LEFT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displaySenderLine(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::LEFT;
                fmt.line = 4;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayAgeLine(),
                    fmt
                ));

                fmt.line = 5;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _getLineDivider(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 6;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageContent(),
                    fmt
                ));
            }
            else if (displayLines >= _MED_DISPLAY_MIN_LINES)
            {
                TextFormat fmt(TextAlignH::CENTER, TextAlignV::LINE, 2);

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayIndex(),
                    fmt
                ));

                fmt.line = 3;
                fmt.hAlign = TextAlignH::LEFT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displaySenderLine(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::LEFT;
                fmt.line = 4;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayAgeLine(),
                    fmt
                ));

                fmt.line = 5;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _getLineDivider(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 6;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageContent(),
                    fmt
                ));
            }
            else if (displayLines >= _MIN_DISPLAY_MIN_LINES)
            {
                TextFormat fmt(TextAlignH::LEFT, TextAlignV::LINE, 2);
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayUserName(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::RIGHT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageAge(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 3;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageContent(),
                    fmt
                ));
            }
            else
            {
                ESP_LOGE(TAG, "Display height too small to show message content");
            }
        }

        std::string _displayIndex()
        {
            auto totalUnread = LoraUtils::GetNumUnreadMessages();
            return "[" + std::to_string(_currIndex) + " of " + std::to_string(totalUnread) + "]";
        }

        std::string _getLineDivider()
        {
            return "=====================";
        }

        std::string _displayUserName()
        {
            if (!_currentMsg) return "";
            return std::string(_currentMsg->senderName);
        }

        std::string _displaySenderLine()
        {
            static constexpr size_t LINE_WIDTH = 21;
            static constexpr const char *PREFIX = "From ";

            std::string name(_currentMsg->senderName);
            size_t prefixLen = strlen(PREFIX);
            size_t maxName = LINE_WIDTH - prefixLen - 1 - 1;
            if (name.length() > maxName)
                name = name.substr(0, maxName);

            size_t fillLen = LINE_WIDTH - prefixLen - 1 - name.length();
            return std::string(PREFIX) + std::string(fillLen, '-') + " " + name;
        }

        std::string _displayAgeLine()
        {
            static constexpr size_t LINE_WIDTH = 21;
            static constexpr const char *PREFIX = "Rcvd ";

            std::string age = _displayMessageAge() + " ago";
            size_t prefixLen = strlen(PREFIX);
            size_t maxAge = LINE_WIDTH - prefixLen - 1 - 1;
            if (age.length() > maxAge)
                age = age.substr(0, maxAge);

            size_t fillLen = LINE_WIDTH - prefixLen - 1 - age.length();
            return std::string(PREFIX) + std::string(fillLen, '-') + " " + age;
        }

        // TODO: Measure time from GPS date/time and message date/time
        std::string _displayMessageAge()
        {
            if (!_currentMsg) return "";

            auto messageAge = NavigationUtils::GetTimeDifference(
                _currentMsg->time,
                _currentMsg->date
            );

            return _currentMsg->GetMessageAge(messageAge);
        }

        std::string _displayMessageContent()
        {
            if (!_currentMsg) return "";
            auto ping = static_cast<MessagePing *>(_currentMsg);
            return std::string(ping->status);
        }

        // Probably don't need these two
        std::string _displayFromTag()
        {
            return "From";
        }
        
        std::string _displayMsgAgeTag()
        {
            return "Msg Age";
        }

        float _getMessageHeading()
        {
            if (!_currentMsg) return 0;
            auto pingMsg = static_cast<MessagePing *>(_currentMsg);

            double heading = NavigationUtils::GetHeadingTo(pingMsg->lat, pingMsg->lng);
            int azimuth = NavigationUtils::GetAzimuth();

            float directionDegrees = heading - azimuth;

            if (directionDegrees < 0)
            {
                directionDegrees += 360;
            }

            return directionDegrees;
        }

        int _getMessageDistance()
        {
            if (!_currentMsg) return 0;
            auto ping = static_cast<MessagePing *>(_currentMsg);
            
            double distance = NavigationUtils::GetDistanceTo(ping->lat, ping->lng);
            return static_cast<int>(distance);
        }
    };
}