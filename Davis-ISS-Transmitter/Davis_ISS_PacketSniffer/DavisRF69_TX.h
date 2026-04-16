#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "DavisConfig.h"
#include "DavisRFM69registers.h"
//Version 4/15/2026
class DavisRF69_TX {
public:
    DavisRF69_TX(uint8_t csPin,
                 uint8_t irqPin,
                 uint8_t resetPin,
                 DavisConfig& config);

    bool begin();
    void sleep();
    void standby();
    bool setChannel(uint8_t channelIndex);
    bool sendPacket(const uint8_t* data, uint8_t len);
    bool waitForPacketSent(uint16_t timeoutMs);
    bool sendISSFrame(const uint8_t* payload, uint8_t len, uint8_t channel);
    bool writeReg(uint8_t addr, uint8_t value);
/*//Version 4/15/2026

private:
    uint8_t _csPin;
    uint8_t _irqPin;
    uint8_t _resetPin;
    DavisConfig& _config;

    uint8_t _currentChannel;
*/
private:
    void setMode(uint8_t mode);
    uint8_t readReg(uint8_t addr);

    void configureCommon();
    void configureTX();
    void setFrequencyFromConfig(uint8_t channelIndex);

    uint8_t _csPin;
    uint8_t _irqPin;
    uint8_t _resetPin;
    DavisConfig& _config; 
       
    uint8_t _currentChannel;


};
