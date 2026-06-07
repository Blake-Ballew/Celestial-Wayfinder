#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <math.h>
#include "esp_log.h"
#include "ArduinoJson.hpp"

static const char *TAG_COMPASS_QMC = "COMPASS";

// Minimal QMC5883L driver — written against the public datasheet register map.
// No external library required.
namespace QMC5883L_REG
{
    static constexpr uint8_t DATA_X_LSB = 0x00; // Burst-read start: X_L, X_H, Y_L, Y_H, Z_L, Z_H
    static constexpr uint8_t CTRL1      = 0x09; // Mode | ODR | RNG | OSR
    static constexpr uint8_t SET_RESET  = 0x0B; // SET/RESET period (datasheet recommends 0x01)

    // CTRL1 field masks
    static constexpr uint8_t MODE_CONT  = 0x01; // Continuous measurement
    static constexpr uint8_t ODR_200HZ  = 0x0C; // 200 Hz output data rate  (bits 3:2 = 11)
    static constexpr uint8_t RNG_8G     = 0x10; // ±8 Gauss full-scale      (bit  4   = 1)
    static constexpr uint8_t OSR_512    = 0x00; // 512 over-sampling ratio  (bits 7:6 = 00)
}

class QMC5883L : public CompassInterface
{
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x0D;

    QMC5883L() : CompassInterface()
    {
        _init();
    }

    QMC5883L(uint8_t addr) : CompassInterface(), _addr(addr)
    {
        _init();
    }

    ~QMC5883L() {}

    int GetAzimuth() override
    {
        _read();

        // Hard-iron correction: subtract the midpoint of the calibration range
        float x = (float)_x - ((_xMin + _xMax) / 2.0f);
        float y = (float)_y - ((_yMin + _yMax) / 2.0f);

        float heading = atan2f(y, x) * (180.0f / M_PI);
        if (heading < 0.0f) heading += 360.0f;
        int azimuth = (int)heading;

        if (_InvertX) azimuth = (540 - azimuth) % 360;
        if (_InvertY) azimuth = 360 - azimuth;

        return azimuth;
    }

    void PrintRawValues() override
    {
        _read();
        ESP_LOGD(TAG_COMPASS_QMC, "X: %d Y: %d Z: %d", _x, _y, _z);
    }

    void BeginCalibration() override
    {
        _xMin =  2147483647;
        _xMax = -2147483647;
        _yMin =  2147483647;
        _yMax = -2147483647;
        _zMin =  2147483647;
        _zMax = -2147483647;
    }

    void IterateCalibration() override
    {
        _read();
        _xMin = min(_xMin, (int)_x);
        _xMax = max(_xMax, (int)_x);
        _yMin = min(_yMin, (int)_y);
        _yMax = max(_yMax, (int)_y);
        _zMin = min(_zMin, (int)_z);
        _zMax = max(_zMax, (int)_z);
    }

    void EndCalibration() override
    {
        // Calibration values are applied live in GetAzimuth(); nothing extra needed.
    }

    void GetCalibrationData(JsonDocument &doc) override
    {
        doc["xMin"] = _xMin;
        doc["xMax"] = _xMax;
        doc["yMin"] = _yMin;
        doc["yMax"] = _yMax;
        doc["zMin"] = _zMin;
        doc["zMax"] = _zMax;
    }

    void SetCalibrationData(JsonDocument &doc) override
    {
        _xMin = doc["xMin"].as<int>();
        _xMax = doc["xMax"].as<int>();
        _yMin = doc["yMin"].as<int>();
        _yMax = doc["yMax"].as<int>();
        _zMin = doc["zMin"].as<int>();
        _zMax = doc["zMax"].as<int>();
    }

    void SetInvertX(bool invert) { _InvertX = invert; }
    void SetInvertY(bool invert) { _InvertY = invert; }

protected:
    uint8_t  _addr    = DEFAULT_ADDR;
    int16_t  _x = 0, _y = 0, _z = 0;
    bool     _InvertX = false;
    bool     _InvertY = false;

    int _xMin = 0, _xMax = 0;
    int _yMin = 0, _yMax = 0;
    int _zMin = 0, _zMax = 0;

private:
    void _writeReg(uint8_t reg, uint8_t val)
    {
        Wire.beginTransmission(_addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    void _init()
    {
        _writeReg(QMC5883L_REG::SET_RESET, 0x01);
        _writeReg(QMC5883L_REG::CTRL1,
                  QMC5883L_REG::MODE_CONT |
                  QMC5883L_REG::ODR_200HZ |
                  QMC5883L_REG::RNG_8G    |
                  QMC5883L_REG::OSR_512);
        _read();
    }

    void _read()
    {
        Wire.beginTransmission(_addr);
        Wire.write(QMC5883L_REG::DATA_X_LSB);
        Wire.endTransmission(false); // repeated START

        if (Wire.requestFrom(_addr, (uint8_t)6) < 6) return;

        uint8_t buf[6];
        for (int i = 0; i < 6; i++) buf[i] = Wire.read();

        // Each axis: two bytes, little-endian, two's-complement
        _x = (int16_t)((buf[1] << 8) | buf[0]);
        _y = (int16_t)((buf[3] << 8) | buf[2]);
        _z = (int16_t)((buf[5] << 8) | buf[4]);
    }
};
