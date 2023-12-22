#pragma once

#include "OLED_Content.h"
#include "ArduinoJson.h"

class Edit_Enum_Content : public OLED_Content
{
public:
    Edit_Enum_Content(Adafruit_SSD1306 *disp)
    {
        display = disp;
        type = ContentType::EDIT_ENUM;
    }

    ~Edit_Enum_Content()
    {
#if DEBUG == 1
        Serial.println("Edit_Enum_Content destructor");
#endif
    }

    void encUp()
    {
        if (selectedValueIndex == valueArray.size() - 1)
        {
            selectedValueIndex = 0;
        }
        else
        {
            selectedValueIndex++;
        }
        printContent();
    }

    void encDown()
    {
        if (selectedValueIndex == 0)
        {
            selectedValueIndex = valueArray.size() - 1;
        }
        else
        {
            selectedValueIndex--;
        }
        printContent();
    }

    void printContent()
    {
        // Clear content area
        display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);
        ArduinoJson::JsonVariant value = textArray[selectedValueIndex];
        const char *enumText = value.as<const char *>();
        if (enumText == NULL)
        {
            uint32_t enumValue = valueArray[selectedValueIndex].as<uint32_t>();
            display->setCursor(OLED_Content::centerTextHorizontal(OLED_Content::getUintLength(enumValue)), OLED_Content::centerTextVertical());
            display->print(enumValue);
            display->display();
            return;
        }
        display->setCursor(OLED_Content::centerTextHorizontal(enumText), OLED_Content::centerTextVertical());
        display->print(enumText);
        display->display();
    }

    void assignEnum(ArduinoJson::JsonArray valueArray, ArduinoJson::JsonArray textArray, size_t index = 0)
    {
        this->valueArray = valueArray;
        this->textArray = textArray;
        selectedValueIndex = index;
        if (selectedValueIndex >= valueArray.size())
        {
            selectedValueIndex = 0;
        }
        printContent();
    }

    size_t getSelectedIndex()
    {
        return selectedValueIndex;
    }

private:
    ArduinoJson::JsonArray valueArray;
    ArduinoJson::JsonArray textArray;
    size_t selectedValueIndex = 0;
};