#ifndef DAVIS_RADIO_H
#define DAVIS_RADIO_H
//Version 4/15/2026
#include <Arduino.h>
#include "DavisConfig.h"
#include "DavisRF69_RX.h"  //only RX build


class DavisRadio {
public:
    DavisRadio(DavisConfig& config,
               uint8_t csPin,
               uint8_t irqPin,
               uint8_t resetPin);

    bool begin();
    void setHop(uint8_t hopIndex);
    uint8_t currentHop() const { return _hopIndex; }
    void nextHop();
    bool receiveFrame(uint8_t* buf, uint8_t& len, int16_t& rssi);
    int16_t rawRSSI();
    uint8_t readReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t val);
    void applyConfig();
    void enterRX();
    void beginRXOnly();




private:
    DavisConfig& _config;
    DavisRF69_RX _rx;     // RX only — no TX
    uint8_t _hopIndex;

    bool parseFrame(uint8_t* buf, uint8_t& len);
};

#endif
