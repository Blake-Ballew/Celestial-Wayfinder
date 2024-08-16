#pragma once

#include "Display_Utils.h"
#include "LoraUtils.h"

// Static class to help interface with esp32 utils compass functionality
// Eventually, all windows pertaining to the compass specific functionality will reference this class
class CompassUtils
{
public:

    static uint8_t MessageReceivedInputID;

    static void PassMessageReceivedToDisplay(uint32_t sendingUserID, bool isNew)
    {
        if (isNew) 
        {
            Display_Utils::sendInputCommand(MessageReceivedInputID);
        }

        // TODO: Maybe add refresh display command if not new
    }
};