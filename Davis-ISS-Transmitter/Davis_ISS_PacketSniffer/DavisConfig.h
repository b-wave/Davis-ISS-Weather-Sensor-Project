#pragma once
#include <Arduino.h>
//Version 4/13/2026
struct FRF {
    uint8_t msb;
    uint8_t mid;
    uint8_t lsb;
};

enum DavisRegion {
    DAVIS_REGION_US_915,
    DAVIS_REGION_EU_868
};

class DavisConfig {
public:
    DavisConfig(DavisRegion region);

    uint8_t hopCount;
    const FRF* hopTable;

    uint8_t bitrateMsb;
    uint8_t bitrateLsb;

    uint8_t fdevMsb;
    uint8_t fdevLsb;

    uint8_t syncValue;

    uint8_t txId;

private:
    void loadRegion(DavisRegion region);
};
