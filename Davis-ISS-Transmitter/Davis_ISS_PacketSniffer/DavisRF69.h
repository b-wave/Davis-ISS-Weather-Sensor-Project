#ifndef DAVIS_RF69_H
#define DAVIS_RF69_H

#include <Arduino.h>
#include <SPI.h>

// -----------------------------------------------------------------------------
// DavisRF69 — Minimal, deterministic RFM69 OOK receiver for Davis ISS packets
// -----------------------------------------------------------------------------
class DavisRF69
{
public:
    DavisRF69();

    // Initialize radio (SPI, CS, IRQ, RESET)
    bool initialize(uint8_t csPin, uint8_t irqPin, uint8_t rstPin);

    // Check if a packet is available (IRQ or polled)
    bool packetAvailable();

    // Read FIFO into buffer, returns number of bytes read
    uint8_t readFifo(uint8_t* buf, uint8_t maxLen);

    // Optional: expose register read/write for debugging
    uint8_t readReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t value);

private:
    uint8_t _csPin;
    uint8_t _irqPin;
    uint8_t _resetPin;

    // SPI helpers
    inline void select();
    inline void unselect();

    // Reset pulse
    void hardwareReset();
};

#endif
