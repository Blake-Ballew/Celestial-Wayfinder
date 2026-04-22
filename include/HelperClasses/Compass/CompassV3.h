#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <math.h>
#include <unordered_set>
#include "esp_log.h"
#include "ArduinoJson.h"

static const char *TAG_COMPASS_V3 = "COMPASS_V3";

// ============================================================
// CompassV3
//
// Implements CompassInterface for hardware v3.
// Magnetometer: MMC5603NJ (I2C 0x30)
// IMU: BMI323 (I2C 0x69) OR LSM6DSO (I2C 0x6A).
// The caller supplies the set of I2C addresses found during boot
// (BootstrapMicrocontroller::ScannedDevices()) so no redundant bus
// scan is performed here.
// The accelerometer gravity vector is used for tilt-compensated heading,
// identical in approach to LSM303AGR on hardware v2.
// ============================================================

namespace
{
    static const float   V3_AZIMUTH_OFFSET = -90.0f;
    // These must be at namespace scope, not class static constexpr, because
    // unordered_set::count(const T&) ODR-uses them and C++14 requires an
    // out-of-class definition for static constexpr members that are ODR-used.
    static constexpr uint8_t V3_BMI323_ADDR = 0x69;
    static constexpr uint8_t V3_LSM6DS_ADDR = 0x6A;
}

struct V3Vector
{
    float x;
    float y;
    float z;
};

enum class ImuV3Type
{
    NONE,
    BMI323,
    LSM6DS,
};

class CompassV3 : public CompassInterface
{
public:
    explicit CompassV3(const std::unordered_set<uint8_t> &scannedDevices)
    {
        if (scannedDevices.count(uint8_t{MMC5603_ADDR}))
        {
            InitializeMmc5603();
        }
        else
        {
            ESP_LOGE(TAG_COMPASS_V3, "MMC5603 (0x30) not found in scanned devices — no magnetometer!");
        }
        SelectAndInitImu(scannedDevices);
    }

    ~CompassV3() {}

    int GetAzimuth() override
    {
        V3Vector mag = ReadMmc5603();
        V3Vector accel = ReadAccelerometer();

        ESP_LOGI(TAG_COMPASS_V3, "Mag raw    X:%.3f Y:%.3f Z:%.3f", mag.x, mag.y, mag.z);
        ESP_LOGI(TAG_COMPASS_V3, "Accel      X:%.3f Y:%.3f Z:%.3f", accel.x, accel.y, accel.z);

        if (_isCalibrated)
        {
            mag.x -= (_xMin + _xMax) / 2.0f;
            mag.y -= (_yMin + _yMax) / 2.0f;
            mag.z -= (_zMin + _zMax) / 2.0f;
            ESP_LOGI(TAG_COMPASS_V3, "Mag cal    X:%.3f Y:%.3f Z:%.3f", mag.x, mag.y, mag.z);
        }

        float heading = 360.0f - ComputeHeading(mag, accel);
        ESP_LOGI(TAG_COMPASS_V3, "Heading:%.1f  imu:%s",
            heading,
            _imuType == ImuV3Type::BMI323 ? "BMI323" :
            _imuType == ImuV3Type::LSM6DS ? "LSM6DS" : "NONE");

        return static_cast<int>(roundf(heading));
    }

    void PrintRawValues() override
    {
        V3Vector mag = ReadMmc5603();
        V3Vector accel = ReadAccelerometer();
        ESP_LOGD(TAG_COMPASS_V3,
            "Mag X:%.2f Y:%.2f Z:%.2f  Accel X:%.3f Y:%.3f Z:%.3f",
            mag.x, mag.y, mag.z, accel.x, accel.y, accel.z);
    }

    void BeginCalibration() override
    {
        _xMin = _yMin = _zMin = 1000000000.0f;
        _xMax = _yMax = _zMax = -1000000000.0f;
    }

    void IterateCalibration() override
    {
        V3Vector mag = ReadMmc5603();

        _xMin = min(_xMin, mag.x);
        _xMax = max(_xMax, mag.x);

        _yMin = min(_yMin, mag.y);
        _yMax = max(_yMax, mag.y);

        _zMin = min(_zMin, mag.z);
        _zMax = max(_zMax, mag.z);
    }

