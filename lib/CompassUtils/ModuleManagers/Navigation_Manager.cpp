#include "Navigation_Manager.h"

QMC5883LCompass Navigation_Manager::compass;
TinyGPSPlus Navigation_Manager::gps;

void Navigation_Manager::init()
{
#if DEBUG == 1
    Serial.println("Navigation_Manager::init()");
#endif
    // Compass Setup
    compass.init();
    compass.read();

    // GPS Setup
    Serial2.begin(9600);
}

void Navigation_Manager::init(uint8_t addr)
{
    // Compass Setup
    compass.setADDR(addr);
    compass.init();
    compass.read();

    // GPS Setup
    Serial2.begin(9600);
}

void Navigation_Manager::read()
{
    compass.read();
}

int Navigation_Manager::getX()
{
    return compass.getX();
}

int Navigation_Manager::getY()
{
    return compass.getY() * -1;
}

int Navigation_Manager::getZ()
{
    return compass.getZ();
}

int Navigation_Manager::InvertXAzimuth(int azimuth)
{
    return (360 + (-1 * (azimuth - 180))) % 360;
}

int Navigation_Manager::InvertYAzimuth(int azimuth)
{
    return 360 - azimuth;
}

int Navigation_Manager::getAzimuth()
{
    return compass.getAzimuth();
}

void Navigation_Manager::getDirection(char *direction)
{
    compass.getDirection(direction, InvertXAzimuth(getAzimuth()));
}

double Navigation_Manager::getDistanceTo(double lat, double lon)
{
#if USE_FAKE_GPS_COORDS != 0
    return gps.distanceBetween(FAKE_GPS_LAT, FAKE_GPS_LON, lat, lon);
#else
    updateGPS();
    return gps.distanceBetween(gps.location.lat(), gps.location.lng(), lat, lon);
#endif
}

double Navigation_Manager::getHeadingTo(double lat, double lon)
{
#if USE_FAKE_GPS_COORDS != 0
    return gps.courseTo(FAKE_GPS_LAT, FAKE_GPS_LON, lat, lon);
#else
    updateGPS();
    return gps.courseTo(gps.location.lat(), gps.location.lng(), lat, lon);
#endif
}

void Navigation_Manager::updateGPS()
{
    while (Serial2.available() > 0)
    {
        gps.encode(Serial2.read());
    }
}

TinyGPSLocation Navigation_Manager::getLocation()
{
    return gps.location;
}

TinyGPSTime Navigation_Manager::getTime()
{
    return gps.time;
}

TinyGPSDate Navigation_Manager::getDate()
{
    return gps.date;
}

uint64_t Navigation_Manager::getTimeDifference(uint32_t time1, uint32_t date1, uint32_t time2, uint32_t date2)
{
#if DEBUG == 1
    Serial.println("Navigation_Manager::getTimeDifference()");
    Serial.print("time1: ");
    Serial.println(time1);
    Serial.print("date1: ");
    Serial.println(date1);
    Serial.print("time2: ");
    Serial.println(time2);
    Serial.print("date2: ");
    Serial.println(date2);
#endif

    uint8_t csec1, sec1, min1, hour1, day1, month1 = 0;
    uint16_t year1 = 0;
    uint8_t csec2, sec2, min2, hour2, day2, month2 = 0;
    uint16_t year2 = 0;

    csec1 = time1 % 100;
    sec1 = (time1 / 100) % 100;
    min1 = (time1 / 10000) % 100;
    hour1 = (time1 / 1000000) % 100;
    day1 = date1 % 100;
    month1 = (date1 / 100) % 100;
    year1 = (date1 / 10000) % 10000;

    csec2 = time2 % 100;
    sec2 = (time2 / 100) % 100;
    min2 = (time2 / 10000) % 100;
    hour2 = (time2 / 1000000) % 100;
    day2 = date2 % 100;
    month2 = (date2 / 100) % 100;
    year2 = (date2 / 10000) % 10000;

    #if DEBUG == 1
    Serial.print("csec1: ");
    Serial.println(csec1);
    Serial.print("sec1: ");
    Serial.println(sec1);
    Serial.print("min1: ");
    Serial.println(min1);
    Serial.print("hour1: ");
    Serial.println(hour1);
    Serial.print("day1: ");
    Serial.println(day1);
    Serial.print("month1: ");
    Serial.println(month1);
    Serial.print("year1: ");
    Serial.println(year1);
    Serial.print("csec2: ");
    Serial.println(csec2);
    Serial.print("sec2: ");
    Serial.println(sec2);
    Serial.print("min2: ");
    Serial.println(min2);
    Serial.print("hour2: ");
    Serial.println(hour2);
    Serial.print("day2: ");
    Serial.println(day2);
    Serial.print("month2: ");
    Serial.println(month2);
    Serial.print("year2: ");
    Serial.println(year2);
    #endif

    uint64_t diff = 0;

    // Centiseconds
    if (csec1 > csec2)
    {
        csec2 += 100;
        sec2--;
    }

    diff |= (csec2 - csec1);

    // Seconds
    if (sec1 > sec2)
    {
        sec2 += 60;
        min2--;
    }

    diff |= (sec2 - sec1) << 8;

    // Minutes
    if (min1 > min2)
    {
        min2 += 60;
        hour2--;
    }

    diff |= (min2 - min1) << 16;

    // Hours
    if (hour1 > hour2)
    {
        hour2 += 24;
        day2--;
    }

    diff |= (hour2 - hour1) << 24;

    // Days
    if (day1 > day2)
    {
        // Get number of days in month
        uint8_t daysInMonth = 0;
        switch (month2)
        {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            daysInMonth = 31;
            break;
        case 2:
            if (year2 % 4 == 0)
            {
                daysInMonth = 29;
            }
            else
            {
                daysInMonth = 28;
            }
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            daysInMonth = 30;
            break;
        }

        day2 += daysInMonth;
        month2--;
    }

    // Date data stored in upper 32 bits
    diff |= ((uint64_t)(day2 - day1) << 32);

    // Months
    if (month1 > month2)
    {
        month2 += 12;
        year2--;
    }
    diff |= ((uint64_t)(month2 - month1) << 40);

    // Years
    diff |= ((uint64_t)(year2 - year1) << 48);

#if DEBUG == 1
    Serial.print("diff: ");
    Serial.println(diff);
    #endif

    return diff;

    
}

uint64_t Navigation_Manager::getTimeDifference(uint32_t time1, uint32_t date1)
{
    Navigation_Manager::updateGPS();

    uint32_t time2 = getTime().value();
    uint32_t date2 = getDate().value();

    return getTimeDifference(time1, date1, time2, date2);
}