#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "SensorDriver.h"

class AS3935_I2C_Driver : public SensorDriver {
public:
    AS3935_I2C_Driver(uint8_t addr, uint8_t irqPin);

    bool begin() override;
    bool read() override;

    bool selfTest() override { return true; }
    SensorStatus getStatus() override { return SENSOR_OK; }
    const char* getName() override { return "AS3935_I2C"; }

    bool lightningEvent();
    bool noiseEvent();
    bool disturberEvent();
    uint8_t getDistanceKm();
    uint8_t getEnergy();

private:
    uint8_t _addr, _irqPin;
    volatile bool _irqFlag;

    uint8_t readReg(uint8_t reg);
    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readInterruptSource();

    friend void as3935_i2c_isr();
};