    void EndCalibration() override
    {
        _isCalibrated = true;
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
        if (!doc.containsKey("xMin") || !doc.containsKey("xMax") ||
            !doc.containsKey("yMin") || !doc.containsKey("yMax") ||
            !doc.containsKey("zMin") || !doc.containsKey("zMax"))
        {
            ESP_LOGW(TAG_COMPASS_V3, "SetCalibrationData: missing keys, ignoring");
            return;
        }

        _xMin = doc["xMin"].as<float>();
        _xMax = doc["xMax"].as<float>();

        _yMin = doc["yMin"].as<float>();
        _yMax = doc["yMax"].as<float>();

        _zMin = doc["zMin"].as<float>();
        _zMax = doc["zMax"].as<float>();

        // Sanity check: if every value is zero the data is meaningless
        // (e.g. loaded from a stale file written by a different sensor).
        if (_xMin == 0 && _xMax == 0 && _yMin == 0 && _yMax == 0 && _zMin == 0 && _zMax == 0)
        {
            ESP_LOGW(TAG_COMPASS_V3, "SetCalibrationData: all values are zero, ignoring");
            return;
        }

        ESP_LOGI(TAG_COMPASS_V3, "Calibration loaded: X[%.2f,%.2f] Y[%.2f,%.2f] Z[%.2f,%.2f]",
            _xMin, _xMax, _yMin, _yMax, _zMin, _zMax);
        _isCalibrated = true;
    }

private:

    ImuV3Type _imuType = ImuV3Type::NONE;

    bool _isCalibrated = false;
    float _xMin = 0, _xMax = 0;
    float _yMin = 0, _yMax = 0;
    float _zMin = 0, _zMax = 0;

    // ----------------------------------------------------------------
    // MMC5603 (magnetometer, I2C 0x30)
    // ----------------------------------------------------------------

    static constexpr uint8_t  MMC5603_ADDR       = 0x30;
    static constexpr uint8_t  MMC5603_PRODUCT_ID  = 0x39;
    static constexpr uint8_t  MMC5603_CTRL0_REG   = 0x1B;
    static constexpr uint8_t  MMC5603_CTRL1_REG   = 0x1C;
    static constexpr uint8_t  MMC5603_CTRL2_REG   = 0x1D;
    static constexpr uint8_t  MMC5603_ODR_REG     = 0x1A;
    static constexpr uint8_t  MMC5603_OUT_X_L     = 0x00;
    static constexpr float    MMC5603_GAUSS_LSB   = 0.00625f;   // Gauss per LSB

    void InitializeMmc5603()
    {
        ESP_LOGI(TAG_COMPASS_V3, "MMC5603 init: starting");

        // Parameter reset
        WriteI2C(MMC5603_ADDR, MMC5603_CTRL1_REG, 0x80);
        delay(20);
        ESP_LOGI(TAG_COMPASS_V3, "MMC5603 init: parameter reset done");

        // Magnetic set / reset to clear sense-coil offsets
        WriteI2C(MMC5603_ADDR, MMC5603_CTRL0_REG, 0x08);
        delay(2);
        WriteI2C(MMC5603_ADDR, MMC5603_CTRL0_REG, 0x10);
        delay(2);
        ESP_LOGI(TAG_COMPASS_V3, "MMC5603 init: SET/RESET done");

        // Enable CMM_FREQ_EN (continuous mode frequency enable)
        WriteI2C(MMC5603_ADDR, MMC5603_CTRL0_REG, 0x80);

        // Enable continuous mode in CTRL2
        WriteI2C(MMC5603_ADDR, MMC5603_CTRL2_REG, 0x10);
        delay(2);

        // Set ODR to 255 Hz
        WriteI2C(MMC5603_ADDR, MMC5603_ODR_REG, 255);
        delay(2);
        ESP_LOGI(TAG_COMPASS_V3, "MMC5603 init: continuous mode + ODR configured");

        // Verify chip ID
        Wire.beginTransmission(MMC5603_ADDR);
        Wire.write(MMC5603_PRODUCT_ID);
        Wire.endTransmission();
        Wire.requestFrom(MMC5603_ADDR, (uint8_t)1);
        uint8_t chipId = Wire.available() ? Wire.read() : 0;
        if (chipId == 0x10)
        {
            ESP_LOGI(TAG_COMPASS_V3, "MMC5603 init: OK (chip ID 0x%02X)", chipId);
        }
        else
        {
            ESP_LOGE(TAG_COMPASS_V3, "MMC5603 init: unexpected chip ID 0x%02X — reads will be garbage", chipId);
        }
    }

    V3Vector ReadMmc5603()
    {
        Wire.beginTransmission(MMC5603_ADDR);
        Wire.write(MMC5603_OUT_X_L);
        Wire.endTransmission();
        uint8_t bytesReceived = Wire.requestFrom(MMC5603_ADDR, (uint8_t)9);

        if (bytesReceived != 9)
        {
            ESP_LOGE(TAG_COMPASS_V3, "MMC5603 read: expected 9 bytes, got %d", bytesReceived);
            return { 0.0f, 0.0f, 0.0f };
        }

        uint8_t buf[9] = {};
        for (int i = 0; i < 9 && Wire.available(); i++)
            buf[i] = Wire.read();

        // 20-bit signed values (MSB in buf[0..5], fractional nibbles in buf[6..8])
        int32_t x = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[6] >> 4);
        int32_t y = ((uint32_t)buf[2] << 12) | ((uint32_t)buf[3] << 4) | ((uint32_t)buf[7] >> 4);
        int32_t z = ((uint32_t)buf[4] << 12) | ((uint32_t)buf[5] << 4) | ((uint32_t)buf[8] >> 4);

