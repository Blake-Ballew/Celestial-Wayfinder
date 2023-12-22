#pragma once

#include "Settings_Manager.h"
#include "OLED_Content.h"
#include <ArduinoJson.h>

enum JSON_VARIANT_TYPE
{
    JSON_VARIANT_TYPE_NULL = 0,
    JSON_VARIANT_TYPE_BOOLEAN,
    JSON_VARIANT_TYPE_INTEGER,
    JSON_VARIANT_TYPE_FLOAT,
    JSON_VARIANT_TYPE_STRING,
    JSON_VARIANT_TYPE_ARRAY,
    JSON_VARIANT_TYPE_OBJECT,
    JSON_VARIANT_CONFIGURABLE_BOOL,
    JSON_VARIANT_CONFIGURABLE_INTEGER,
    JSON_VARIANT_CONFIGURABLE_FLOAT,
    JSON_VARIANT_CONFIGURABLE_STRING,
    JSON_VARIANT_CONFIGURABLE_ENUM,
};

struct JsonVariantStack
{
    ArduinoJson::JsonVariant variant;
    JSON_VARIANT_TYPE type;
    size_t idx;
    JsonVariantStack *next;
};

class OLED_Settings_Content : public OLED_Content
{
public:
    JsonVariantStack *settings;
    uint16_t settingsIndex;

    OLED_Settings_Content(Adafruit_SSD1306 *disp, ArduinoJson::JsonDocument *settings);
    ~OLED_Settings_Content();
    void printContent();
    void encUp();
    void encDown();
    void popVariant(bool printAfter = false);
    void pushVariant(ArduinoJson::JsonVariant variant, bool printAfter = false);
    void refresh();
    size_t getVariantDepth();
    void printVariantValue(ArduinoJson::JsonVariant variant);
    JSON_VARIANT_TYPE getVariantType(ArduinoJson::JsonVariant variant);

private:
};