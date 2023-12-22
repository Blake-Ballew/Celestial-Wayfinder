#include "GPS_Content.h"

GPS_Content::GPS_Content(Adafruit_SSD1306 *disp)
{
    display = disp;
}

GPS_Content::~GPS_Content()
{
}

void GPS_Content::encUp()
{
}

void GPS_Content::encDown()
{
}

void GPS_Content::printContent()
{
    display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);

    Navigation_Manager::updateGPS();
    TinyGPSLocation loc = Navigation_Manager::getLocation();

    display->setCursor(0, 8);
    display->print(" Lat:");
#if DEBUG == 1
    Serial.print("Lat: ");
    Serial.println(loc.lat(), 10);
#endif
    display->print(loc.lat(), 10);
    display->setCursor(0, 16);
#if DEBUG == 1
    Serial.print("Long: ");
    Serial.println(loc.lng(), 10);
#endif
    display->print("Long:");
    display->print(loc.lng(), 10);

    display->display();
}