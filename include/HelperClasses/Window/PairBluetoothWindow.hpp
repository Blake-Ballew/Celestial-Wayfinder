#pragma once

#include "Window.hpp"
#include "WindowState.hpp"
#include "FnDrawCommand.hpp"
#include "TextDrawCommand.hpp"
#include "DisplayUtilities.hpp"
#include "BluetoothUtilities.hpp"
#include "HelperClasses/Window/States/BtState.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // PairBluetoothWindow
    // -------------------------------------------------------------------------
    // Shows real-time Bluetooth pairing status at 500 ms refresh:
    //   - Awaiting connection: "Waiting for Connection..."
    //   - Connected, not paired: "PIN: XXXXXX"
    //   - Connected and paired: "Connected!"
    //
    // BUTTON_3 pops the window.
    //
    // Usage:
    //   Utilities::pushWindow(std::make_shared<PairBluetoothWindow>());

    class PairBluetoothWindow : public Window
    {
    public:
        PairBluetoothWindow()
        {
            _state = std::make_shared<BtState>();

            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3,
                [](const InputContext &) { Utilities::popWindow(); });

            setInitialState(_state);
        }

    private:
        std::shared_ptr<WindowState> _state;
    };

} // namespace DisplayModule
