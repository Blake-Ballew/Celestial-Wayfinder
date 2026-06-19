#pragma once

#include "Window.hpp"
#include "States/ReceivedMessagesState.hpp"
#include "States/SelectMessageState.hpp"
#include "States/TrackingState.hpp"
#include "DisplayUtilities.hpp"
#include "LoraUtils.h"
#include "NavigationUtils.h"
#include "LED_Utils.h"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "HelperClasses/PingMessage.hpp"


namespace DisplayModule
{
    class ReceivedMessagesWindow : public Window
    {
    public:
        ReceivedMessagesWindow()
        {
            _receivedState  = std::make_shared<ReceivedMessagesState>();
            _trackingState  = std::make_shared<TrackingState>();
            _selectMsgState = std::make_shared<SelectMessageState>();

            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3,
                [this](const InputContext &ctx)
                {
                    if (_currentState == _receivedState)
                        Utilities::popWindow();
                    else
                        popState();
                });

            registerInput(InputID::BUTTON_4, "Track");
            addInputCommand(InputID::BUTTON_4,
                [this](const InputContext &ctx)
                {
                    if (_currentState != _receivedState) { return; }

                    auto payload = _receivedState->buildTrackPayload();
                    if (!payload) { return; }

                    StateTransferData d;
                    d.inputID = ctx.inputID;
                    d.payload = payload;
                    pushState(_trackingState, d);
                });

            registerInput(InputID::BUTTON_2, "Reply");
            addInputCommand(InputID::BUTTON_2,
                [this](const InputContext &ctx)
                {
                    if (_currentState != _receivedState) { return; }

                    auto doc = std::make_shared<ArduinoJson::JsonDocument>();
                    auto arr = (*doc)["Messages"].to<ArduinoJson::JsonArray>();
                    for (auto it = WayfinderLoraState::SavedMessageListBegin();
                         it != WayfinderLoraState::SavedMessageListEnd(); ++it)
                    {
                        arr.add(*it);
                    }

                    StateTransferData d;
                    d.inputID = ctx.inputID;
                    d.payload = doc;
                    pushState(_selectMsgState, d);
                });

            // SelectMessageState: BUTTON_4 = Send as broadcast
            addInputCommand(InputID::BUTTON_4,
                [this](const InputContext &ctx)
                {
                    if (_currentState != _selectMsgState) { return; }

                    auto payload = _selectMsgState->buildSelectPayload();
                    if (!payload) { return; }

                    double myLat = 0, myLon = 0;
                    if (!NavigationUtils::GetCurrentLocation(myLat, myLon))
                    {
                        Utilities::clearAndDisplay(TextDrawCommand::createCenteredMessage("No Location Data"));
                        popState();
                        return;
                    }

                    time_t t;
                    uint32_t gpsTime, gpsDate;
                    System_Utils::GetCurrentUTC(t);
                    NavigationUtils::TimeTToGpsPacked(t, gpsTime, gpsDate);

                    auto ping = std::make_shared<PingMessage>();
                    ping->msgID       = esp_random();
                    ping->sender      = LoraUtils::UserID();
                    ping->bouncesLeft = 3;
                    ping->time        = gpsTime;
                    ping->date        = gpsDate;
                    ping->senderName  = LoraUtils::UserName();
                    ping->color_R     = LED_Utils::ThemeColor().r;
                    ping->color_G     = LED_Utils::ThemeColor().g;
                    ping->color_B     = LED_Utils::ThemeColor().b;
                    ping->lat         = myLat;
                    ping->lng         = myLon;
                    ping->status      = (*payload)["Message"].as<std::string>();

                    LoraUtils::SendMessage(ping);
                    popState();
                });

            setInitialState(_receivedState);
        }

        ~ReceivedMessagesWindow() = default;

    private:
        std::shared_ptr<ReceivedMessagesState> _receivedState;
        std::shared_ptr<TrackingState>         _trackingState;
        std::shared_ptr<SelectMessageState>    _selectMsgState;
    };

} // namespace DisplayModule
