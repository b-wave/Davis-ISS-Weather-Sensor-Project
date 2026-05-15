#pragma once
#include <stdint.h>

class TxPacketEngine {
public:
    // Initialize with Davis TX ID (1–8)
    void init(uint8_t txID);

    // Build the next 8-byte Davis packet
    void buildNextPacket(uint8_t* outPkt);

    // Optional for serial shell
    void dumpSensors();

private:
    uint8_t txID = 1;
    uint8_t seqIndex = 0;

    // Internal helpers
    void encodeTemp(uint8_t* pkt);
    void encodeHumidity(uint8_t* pkt);
    void encodeWind(uint8_t* pkt);
    void encodeRain(uint8_t* pkt);
    void encodeRainRate(uint8_t* pkt);
    void encodeBattery(uint8_t* pkt);
};
