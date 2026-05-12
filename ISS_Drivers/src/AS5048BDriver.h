/**
 * AS5048BDriver.h — AS5048B Magnetic Angle Sensor (Wind Direction)
 * Note: Not yet tested. 
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
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "SensorDriver.h"

#define AS5048B_REG_ANGLE_HI  0xFE
#define AS5048B_REG_ANGLE_LO  0xFF
#define AS5048B_REG_AGC       0xFA
#define AS5048B_RESOLUTION    16384

class AS5048BDriver : public SensorDriver {
public:
    AS5048BDriver(uint8_t addr, TwoWire* wire = &Wire);

    bool begin() override;
    bool read() override;

    bool selfTest() override;
    SensorStatus getStatus() override;
    const char* getName() override;

    float    getAngleDeg() const;
    uint16_t getAngleRaw14() const;
    uint8_t  getDavisDirectionByte() const;

    uint8_t  getAGC() const;
    bool     isMagnetOK() const;

    void     setZeroOffset(float offsetDeg);

private:
    TwoWire* _wire;
    uint8_t  _addr;

    bool     _initialized;
    SensorStatus _status;

    uint16_t _angleRaw;
    float    _angleDeg;
    uint8_t  _agc;
    float    _zeroOffset;
    bool     _magnetOK;

    uint8_t  readRegister8(uint8_t reg);
    uint16_t readRegister14(uint8_t regHi, uint8_t regLo);
};
