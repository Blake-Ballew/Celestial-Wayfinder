#pragma once

#include "OLED_Content.h"

class Edit_Float_Content : public OLED_Content
{
public:
    Edit_Float_Content(Adafruit_SSD1306 *disp)
    {
        display = disp;
        type = ContentType::EDIT_FLOAT;
    }

    ~Edit_Float_Content()
    {
#if DEBUG == 1
        Serial.println("Edit_Float_Content destructor");
#endif
    }

    void encDown()
    {
        this->floatToEdit += incrementAmt;
        if (this->floatToEdit > max)
        {
            this->floatToEdit = min;
        }
        // printContent();
    }

    void encUp()
    {
        this->floatToEdit -= incrementAmt;
        if (this->floatToEdit < min)
        {
            this->floatToEdit = max;
        }
        // printContent();
    }

    void printContent()
    {
        // Clear content area
        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);
        display->setCursor(OLED_Content::centerTextHorizontal(6), OLED_Content::centerTextVertical());
        display->print(this->floatToEdit);
#if DEBUG == 1
        Serial.print("Edit_Float_Content::printContent() - ");
        Serial.println(this->floatToEdit);
        Serial.print("display x: ");
        Serial.println(display->getCursorX());
        Serial.print("display y: ");
        Serial.println(display->getCursorY());
#endif
        display->display();
    }

    void editFloat(double floatToEdit, double min, double max, double incrementAmt = 0.1f)
    {
        this->floatToEdit = floatToEdit;
        this->incrementAmt = incrementAmt;
        this->min = min;
        this->max = max;
    }

    double getFloat()
    {
        return this->floatToEdit;
    }

private:
    double floatToEdit = 0.0f;
    double incrementAmt = 0.1f;
    double min = 0.0f;
    double max = 0.0f;
};