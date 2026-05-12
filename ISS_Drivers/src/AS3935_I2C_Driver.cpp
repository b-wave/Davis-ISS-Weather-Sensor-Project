//File: AS3935_I2C_Driver.cpp
//---------------------------
#include "AS3935_I2C_Driver.h"

static AS3935_I2C_Driver* g_as3935_i2c_instance = nullptr;

void as3935_i2c_isr() {
    if (g_as3935_i2c_instance)
        g_as3935_i2c_instance->_irqFlag = true;
}

static const uint8_t AS3935_REG_INT      = 0x03;
static const uint8_t AS3935_REG_ENERGY_H = 0x04;
static const uint8_t AS3935_REG_ENERGY_M = 0x05;
static const uint8_t AS3935_REG_ENERGY_L = 0x06;
static const uint8_t AS3935_REG_DISTANCE = 0x07;

AS3935_I2C_Driver::AS3935_I2C_Driver(uint8_t addr, uint8_t irqPin)
    : _addr(addr), _irqPin(irqPin), _irqFlag(false) {
    g_as3935_i2c_instance = this;
}

bool AS3935_I2C_Driver::begin() {
    Wire.begin();
    pinMode(_irqPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_irqPin), as3935_i2c_isr, RISING);
    return true;
}

bool AS3935_I2C_Driver::read() {
    return true; 
}

//void AS3935_I2C_Driver::read() {} //old

uint8_t AS3935_I2C_Driver::readReg(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg & 0x3F);
    Wire.endTransmission(false);
    Wire.requestFrom((int)_addr, 1);
    return Wire.available() ? Wire.read() : 0;
}

void AS3935_I2C_Driver::writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_addr);
    Wire.write(0x40 | (reg & 0x3F));
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t AS3935_I2C_Driver::readInterruptSource() {
    return readReg(AS3935_REG_INT) & 0x0F;
}

bool AS3935_I2C_Driver::lightningEvent() {
    if (!_irqFlag) return false;
    _irqFlag = false;
    return (readInterruptSource() == 0x08);
}

bool AS3935_I2C_Driver::noiseEvent() {
    if (!_irqFlag) return false;
    _irqFlag = false;
    return (readInterruptSource() == 0x01);
}

bool AS3935_I2C_Driver::disturberEvent() {
    if (!_irqFlag) return false;
    _irqFlag = false;
    return (readInterruptSource() == 0x04);
}

uint8_t AS3935_I2C_Driver::getDistanceKm() {
    return readReg(AS3935_REG_DISTANCE) & 0x3F;
}

uint8_t AS3935_I2C_Driver::getEnergy() {
    uint32_t e = 0;
    e |= (uint32_t)readReg(AS3935_REG_ENERGY_H) << 16;
    e |= (uint32_t)readReg(AS3935_REG_ENERGY_M) << 8;
    e |= (uint32_t)readReg(AS3935_REG_ENERGY_L);
    if (e == 0) return 0;
    while (e > 255) e >>= 1;
    return (uint8_t)e;
}

