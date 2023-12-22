#pragma once

#include "OLED_Window.h"
#include "OLED_Manager.h"
#include "Settings_Manager.h"
#include "OLED_Settings_Content.h"
#include "Edit_Bool_Content.h"
#include "Edit_Int_Content.h"
#include "Edit_String_Content.h"
#include "Edit_Float_Content.h"
#include "Edit_Enum_Content.h"
#include <Arduino.h>
#include <ArduinoJson.h>

class OLED_Settings_Window : public OLED_Window
{
public:
    OLED_Settings_Window(OLED_Window *parent);
    ~OLED_Settings_Window();

    void execBtnCallback(uint8_t buttonNumber, void *arg);
    void drawWindow();

    // void execBtnCallback(uint8_t buttonNumber, void *arg);

protected:
    OLED_Settings_Content *settingsContent;
    Edit_Bool_Content *editBoolContent;
    Edit_Int_Content *editIntContent;
    Edit_String_Content *editStringContent;
    Edit_Float_Content *editFloatContent;
    Edit_Enum_Content *editEnumContent;

    bool saveSettings;
};
