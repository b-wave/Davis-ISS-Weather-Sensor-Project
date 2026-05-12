//File: AS3935_SPI_Driver.cpp
//---------------------------
#include "AS3935_SPI_Driver.h"
void as3935_spi_isr();

static AS3935_SPI_Driver* g_as3935_spi_instance = nullptr;

void  as3935_spi_isr() {
    if (g_as3935_spi_instance)
        g_as3935_spi_instance->_irqFlag = true;
}

static const uint8_t AS3935_REG_INT      = 0x03;
static const uint8_t AS3935_REG_ENERGY_H = 0x04;
static const uint8_t AS3935_REG_ENERGY_M = 0x05;
static const uint8_t AS3935_REG_ENERGY_L = 0x06;
static const uint8_t AS3935_REG_DISTANCE = 0x07;

AS3935_SPI_Driver::AS3935_SPI_Driver(uint8_t csPin, uint8_t irqPin)
    : _csPin(csPin), _irqPin(irqPin), _irqFlag(false) {
    g_as3935_spi_instance = this;
}

bool AS3935_SPI_Driver::begin() {
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    pinMode(_irqPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_irqPin), as3935_spi_isr, RISING);

    SPI.begin();
    return true;
}

bool AS3935_SPI_Driver::read() {
    return true;
}

uint8_t AS3935_SPI_Driver::readReg(uint8_t reg) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(reg & 0x3F);
    uint8_t v = SPI.transfer(0x00);
    digitalWrite(_csPin, HIGH);
    return v;
}

void AS3935_SPI_Driver::writeReg(uint8_t reg, uint8_t value) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(0x40 | (reg & 0x3F));
    SPI.transfer(value);
    digitalWrite(_csPin, HIGH);
}

uint8_t AS3935_SPI_Driver::readInterruptSource() {
    return readReg(AS3935_REG_INT) & 0x0F;
}

bool AS3935_SPI_Driver::lightningEvent() {
    if (!_irqFlag) return false;
    _irqFlag = false;
    return (readInterruptSource() == 0x08);
}

bool AS3935_SPI_Driver::noiseEvent() {
    if (!_irqFlag) return false;
    _irqFlag = false;
    return (readInterruptSource() == 0x01);
}

bool AS3935_SPI_Driver::disturberEvent() {
    if (!_irqFlag) return false;
    _irqFlag = false;
    return (readInterruptSource() == 0x04);
}

uint8_t AS3935_SPI_Driver::getDistanceKm() {
    return readReg(AS3935_REG_DISTANCE) & 0x3F;
}

uint8_t AS3935_SPI_Driver::getEnergy() {
    uint32_t e = 0;
    e |= (uint32_t)readReg(AS3935_REG_ENERGY_H) << 16;
    e |= (uint32_t)readReg(AS3935_REG_ENERGY_M) << 8;
    e |= (uint32_t)readReg(AS3935_REG_ENERGY_L);
    if (e == 0) return 0;
    while (e > 255) e >>= 1;
    return (uint8_t)e;
}

