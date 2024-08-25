#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM303_Accel.h>

// Reads magnetometer to compute azimuth with z correction from accelerometer
class LSM303AGR : public CompassInterface
{
public:
    LSM303AGR() : CompassInterface()
    {
        
    }

    ~LSM303AGR()
    {
    }

    int GetAzimuth()
    {
        
    }

protected:

    Adafruit_LIS2MDL _CompassMagnetometer;  
    Adafruit_LSM303_Accel_Unified _CompassAccelerometer;
};