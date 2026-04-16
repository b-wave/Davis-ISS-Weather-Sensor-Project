#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "DavisConfig.h"
#include "DavisRFM69registers.h"
//Version 4/15/2026
class DavisRF69_RX {
public:
    DavisRF69_RX(uint8_t csPin,
                 uint8_t irqPin,
                 uint8_t resetPin,
                 DavisConfig& config);

    bool begin();
    void sleep();
    void standby();
    int16_t getRSSI();
    uint8_t getReg(uint8_t addr);

    // Set RF channel by hop index (0..hopCount-1)
    bool setChannel(uint8_t channelIndex);

    // A: simple raw receive
    bool receive(uint8_t* buffer, uint8_t& len);

    // B: receive with RSSI
    bool receive(uint8_t* buffer, uint8_t& len, int16_t& rssi);

private:
    uint8_t _csPin;
    uint8_t _irqPin;
    uint8_t _resetPin;
    DavisConfig& _config;

    uint8_t _currentChannel;

    // Low-level helpers
    void setMode(uint8_t mode);
    bool writeReg(uint8_t addr, uint8_t value);
    uint8_t readReg(uint8_t addr);
    int16_t readRSSI();
    

    void configureCommon();
    void configureRX();
    void setFrequencyFromConfig(uint8_t channelIndex);

    bool waitForPayload(uint16_t timeoutMs);
};
