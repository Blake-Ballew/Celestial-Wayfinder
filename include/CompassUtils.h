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
            #if DEBUG == 1
            Serial.println("CompassUtils::PassMessageReceivedToDisplay: New message received");
            #endif
            Display_Utils::sendInputCommand(MessageReceivedInputID);
        }
        #if DEBUG == 1
        else
        {
            Serial.println("CompassUtils::PassMessageReceivedToDisplay: Old message received");
        }
        #endif

        // TODO: Maybe add refresh display command if not new
    }
};