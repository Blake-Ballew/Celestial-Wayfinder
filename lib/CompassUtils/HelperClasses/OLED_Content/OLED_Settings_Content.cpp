#include "OLED_Settings_Content.h"

OLED_Settings_Content::OLED_Settings_Content(Adafruit_SSD1306 *disp, ArduinoJson::JsonDocument *jsonDoc)
{
    this->display = disp;
    this->type = ContentType::SETTINGS;
    ArduinoJson::JsonVariant variant = jsonDoc->as<ArduinoJson::JsonVariant>();

    this->settings = new JsonVariantStack;
    this->settings->variant = variant;
    this->settings->type = getVariantType(variant);
    this->settings->next = NULL;

    if (this->settings->type == JSON_VARIANT_TYPE_OBJECT || this->settings->type == JSON_VARIANT_TYPE_ARRAY)
    {
        this->settingsIndex = 0;
    }
}

OLED_Settings_Content::~OLED_Settings_Content()
{
    delete this->settings;
}

void OLED_Settings_Content::printContent()
{
#if DEBUG == 1
    Serial.println("OLED_Settings_Content::printContent()");
#endif
    JSON_VARIANT_TYPE variantType = getVariantType(settings->variant);
    this->display->fillRect(6, 8, OLED_WIDTH - 12, OLED_HEIGHT - 16, SSD1306_BLACK);

    switch (variantType)
    {
    case (uint8_t)JSON_VARIANT_TYPE_NULL:
        break;
    case (uint8_t)JSON_VARIANT_TYPE_OBJECT:
    {
        ArduinoJson::JsonObject::iterator it = this->settings->variant.as<ArduinoJson::JsonObject>().begin();
        it += settingsIndex;
        size_t textLength = it->key().size();
        this->display->setCursor(OLED_Content::centerTextHorizontal(it->key().c_str()), 8);
        this->display->print(it->key().c_str());
        printVariantValue(it->value());
        break;
    }
    case (uint8_t)JSON_VARIANT_TYPE_ARRAY:
    {
        ArduinoJson::JsonArray::iterator it = this->settings->variant.as<ArduinoJson::JsonArray>().begin();
        it += settingsIndex;
        printVariantValue(*it);
        break;
    }
    case (uint8_t)JSON_VARIANT_TYPE_BOOLEAN:
    {
        this->display->setCursor((OLED_WIDTH / 2) - 24, OLED_HEIGHT / 2 - 4);
        if (this->settings->variant.as<bool>())
        {
            this->display->print("true");
        }
        else
        {
            this->display->print("false");
        }
        break;
    }
    case (uint8_t)JSON_VARIANT_TYPE_INTEGER:
    {
        this->display->setCursor((OLED_WIDTH / 2) - 24, OLED_HEIGHT / 2 - 4);
        this->display->print(this->settings->variant.as<int>());
        break;
    }
    case (uint8_t)JSON_VARIANT_TYPE_FLOAT:
    {
        this->display->setCursor((OLED_WIDTH / 2) - 24, OLED_HEIGHT / 2 - 4);
        this->display->print(this->settings->variant.as<float>());
        break;
    }
    case (uint8_t)JSON_VARIANT_TYPE_STRING:
    {
        this->display->setCursor((OLED_WIDTH / 2) - 24, OLED_HEIGHT / 2 - 4);
        this->display->print(this->settings->variant.as<const char *>());
        break;
    }
    default:
        break;
    }
    display->display();
}

void OLED_Settings_Content::encUp()
{
    JSON_VARIANT_TYPE variantType = getVariantType(settings->variant);

    switch (variantType)
    {
    case JSON_VARIANT_TYPE_OBJECT:
        if (settingsIndex == 0)
        {
            this->settingsIndex = this->settings->variant.as<ArduinoJson::JsonObject>().size() - 1;
        }
        else
        {
            this->settingsIndex--;
        }
        this->printContent();
        break;
    case JSON_VARIANT_TYPE_ARRAY:
        if (settingsIndex == 0)
        {
            this->settingsIndex = this->settings->variant.as<ArduinoJson::JsonArray>().size() - 1;
        }
        else
        {
            this->settingsIndex--;
        }
        this->printContent();
        break;
    default:
        break;
    }
}

