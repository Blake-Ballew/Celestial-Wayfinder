#pragma once

#include "OLED_Content.h"

class Edit_String_Content : public OLED_Content
{
public:
    Edit_String_Content(Adafruit_SSD1306 *disp)
    {
        display = disp;
        type = ContentType::EDIT_STRING;
        thisInstance = this;
        currStr = nullptr;
    }

    ~Edit_String_Content()
    {
#if DEBUG == 1
        Serial.println("Edit_String_Content destructor");
#endif
        stop();
        thisInstance = nullptr;
        xTimerStop(timer, 0);
        if (currStr != nullptr)
        {
            delete[] currStr;
        }
    }

    void printContent()
    {
        if (currStr == nullptr)
        {
            return;
        }

        // Clear content area
        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

        // Print string
        display->setCursor(OLED_Content::centerTextHorizontal(cursorPos + 1), OLED_Content::selectTextLine(3));
        display->print(currStr);
        if (displayCursor)
        {
            display->print('_');
        }
        else
        {
            display->print(legalChars[cursorCharPos]);
        }

        displayCursor = !displayCursor;

        // print max length and current length
        display->setCursor(OLED_Content::centerTextHorizontal(OLED_Content::getUintLength(currStrLen) + OLED_Content::getUintLength(cursorPos) + 3), OLED_Content::selectTextLine(2));
        display->printf("(%d/%d)", cursorPos, currStrLen);

        display->display();
    }

    void encUp()
    {
        if (cursorCharPos == 0)
        {
            cursorCharPos = strlen(legalChars) - 1;
        }
        else
        {
            cursorCharPos--;
        }

        displayCursor = false;
    }

    void encDown()
    {
        cursorCharPos++;
        if (cursorCharPos > strlen(legalChars) - 1)
        {
            cursorCharPos = 0;
        }

        displayCursor = false;
    }

    void start()
    {
        if (!xTimerIsTimerActive(timer))
            xTimerStart(timer, 0);
    }

    void stop()
    {
        if (xTimerIsTimerActive(timer))
            xTimerStop(timer, 0);
    }

    void setString(const char *str, size_t maxLen, size_t pos)
    {
        if (currStr != nullptr)
        {
            delete[] currStr;
        }
        currStr = new char[maxLen + 1];
        memcpy(currStr, str, maxLen);
        currStr[maxLen] = '\0';
        currStrLen = maxLen;
        cursorPos = pos;
        cursorCharPos = 0;
    }

    void passButtonPress(uint8_t btnNumber)
    {
        // Delete
        if (btnNumber == 1)
        {
            if (cursorPos > 0)
            {
                cursorPos--;
                currStr[cursorPos] = '\0';
            }
        }

        // Enter
        if (btnNumber == 4)
        {
            if (cursorPos < currStrLen)
            {
                currStr[cursorPos] = legalChars[cursorCharPos];
                cursorPos++;
                currStr[cursorPos] = '\0';
            }
        }
    }

    const char *getString() { return currStr; }

    void clearString() { delete[] currStr; }

private:
    static void timerCallback(TimerHandle_t xTimer)
    {
        if (thisInstance == nullptr)
        {
            xTimerStop(timer, 0);
            return;
        }
        thisInstance->printContent();
    }
    const static char legalChars[];

    size_t cursorCharPos = 0;
    size_t cursorPos = 0;
    char *currStr;
    size_t currStrLen;
    static Edit_String_Content *thisInstance;
    static TimerHandle_t timer;
    static StaticTimer_t timerBuffer;
    bool displayCursor = false;
};