        // Remove center offset (unsigned 20-bit -> signed)
        x -= (1 << 19);
        y -= (1 << 19);
        z -= (1 << 19);

        return { x * MMC5603_GAUSS_LSB, y * MMC5603_GAUSS_LSB, z * MMC5603_GAUSS_LSB };
    }

    // ----------------------------------------------------------------
    // IMU detection and initialisation
    // ----------------------------------------------------------------

    // BMI323 (Bosch IMU, I2C 0x69 — SDO pulled high on Beacon V3)
    static constexpr uint8_t  BMI323_ADDR            = V3_BMI323_ADDR;
    static constexpr uint8_t  BMI323_REG_CHIP_ID     = 0x00;
    static constexpr uint8_t  BMI323_REG_ACC_CONF    = 0x20;
    static constexpr uint8_t  BMI323_REG_GYR_CONF    = 0x21;
    static constexpr uint8_t  BMI323_REG_ACC_DATA_X  = 0x03;
    static constexpr uint8_t  BMI323_REG_ACC_DATA_Y  = 0x04;
    static constexpr uint8_t  BMI323_REG_ACC_DATA_Z  = 0x05;
    static constexpr uint8_t  BMI323_REG_CMD         = 0x7E;
    static constexpr uint8_t  BMI323_REG_FEAT_IO0    = 0x10;
    static constexpr uint8_t  BMI323_REG_FEAT_IO1    = 0x11;
    static constexpr uint8_t  BMI323_REG_FEAT_IO2    = 0x12;
    static constexpr uint8_t  BMI323_REG_FEAT_IOSTAT = 0x14;
    static constexpr uint8_t  BMI323_REG_FEAT_CTRL   = 0x40;
    static constexpr uint16_t BMI323_SOFT_RESET      = 0xDEAF;
    static constexpr uint16_t BMI323_ACC_CONF_VAL    = 0x4028; // 100 Hz, ±8 g
    static constexpr uint16_t BMI323_GYR_CONF_VAL    = 0x4048; // 100 Hz, ±2000 dps
    static constexpr float    BMI323_ACC_LSB_PER_G   = 4096.0f; // ±8 g range

    // LSM6DSO (ST Micro IMU, I2C 0x6A on Beacon V3)
    static constexpr uint8_t  LSM6DS_ADDR            = V3_LSM6DS_ADDR;
    static constexpr uint8_t  LSM6DS_REG_CTRL1_XL    = 0x10;  // Accel control 1
    static constexpr uint8_t  LSM6DS_REG_OUTX_L_A    = 0x28;  // Accel output, X low byte
    static constexpr float    LSM6DS_ACCEL_SENS_2G    = 0.000061f; // g/LSB at ±2 g

    void SelectAndInitImu(const std::unordered_set<uint8_t> &scannedDevices)
    {
        if (scannedDevices.count(V3_BMI323_ADDR))
        {
            InitBmi323();
            _imuType = ImuV3Type::BMI323;
            ESP_LOGI(TAG_COMPASS_V3, "Using BMI323 IMU for tilt compensation");
        }
        else if (scannedDevices.count(V3_LSM6DS_ADDR))
        {
            InitLsm6ds();
            _imuType = ImuV3Type::LSM6DS;
            ESP_LOGI(TAG_COMPASS_V3, "Using LSM6DSO IMU for tilt compensation");
        }
        else
        {
            ESP_LOGW(TAG_COMPASS_V3, "No IMU found in scanned devices — heading will not be tilt compensated");
        }
    }

    void InitBmi323()
    {
        // Soft reset and wait
        WriteBmi323Reg16(BMI323_REG_CMD, BMI323_SOFT_RESET);
        delay(5);

        // Bring up feature engine
        WriteBmi323Reg16(BMI323_REG_FEAT_IO0,    0x0000); delay(1);
        WriteBmi323Reg16(BMI323_REG_FEAT_IO2,    0x012C); delay(1);
        WriteBmi323Reg16(BMI323_REG_FEAT_IOSTAT, 0x0001); delay(1);
        WriteBmi323Reg16(BMI323_REG_FEAT_CTRL,   0x0001); delay(10);

        // Poll until feature engine ready (max ~500 ms)
        for (int i = 0; i < 50; i++)
        {
            delay(10);
            uint8_t status = ReadBmi323Reg16(BMI323_REG_FEAT_IO1) & 0x0F;
            if (status == 0x01) break;
            if (status == 0x03)
            {
                ESP_LOGW(TAG_COMPASS_V3, "BMI323 feature engine error during init");
                return;
            }
        }

        WriteBmi323Reg16(BMI323_REG_ACC_CONF, BMI323_ACC_CONF_VAL);
        WriteBmi323Reg16(BMI323_REG_GYR_CONF, BMI323_GYR_CONF_VAL);
        delay(50);
    }

    void InitLsm6ds()
    {
        // CTRL1_XL: ODR = 104 Hz, FS = ±2 g
        WriteI2C(LSM6DS_ADDR, LSM6DS_REG_CTRL1_XL, 0x40);
        delay(10);
    }

    // ----------------------------------------------------------------
    // Accelerometer reads
    // ----------------------------------------------------------------

    V3Vector ReadAccelerometer()
    {
        switch (_imuType)
        {
            case ImuV3Type::BMI323: return ReadBmi323Accel();
            case ImuV3Type::LSM6DS: return ReadLsm6dsAccel();
            default:
                // No IMU: report gravity pointing straight down so the heading
                // math still produces a valid (though not tilt-compensated) result.
                return { 0.0f, 0.0f, 1.0f };
        }
    }

    V3Vector ReadBmi323Accel()
    {
        return {
            ConvertBmi323Accel(ReadBmi323Reg16(BMI323_REG_ACC_DATA_X)),
            ConvertBmi323Accel(ReadBmi323Reg16(BMI323_REG_ACC_DATA_Y)),
            ConvertBmi323Accel(ReadBmi323Reg16(BMI323_REG_ACC_DATA_Z)),
        };
    }

    float ConvertBmi323Accel(uint16_t raw)
    {
        int16_t s = (int16_t)raw;
        if (s == -32768) return 0.0f; // invalid sentinel per BMI323 datasheet
        return s / BMI323_ACC_LSB_PER_G;
    }

    V3Vector ReadLsm6dsAccel()
    {
        Wire.beginTransmission(LSM6DS_ADDR);
        Wire.write(LSM6DS_REG_OUTX_L_A);
        Wire.endTransmission(false);
        Wire.requestFrom(LSM6DS_ADDR, (uint8_t)6);

        int16_t rx = (int16_t)(Wire.read() | ((int16_t)Wire.read() << 8));
        int16_t ry = (int16_t)(Wire.read() | ((int16_t)Wire.read() << 8));
        int16_t rz = (int16_t)(Wire.read() | ((int16_t)Wire.read() << 8));

        return {
            rx * LSM6DS_ACCEL_SENS_2G,
            ry * LSM6DS_ACCEL_SENS_2G,
            rz * LSM6DS_ACCEL_SENS_2G,
        };
    }

    // ----------------------------------------------------------------
    // Heading computation (identical algorithm to LSM303AGR)
    // ----------------------------------------------------------------

    float ComputeHeading(V3Vector &mag, V3Vector &accel)
    {
        V3Vector from = { 1, 0, 0 };
        V3Vector east, north;

        VectorCross(mag, accel, east);
        VectorNormalize(east);

        VectorCross(accel, east, north);
        VectorNormalize(north);

        float heading = atan2(VectorDot(east, from), VectorDot(north, from)) * (180.0f / M_PI);
        heading += V3_AZIMUTH_OFFSET;
        if (heading < 0) heading += 360.0f;
        return heading;
    }

    void VectorCross(const V3Vector &a, const V3Vector &b, V3Vector &out)
    {
        out.x = a.y * b.z - a.z * b.y;
        out.y = a.z * b.x - a.x * b.z;
        out.z = a.x * b.y - a.y * b.x;
    }

    float VectorDot(const V3Vector &a, const V3Vector &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    void VectorNormalize(V3Vector &v)
    {
        float len = sqrt(VectorDot(v, v));
        if (len > 0) { v.x /= len; v.y /= len; v.z /= len; }
    }

    // ----------------------------------------------------------------
    // I2C helpers
    // ----------------------------------------------------------------

    void WriteI2C(uint8_t addr, uint8_t reg, uint8_t val)
    {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    // BMI323 uses a burst-read protocol: 2 dummy bytes precede the payload.
    uint16_t ReadBmi323Reg16(uint8_t reg)
    {
        Wire.beginTransmission(BMI323_ADDR);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(BMI323_ADDR, (uint8_t)4);
        if (Wire.available() < 4) return 0xFFFF;
        Wire.read(); // dummy
        Wire.read(); // dummy
        uint8_t lsb = Wire.read();
        uint8_t msb = Wire.read();
        return ((uint16_t)msb << 8) | lsb;
    }

    void WriteBmi323Reg16(uint8_t reg, uint16_t data)
    {
        Wire.beginTransmission(BMI323_ADDR);
        Wire.write(reg);
        Wire.write(data & 0xFF);
        Wire.write((data >> 8) & 0xFF);
        Wire.endTransmission();
    }
};
