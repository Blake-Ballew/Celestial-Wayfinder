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

    int InvertXAzimuth(int azimuth)
    {
        return (360 + (-1 * (azimuth - 180))) % 360;
    }

    int InvertYAzimuth(int azimuth)
    {
        return 360 - azimuth;
    }
};