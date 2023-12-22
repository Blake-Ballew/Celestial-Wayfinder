#pragma once

#include "TinyGPS++.h"
#include <Wire.h>
#include <QMC5883LCompass.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include "globalDefines.h"

#define COMPASS_ADDR 0x0D
#define BOTTOM_PCB_FIX 1

#define RX_PIN 17
#define TX_PIN 16

struct Compass_Data
{
    int x;
    int y;
    int z;
    int azimuth;
};

class Navigation_Manager
{
public:
    static void init();
    static void init(uint8_t addr);

    static void read();

    static int getX();
    static int getY();
    static int getZ();

    static int getAzimuth();
    static int InvertXAzimuth(int azimuth);
    static int InvertYAzimuth(int azimuth);
    static void getDirection(char *direction);
    static double getDistanceTo(double lat, double lon);
    static double getHeadingTo(double lat, double lon);

    static void updateGPS();
    static TinyGPSLocation getLocation();
    static TinyGPSTime getTime();
    static TinyGPSDate getDate();

    static uint64_t getTimeDifference(uint32_t time1, uint32_t date1, uint32_t time2, uint32_t date2);

    static uint64_t getTimeDifference(uint32_t time1, uint32_t date1);

private:
    static QMC5883LCompass compass;
    static TinyGPSPlus gps;
};