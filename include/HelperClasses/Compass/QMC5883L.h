#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <QMC5883LCompass.h>

class QMC5883L : public CompassInterface
{
public:
    QMC5883L() : CompassInterface()
    {
        _Compass.init();
        _Compass.read();    
    }

    QMC5883L(uint8_t addr) : CompassInterface()
    {
        _Compass.setADDR(addr);
        _Compass.init();
        _Compass.read();
    }

    ~QMC5883L()
    {
    }

    int GetAzimuth()
    {
        _Compass.read();
        auto azimuth = _Compass.getAzimuth();

        if (_InvertX)
        {
            azimuth = InvertXAzimuth(azimuth);
        }

        if (_InvertY)
        {
            azimuth = InvertYAzimuth(azimuth);
        }

        return azimuth;
    }

    void PrintRawValues()
    {
        _Compass.read();
        
        auto x = _Compass.getX();
        auto y = _Compass.getY();
        auto z = _Compass.getZ();
        Serial.print("X: ");
        Serial.print(x);
        Serial.print(" Y: ");
        Serial.print(y);
        Serial.print(" Z: ");
        Serial.println(z);
    }

    // Calibration methods
    void BeginCalibration()
    {
        _xMin = 1000000000;
        _xMax = -1000000000;

        _yMin = 1000000000;
        _yMax = -1000000000;

        _zMin = 1000000000;
        _zMax = -1000000000;
    }

    void IterateCalibration()
    {
        _Compass.read();

        _xMin = min(_xMin, _Compass.getX());
        _xMax = max(_xMax, _Compass.getX());

        _yMin = min(_yMin, _Compass.getY());
        _yMax = max(_yMax, _Compass.getY());

        _zMin = min(_zMin, _Compass.getZ());
        _zMax = max(_zMax, _Compass.getZ());
    }

    void EndCalibration()
    {
        _Compass.setCalibration(_xMin, _xMax, _yMin, _yMax, _zMin, _zMax);
    }

    void GetCalibrationData(JsonDocument &doc)
    {
        doc["xMin"] = _xMin;
        doc["xMax"] = _xMax;

        doc["yMin"] = _yMin;
        doc["yMax"] = _yMax;

        doc["zMin"] = _zMin;
        doc["zMax"] = _zMax;
    }

    void SetCalibrationData(JsonDocument &doc)
    {
        _xMin = doc["xMin"].as<int>();
        _xMax = doc["xMax"].as<int>();

        _yMin = doc["yMin"].as<int>();
        _yMax = doc["yMax"].as<int>();

        _zMin = doc["zMin"].as<int>();
        _zMax = doc["zMax"].as<int>();

        _Compass.setCalibration(_xMin, _xMax, _yMin, _yMax, _zMin, _zMax);
    }

    void SetInvertX(bool invert)
    {
        _InvertX = invert;
    }

    void SetInvertY(bool invert)
    {
        _InvertY = invert;
    }

protected:
    QMC5883LCompass _Compass;
    bool _InvertX = false;
    bool _InvertY = false;

    // Callibration data
    int _xMin = 0;
    int _xMax = 0;

    int _yMin = 0;
    int _yMax = 0;

    int _zMin = 0;
    int _zMax = 0;

    int InvertXAzimuth(int azimuth)
    {
        return (360 + (-1 * (azimuth - 180))) % 360;
    }

    int InvertYAzimuth(int azimuth)
    {
        return 360 - azimuth;
    }
};