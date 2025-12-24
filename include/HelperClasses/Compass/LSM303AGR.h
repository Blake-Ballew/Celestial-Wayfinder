#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM303_Accel.h>
#include "esp_log.h"

static const char *TAG_COMPASS_LSM = "COMPASS";

namespace
{
    const float AZIMUTH_OFFSET = -90.0f;
};

struct Vector
{
    float x;
    float y;
    float z;
};

// Reads magnetometer to compute azimuth with z correction from accelerometer
class LSM303AGR : public CompassInterface
{
public:
    LSM303AGR()
    {
        if (!Wire.available())
        {
            #if DEBUG == 1
            ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Initializing Wire");
            #endif
        }
        else
        {
            #if DEBUG == 1
            ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Wire already initialized");
            #endif
        }

        #if DEBUG == 1
        ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Initializing");
        #endif

        if (!_CompassMagnetometer.begin(0x1E, &Wire))
        {
            ESP_LOGE(TAG_COMPASS_LSM, "LSM303AGR: Magnetometer not found");
        }

        #if DEBUG == 1
        ESP_LOGI(TAG_COMPASS_LSM, "CompassMagnetometer Initialized");
        #endif

        if (!_CompassAccelerometer.begin())
        {
            ESP_LOGE(TAG_COMPASS_LSM, "LSM303AGR: Accelerometer not found");
        }

        ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Initialized");
    }

    ~LSM303AGR()
    {
    }

    int GetAzimuth()
    {
        sensors_event_t magEvent;
        sensors_event_t accelEvent;

        Vector mag;
        Vector accel;

        _CompassMagnetometer.getEvent(&magEvent);
        _CompassAccelerometer.getEvent(&accelEvent);

        float Mx = magEvent.magnetic.x;
        float My = magEvent.magnetic.y;
        float Mz = magEvent.magnetic.z;

        if (_IsCalibrated)
        {
            Mx -= (_xMin + _xMax) / 2.0f;
            My -= (_yMin + _yMax) / 2.0f;
            Mz -= (_zMin + _zMax) / 2.0f;
        }

        float Ax = accelEvent.acceleration.x;
        float Ay = accelEvent.acceleration.y;
        float Az = accelEvent.acceleration.z;

        mag.x = Mx;
        mag.y = My;
        mag.z = Mz;

        accel.x = Ax;
        accel.y = Ay;
        accel.z = Az;

        float azimuth = GetHeading(mag, accel);

        return 360.0f - azimuth;
    }

    void PrintRawValues()
    {
        sensors_event_t magEvent;
        sensors_event_t accelEvent;

        _CompassMagnetometer.getEvent(&magEvent);
        _CompassAccelerometer.getEvent(&accelEvent);

        float Mx = magEvent.magnetic.x;
        float My = magEvent.magnetic.y;
        float Mz = magEvent.magnetic.z;

        float Ax = accelEvent.acceleration.x;
        float Ay = accelEvent.acceleration.y;
        float Az = accelEvent.acceleration.z;

        // ESP_LOGD(TAG_COMPASS_LSM, "Mx: %.2f My: %.2f Mz: %.2f Ax: %.2f Ay: %.2f Az: %.2f", Mx, My, Mz, Ax, Ay, Az);
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
        sensors_event_t magEvent;
        _CompassMagnetometer.getEvent(&magEvent);

        _xMin = min(_xMin, magEvent.magnetic.x);
        _xMax = max(_xMax, magEvent.magnetic.x);

        _yMin = min(_yMin, magEvent.magnetic.y);
        _yMax = max(_yMax, magEvent.magnetic.y);

        _zMin = min(_zMin, magEvent.magnetic.z);
        _zMax = max(_zMax, magEvent.magnetic.z);
    }

    void EndCalibration()
    {
        _IsCalibrated = true;
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
        _xMin = doc["xMin"].as<float>();
        _xMax = doc["xMax"].as<float>();

        _yMin = doc["yMin"].as<float>();
        _yMax = doc["yMax"].as<float>();

        _zMin = doc["zMin"].as<float>();
        _zMax = doc["zMax"].as<float>();

        _IsCalibrated = true;
    }

protected:

    Adafruit_LIS2MDL _CompassMagnetometer = Adafruit_LIS2MDL(12345);
    Adafruit_LSM303_Accel_Unified _CompassAccelerometer = Adafruit_LSM303_Accel_Unified(54321);

    // Callibration data
    float _xMin = 0;
    float _xMax = 0;

    float _yMin = 0;
    float _yMax = 0;

    float _zMin = 0;
    float _zMax = 0;

    bool _IsCalibrated = false;

    float getTiltCompensatedAzimuth(float Mx, float My, float Mz, float Ax, float Ay, float Az) 
    {
        // Normalize accelerometer values (optional but improves stability)
        float accel_magnitude = sqrt(Ax * Ax + Ay * Ay + Az * Az);
        Ax /= accel_magnitude;
        Ay /= accel_magnitude;
        Az /= accel_magnitude;

        // Calculate pitch and roll
        float pitch = asin(-Ax);  // Pitch around X-axis
        float roll = atan2(Ay, Az);  // Roll around Y-axis

        // Tilt compensation for magnetometer
        float Mx_tc = Mx * cos(pitch) + Mz * sin(pitch);
        float My_tc = Mx * sin(roll) * sin(pitch) + My * cos(roll) - Mz * sin(roll) * cos(pitch);

        // Calculate azimuth
        float azimuth = atan2(My_tc, Mx_tc) * (180.0 / M_PI);  // Convert to degrees
        if (azimuth < 0) 
        {
            azimuth += 360.0;  // Normalize to [0, 360]
        }

        return azimuth;
    }

    float GetHeading(Vector &mag, Vector &accel)
    {
        Vector from = {1, 0, 0};
        Vector east;
        Vector north;

        VectorCross(mag, accel, east);
        VectorNormalize(east);

        VectorCross(accel, east, north);
        VectorNormalize(north);

        float heading = atan2(VectorDot(east, from), VectorDot(north, from)) * (180.0 / M_PI);
        heading += AZIMUTH_OFFSET;
        if (heading < 0)
        {
            heading += 360.0;
        }

        return heading;
    }

    void VectorCross(const Vector &a, const Vector &b, Vector &result)
    {
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
    }

    float VectorDot(const Vector &a, const Vector &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    void VectorNormalize(Vector &v)
    {
        float length = sqrt(VectorDot(v, v));
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }
};