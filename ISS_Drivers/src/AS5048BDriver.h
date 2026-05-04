/**
 * AS5048BDriver.h — AS5048B Magnetic Angle Sensor (Wind Direction)
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Reads absolute angular position from a diametrically magnetized
 * disc magnet via I2C. 14-bit resolution (0.022°/LSB).
 * Contactless — no potentiometer dead zone, no wear.
 *
 * Davis direction byte encoding:
 *   davis_dir = (uint8_t)(angle_deg * 255.0 / 360.0)
 *   Direction 0x00 = "no reading" in the Davis protocol
 *
 * I2C variant chosen over SPI (AS5048A) to keep SPI bus clear
 * for RFM69 radio + future AS3935 lightning sensor.
 */

#ifndef AS5048B_DRIVER_H
#define AS5048B_DRIVER_H

#include "SensorDriver.h"
#include <Wire.h>

// AS5048B I2C register addresses
#define AS5048B_REG_AGC        0xFA
#define AS5048B_REG_MAGNITUDE  0xFB  // 14-bit (high + low)
#define AS5048B_REG_ANGLE_HI   0xFE
#define AS5048B_REG_ANGLE_LO   0xFF
#define AS5048B_REG_ZERO_HI    0x16
#define AS5048B_REG_ZERO_LO    0x17
#define AS5048B_REG_DIAG       0xFB

// 14-bit resolution: 16384 positions per revolution
#define AS5048B_RESOLUTION     16384

class AS5048BDriver : public SensorDriver {
public:
    /**
     * @param addr  I2C address (default 0x40, configurable via A1/A2 pads)
     * @param wire  TwoWire instance
     */
    AS5048BDriver(uint8_t addr = 0x40, TwoWire* wire = &Wire);

    bool begin() override;
    bool read() override;
    bool selfTest() override;
    SensorStatus getStatus() override;
    const char* getName() override;

    // --- Angle accessors ---

    /** Wind direction in degrees (0.0 - 359.9) */
    float getAngleDeg() const;

    /** Raw 14-bit angle value (0 - 16383) */
    uint16_t getAngleRaw14() const;

    /**
     * Davis-format direction byte for packet byte 2.
     * Maps 0-360° → 1-255. Returns 0 if magnet is absent (no reading).
     */
    uint8_t getDavisDirectionByte() const;

    /** AGC (automatic gain control) value — indicates magnet signal */
    uint8_t getAGC() const;

    /** Check if magnet is present and in valid range */
    bool isMagnetOK() const;

    // --- Zero position programming ---

    /**
     * Set the current position as the zero (North) reference.
     * Call this during installation with the vane pointing North.
     * The zero offset is stored in the AS5048B's OTP registers.
     */
    bool programZeroPosition();

    /**
     * Set a software zero offset (non-persistent, lost on power cycle).
     * @param offsetDeg  Offset in degrees to subtract from readings
     */
    void setZeroOffset(float offsetDeg);

private:
    TwoWire* _wire;
    uint8_t  _addr;
    bool     _initialized;
    SensorStatus _status;

    float    _angleDeg;
    uint16_t _angleRaw;
    uint8_t  _agc;
    float    _zeroOffset;
    bool     _magnetOK;

    uint8_t  readRegister8(uint8_t reg);
    uint16_t readRegister14(uint8_t regHi, uint8_t regLo);
    void     writeRegister8(uint8_t reg, uint8_t value);
};

// =============================================================================
// Test stub
// =============================================================================
class AS5048BStub : public SensorDriver {
public:
    AS5048BStub() : _angleDeg(0.0f), _magnetOK(true) {}

    void setAngle(float degrees) { _angleDeg = fmod(degrees, 360.0f); }
    void setMagnetOK(bool ok) { _magnetOK = ok; }

    bool begin() override { return true; }
    bool read() override { return true; }
    bool selfTest() override { return true; }
    SensorStatus getStatus() override { return SENSOR_OK; }
    const char* getName() override { return "AS5048B_STUB"; }

    float    getAngleDeg()          const { return _angleDeg; }
    uint16_t getAngleRaw14()        const { return (uint16_t)(_angleDeg * AS5048B_RESOLUTION / 360.0f); }
    uint8_t  getDavisDirectionByte() const {
        if (!_magnetOK) return 0;
        uint8_t d = (uint8_t)(_angleDeg * 255.0f / 360.0f);
        return (d == 0 && _angleDeg > 0.5f) ? 1 : (d == 0 ? 1 : d);
    }
    bool isMagnetOK() const { return _magnetOK; }
    uint8_t getAGC() const { return 128; }

private:
    float _angleDeg;
    bool  _magnetOK;
};

#endif // AS5048B_DRIVER_H
