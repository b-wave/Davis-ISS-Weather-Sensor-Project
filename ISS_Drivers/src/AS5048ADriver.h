//=====================================================================
// AS5048A SPI DRIVER  (replaces AS5048BDriver)
//  - Non-blocking 90° calibration
//  - Status LED on pin 8 (default)
//=====================================================================
//File: AS5048ADriver.h
//---------------------
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "SensorDriver.h"

class AS5048ADriver : public SensorDriver {
public:
    // statusLedPin defaults to 8 so existing code still compiles
    AS5048ADriver(uint8_t csPin, uint8_t statusLedPin = 8);

    bool begin() override;
    bool read() override;

    bool selfTest() override { return true; }
    SensorStatus getStatus() override { return SENSOR_OK; }
    const char* getName() override { return "AS5048A"; }

    // Calibrated angle (0–360, with 90° at power-up reference)
    float getAngleDeg() const;
    // Davis-style 0–255 direction byte
    uint8_t getDavisDirectionByte() const;

private:
    enum CalState {
        CAL_WAIT,   // waiting for first valid reading (magnet present)
        CAL_DONE    // calibration complete, normal operation
    };

    uint8_t  _csPin;
    uint8_t  _statusLedPin;
    uint16_t _lastRaw;
    float    _lastDeg;       // calibrated angle
    float    _offsetDeg;     // offset so that power-up position = 90°
    CalState _calState;

    uint32_t _lastBlinkMs;
    bool     _ledState;

    uint16_t readRegister(uint16_t reg);
    bool     checkParity(uint16_t value);
};
