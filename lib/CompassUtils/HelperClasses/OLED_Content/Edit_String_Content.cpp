#include "Edit_String_Content.h"

Edit_String_Content *Edit_String_Content::thisInstance = nullptr;
StaticTimer_t Edit_String_Content::timerBuffer;
TimerHandle_t Edit_String_Content::timer = xTimerCreateStatic("Edit_String_Content_Update_Timer", pdMS_TO_TICKS(500), pdTRUE, (void *)0, timerCallback, &timerBuffer);

const char Edit_String_Content::legalChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}\\|;:'\",./<>?`~";