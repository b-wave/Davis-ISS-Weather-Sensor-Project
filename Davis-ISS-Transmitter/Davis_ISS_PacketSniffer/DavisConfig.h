#pragma once
#include <stdint.h>
class DavisRF69_RX;

enum DavisRegion {
    DAVIS_REGION_US_915,
    DAVIS_REGION_EU_868,
};

struct FRF {
    uint8_t msb;
    uint8_t mid;
    uint8_t lsb;
};

struct DavisConfig {
    // Region + hopping
    void loadRegion(DavisRegion region);
    void applyRX(DavisRF69_RX& radio);
    const FRF*  hopTable;
    uint8_t     hopCount;

    // Core RF parameters
    uint8_t bitrateMsb;
    uint8_t bitrateLsb;

    uint8_t fdevMsb;
    uint8_t fdevLsb;

    uint8_t rxBw;
    uint8_t afcBw;

    // Sync
    uint8_t syncConfig;
    uint8_t syncValue1;
    uint8_t syncValue2;

    // Packet mode
    uint8_t packetConfig1;
    uint8_t packetConfig2;
    uint8_t payloadLength;

    // Optional Davis-specific
    uint16_t txId;

    DavisConfig() = default;
    DavisConfig(DavisRegion region);

};
