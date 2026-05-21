#pragma once

#include "Window.hpp"
#include "States/SavedStatusMsgListState.hpp"
#include "States/EditStringState.hpp"
#include "DisplayUtilities.hpp"

namespace DisplayModule
{
    class EditStatusMessagesWindow : public Window
    {
    public:
        EditStatusMessagesWindow()
        {
            _listState    = std::make_shared<SavedStatusMsgListState>();
            _editStrState = std::make_shared<EditStringState>();

            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3,
                [this](const InputContext &ctx)
                {
                    if (_currentState == _listState)
                        Utilities::popWindow();
                    else
                        popState();
                });

            registerInput(InputID::BUTTON_4, "Edit");
            addInputCommand(InputID::BUTTON_4,
                [this](const InputContext &ctx)
                {
                    if (_currentState != _listState) { return; }

                    StateTransferData d;
                    d.inputID = ctx.inputID;
                    d.payload = _listState->buildEditPayload();
                    pushState(_editStrState, d);
                });

            registerInput(InputID::BUTTON_2, "Create");
            addInputCommand(InputID::BUTTON_2,
                [this](const InputContext &ctx)
                {
                    if (_currentState == _listState)
                    {
                        StateTransferData d;
                        d.inputID = ctx.inputID;
                        d.payload = _listState->buildCreatePayload();
                        pushState(_editStrState, d);
                    }
                    else if (_currentState == _editStrState)
                    {
                        StateTransferData d;
                        d.inputID = ctx.inputID;
                        d.payload = _editStrState->buildResultPayload();
                        popState(d);
                    }
                });

            registerInput(InputID::BUTTON_1, "Del");

            setInitialState(_listState);
        }

    private:
        std::shared_ptr<SavedStatusMsgListState> _listState;
        std::shared_ptr<EditStringState>         _editStrState;
    };

} // namespace DisplayModule
