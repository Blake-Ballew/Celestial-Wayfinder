#pragma once

#include "OLED_Content.h"

class Edit_Bool_Content : public OLED_Content
{
public:
    Edit_Bool_Content(Adafruit_SSD1306 *disp)
    {
        display = disp;
        type = ContentType::EDIT_BOOL;
    }

    ~Edit_Bool_Content()
    {
#if DEBUG == 1
        Serial.println("Edit_Bool_Content destructor");
#endif
    }

    void encUp()
    {
        editBool = !editBool;
        printContent();
    }

    void encDown()
    {
        editBool = !editBool;
        printContent();
    }

    void printContent()
    {
        // Clear content area
        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);
        if (editBool)
        {
            display->setCursor(OLED_Content::centerTextVertical(), OLED_Content::centerTextHorizontal(4));
            display->print("True");
        }
        else
        {
            display->setCursor(OLED_Content::centerTextVertical(), OLED_Content::centerTextHorizontal(5));
            display->print("False");
        }
        display->display();
    }

    void setBool(bool boolToEdit)
    {
        editBool = boolToEdit;
    }

    bool getBool()
    {
        return editBool;
    }

private:
    bool editBool = false;
};