void OLED_Settings_Content::encDown()
{
    JSON_VARIANT_TYPE variantType = getVariantType(settings->variant);

    switch (variantType)
    {
    case JSON_VARIANT_TYPE_OBJECT:
        if (settingsIndex == this->settings->variant.as<ArduinoJson::JsonObject>().size() - 1)
        {
            this->settingsIndex = 0;
        }
        else
        {
            this->settingsIndex++;
        }
        this->printContent();
        break;
    case JSON_VARIANT_TYPE_ARRAY:
        if (settingsIndex == this->settings->variant.as<ArduinoJson::JsonArray>().size() - 1)
        {
            this->settingsIndex = 0;
        }
        else
        {
            this->settingsIndex++;
        }
        this->printContent();
        break;
    default:
        break;
    }
}

void OLED_Settings_Content::refresh()
{
    size_t idx = settingsIndex;
    this->popVariant(false);
    JsonObject::iterator it = settings->variant.as<JsonObject>().begin();
    it += settingsIndex;
    this->pushVariant(it->value(), false);
    this->settingsIndex = idx;
    this->printContent();
}

size_t OLED_Settings_Content::getVariantDepth()
{
    return this->settings->variant.nesting();
}

void OLED_Settings_Content::popVariant(bool printAfter)
{
    if (this->settings->next != NULL)
    {
        JsonVariantStack *temp = this->settings;
        this->settings = this->settings->next;
        delete temp;
        this->settingsIndex = this->settings->idx;
        if (printAfter)
        {
            this->printContent();
        }
    }
}

void OLED_Settings_Content::pushVariant(ArduinoJson::JsonVariant variant, bool printAfter)
{
    JsonVariantStack *temp = new JsonVariantStack;
    temp->variant = variant;
    temp->type = getVariantType(variant);
    temp->next = this->settings;
    this->settings->idx = this->settingsIndex;
    this->settings = temp;
    this->settingsIndex = 0;
    if (printAfter)
    {
        this->printContent();
    }
}

void OLED_Settings_Content::printVariantValue(ArduinoJson::JsonVariant variant)
{
    JSON_VARIANT_TYPE variantType = getVariantType(variant);
    switch (variantType)
    {
    case JSON_VARIANT_TYPE_ARRAY:
        display->setCursor(OLED_Content::centerTextHorizontal(9), 16);
        display->print("[ . . . ]");
        break;
    case JSON_VARIANT_TYPE_OBJECT:
        display->setCursor(OLED_Content::centerTextHorizontal(9), 16);
        display->print("{ . . . }");
        break;
    case JSON_VARIANT_TYPE_BOOLEAN:

        if (variant.as<bool>())
        {
            display->setCursor(OLED_Content::centerTextHorizontal(4), 16);
            display->print("true");
        }
        else
        {
            display->setCursor(OLED_Content::centerTextHorizontal(5), 16);
            display->print("false");
        }
        break;
    case JSON_VARIANT_TYPE_INTEGER:
    {
        int num = variant.as<int>();
        display->setCursor(OLED_Content::centerTextHorizontal(OLED_Content::getIntLength(num)), 16);
        display->print(variant.as<int>());
        break;
    }
    case JSON_VARIANT_TYPE_FLOAT:
    {
        double num = variant.as<double>();
        display->setCursor(OLED_Content::centerTextHorizontal(5), 16);
        display->print(num, 6);
        break;
    }
    case JSON_VARIANT_TYPE_STRING:
    {
        display->setCursor(OLED_Content::centerTextHorizontal(variant.as<const char *>()), 16);
        display->print(variant.as<const char *>());
        break;
    }
    case JSON_VARIANT_CONFIGURABLE_ENUM:
    {
        const char *str = variant["valTxt"][variant["cfgVal"].as<uint8_t>()].as<const char *>();
        display->setCursor(OLED_Content::centerTextHorizontal(str), 16);
        display->print(str);
        break;
    }
    case JSON_VARIANT_CONFIGURABLE_STRING:
    {
        display->setCursor(OLED_Content::centerTextHorizontal(variant["cfgVal"].as<const char *>()), 16);
        display->print(variant["cfgVal"].as<const char *>());
        break;
    }
    case JSON_VARIANT_CONFIGURABLE_INTEGER:
    {
        if (variant["signed"].as<bool>())
        {
            int32_t num = variant["cfgVal"].as<int32_t>();
            display->setCursor(OLED_Content::centerTextHorizontal(OLED_Content::getIntLength(num)), 16);
            display->print(num);
        }
        else
        {
            uint32_t num = variant["cfgVal"].as<uint32_t>();
            display->setCursor(OLED_Content::centerTextHorizontal(OLED_Content::getIntLength(num)), 16);
            display->print(num);
        }
        break;
    }
    case JSON_VARIANT_CONFIGURABLE_FLOAT:
    {
        double num = variant["cfgVal"].as<double>();
        display->setCursor(OLED_Content::centerTextHorizontal(5), 16);
        display->print(num, 6);
        break;
    }
    }
}

