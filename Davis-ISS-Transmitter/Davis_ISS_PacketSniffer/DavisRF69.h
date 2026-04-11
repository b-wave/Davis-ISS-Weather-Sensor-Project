#ifndef DAVISRF69_H
#define DAVISRF69_H

#include <Arduino.h>
#include <SPI.h>
#include "DavisRFM69registers.h"

class DavisRF69 {
public:
    DavisRF69(uint8_t csPin, uint8_t irqPin);

    void initialize();
    void setMode(uint8_t mode);
    void setFrequency(uint32_t frf);
    uint8_t readReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t value);

private:
    uint8_t _csPin;
    uint8_t _irqPin;

    void select();
    void unselect();
};

#endif
