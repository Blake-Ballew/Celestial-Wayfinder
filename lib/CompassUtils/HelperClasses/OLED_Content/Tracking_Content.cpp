#include "Tracking_Content.h"

// Had to make a whole ass file just for one static member
Tracking_Content *Tracking_Content::thisInstance = nullptr;
StaticTimer_t Tracking_Content::updateTimerBuffer;
TimerHandle_t Tracking_Content::updateTimer = xTimerCreateStatic("Statuses_Content_Update_Timer", pdMS_TO_TICKS(40), pdTRUE, (void *)0, updateDisplay, &updateTimerBuffer);
;