JSON_VARIANT_TYPE OLED_Settings_Content::getVariantType(ArduinoJson::JsonVariant variant)
{
    if (variant.isNull())
    {
        return JSON_VARIANT_TYPE_NULL;
    }
    else if (variant.is<bool>())
    {
        return JSON_VARIANT_TYPE_BOOLEAN;
    }
    else if (variant.is<int>())
    {
        return JSON_VARIANT_TYPE_INTEGER;
    }
    else if (variant.is<float>())
    {
        return JSON_VARIANT_TYPE_FLOAT;
    }
    else if (variant.is<const char *>())
    {
        return JSON_VARIANT_TYPE_STRING;
    }
    else if (variant.is<ArduinoJson::JsonArray>())
    {
        return JSON_VARIANT_TYPE_ARRAY;
    }
    else if (variant.is<ArduinoJson::JsonObject>())
    {
        ArduinoJson::JsonObject obj = variant.as<ArduinoJson::JsonObject>();
        if (obj.containsKey("cfgType") &&
            obj.containsKey("cfgVal") &&
            obj.containsKey("dftVal"))
        {
            switch (obj["cfgType"].as<uint8_t>())
            {
            case (uint8_t)JSON_VARIANT_CONFIGURABLE_STRING:
                if (obj.containsKey("maxLen"))
                    return JSON_VARIANT_CONFIGURABLE_STRING;
            case (uint8_t)JSON_VARIANT_CONFIGURABLE_INTEGER:
                if (obj.containsKey("maxVal") &&
                    obj.containsKey("minVal") &&
                    obj.containsKey("incVal") &&
                    obj.containsKey("signed"))
                    return JSON_VARIANT_CONFIGURABLE_INTEGER;
            case (uint8_t)JSON_VARIANT_CONFIGURABLE_FLOAT:
                if (obj.containsKey("minVal") &&
                    obj.containsKey("maxVal") &&
                    obj.containsKey("incVal"))
                    return JSON_VARIANT_CONFIGURABLE_FLOAT;
            case (uint8_t)JSON_VARIANT_CONFIGURABLE_BOOL:
                return JSON_VARIANT_CONFIGURABLE_BOOL;
            case (uint8_t)JSON_VARIANT_CONFIGURABLE_ENUM:
                if (obj.containsKey("vals") &&
                    obj.containsKey("valTxt"))
                    return JSON_VARIANT_CONFIGURABLE_ENUM;
            default:
                return JSON_VARIANT_TYPE_OBJECT;
            }
        }
        return JSON_VARIANT_TYPE_OBJECT;
    }
    else
    {
        return JSON_VARIANT_TYPE_NULL;
    }
}