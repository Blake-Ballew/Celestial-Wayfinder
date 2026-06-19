#pragma once

#include "Window.hpp"
#include "States/EditStringState.hpp"
#include "DisplayUtilities.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "HelperClasses/PingMessage.hpp"

namespace DisplayModule
{
    class SaveStatusMessageWindow : public Window
    {
    public:
        SaveStatusMessageWindow()
        {
            _editState = std::make_shared<EditStringState>();

            StateTransferData initialData;
            {
                auto payload = std::make_shared<ArduinoJson::JsonDocument>();
                (*payload)["cfgVal"] = "";
                (*payload)["maxLen"] = static_cast<int>(PingMessage::STATUS_LENGTH);
                initialData.payload = payload;
            }
            setInitialState(_editState);
            _editState->onEnter(initialData);

            registerInput(InputID::BUTTON_1, "Del");
            registerInput(InputID::BUTTON_4, "Add");

            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3,
                [](const InputContext &) { Utilities::popWindow(); });

            registerInput(InputID::BUTTON_2, "Done");
            addInputCommand(InputID::BUTTON_2,
                [this](const InputContext &)
                {
                    if (_currentState != _editState) { return; }

                    const std::string &msg = _editState->currentString();
                    if (!msg.empty()) { WayfinderLoraState::AddSavedMessage(msg); }

                    Utilities::popWindow();
                });
        }

    private:
        std::shared_ptr<EditStringState> _editState;
    };

} // namespace DisplayModule
