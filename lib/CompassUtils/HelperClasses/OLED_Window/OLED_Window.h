#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "OLED_Content.h"
#include "globalDefines.h"
// #include "OLED_Manager.h"

#define BUTTON_TEXT_MAX 12

class OLED_Window
{
public:
    OLED_Window();
    OLED_Window(OLED_Window *parent);

    static Adafruit_SSD1306 *display;
    OLED_Content *content;

    virtual void drawWindow();
    void assignButton(uint32_t callbackID, int buttonNumber, const char *buttonText, uint8_t textLength);
    OLED_Window *getParentWindow();

    // Only use in child class
    virtual void execBtnCallback(uint8_t buttonNumber, void *arg);
    virtual void Pause() {}
    virtual void Resume() {}

    virtual void encUp()
    {
        if (content != NULL)
            content->encUp();
    }
    virtual void encDown()
    {
        if (content != NULL)
            content->encDown();
    }
    // void execBtnCallback(uint8_t buttonNumber, void *arg);

    virtual ~OLED_Window();

    bool isPaused = false;
    bool allowInterrupts = false;

    uint32_t btn1CallbackID = 0;
    uint32_t btn2CallbackID = 0;
    uint32_t btn3CallbackID = 0;
    uint32_t btn4CallbackID = 0;

protected:
    void (*btnCallback)(uint8_t, void *, OLED_Window *);

    char btn1Text[BUTTON_TEXT_MAX + 1];
    char btn2Text[BUTTON_TEXT_MAX + 1];
    char btn3Text[BUTTON_TEXT_MAX + 1];
    char btn4Text[BUTTON_TEXT_MAX + 1];

    OLED_Window *parentWindow;
};
