#ifndef DAVISRF69_H
#define DAVISRF69_H

#include <Arduino.h>
#include "DavisRFM69registers.h"

class DavisRF69 {
public:
    DavisRF69();

    // Initialization
    void initialize(uint8_t cs, uint8_t irq, uint8_t rst);
    void initialize();  // legacy no‑arg version if needed

    // Radio control
    void setMode(uint8_t mode);

    // RSSI
    int16_t readRSSI(bool forceTrigger = false);

    // Packet RX
    bool receiveDone();
    uint8_t DATALEN;
    uint8_t DATA[66];

    // Debug
    void dumpRegisters();

    // Register access
    uint8_t readReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t value);

private:
    uint8_t csPin_;
    uint8_t irqPin_;
    uint8_t resetPin_;

    //volatile bool packetReceived_;
    static volatile bool packetReceived_;

    static void onDio0();

    void configureDavisRegisters();

    
};

#endif
