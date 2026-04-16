#include "DavisConfig.h"
//Version 4/15/2026
// -----------------------------------------------------------------------------
// Base Davis US 915 MHz hop table as FRF words (same as original, compact form)
// -----------------------------------------------------------------------------
const uint32_t DAVIS_FREQ_TABLE[51] = {
    0xE1C000, // 902.3 MHz
    0xE1D000, // 902.5
    0xE1E000, // 902.7
    0xE1F000, // 902.9
    0xE20000, // 903.1
    0xE21000, // 903.3
    0xE22000, // 903.5
    0xE23000, // 903.7
    0xE24000, // 903.9
    0xE25000, // 904.1
    0xE26000, // 904.3
    0xE27000, // 904.5
    0xE28000, // 904.7
    0xE29000, // 904.9
    0xE2A000, // 905.1
    0xE2B000, // 905.3
    0xE2C000, // 905.5
    0xE2D000, // 905.7
    0xE2E000, // 905.9
    0xE2F000, // 906.1
    0xE30000, // 906.3
    0xE31000, // 906.5
    0xE32000, // 906.7
    0xE33000, // 906.9
    0xE34000, // 907.1
    0xE35000, // 907.3
    0xE36000, // 907.5
    0xE37000, // 907.7
    0xE38000, // 907.9
    0xE39000, // 908.1
    0xE3A000, // 908.3
    0xE3B000, // 908.5
    0xE3C000, // 908.7
    0xE3D000, // 908.9
    0xE3E000, // 909.1
    0xE3F000, // 909.3
    0xE40000, // 909.5
    0xE41000, // 909.7
    0xE42000, // 909.9
    0xE43000, // 910.1
    0xE44000, // 910.3
    0xE45000, // 910.5
    0xE46000, // 910.7
    0xE47000, // 910.9
    0xE48000, // 911.1
    0xE49000, // 911.3
    0xE4A000, // 911.5
    0xE4B000, // 911.7
    0xE4C000, // 911.9
    0xE4D000, // 912.1
    0xE4E000  // 912.3
};

// -----------------------------------------------------------------------------
// Helper macro: convert 24‑bit FRF word to FRF triplet (msb, mid, lsb)
// -----------------------------------------------------------------------------
#define FRF_FROM_WORD(w) { (uint8_t)((w) >> 16), (uint8_t)((w) >> 8), (uint8_t)(w) }

// -----------------------------------------------------------------------------
// US 915 MHz hop table as FRF triplets (full ISS hop emulation)
// -----------------------------------------------------------------------------
const FRF hopTable_US_915[51] = {
    FRF_FROM_WORD(0xE1C000),
    FRF_FROM_WORD(0xE1D000),
    FRF_FROM_WORD(0xE1E000),
    FRF_FROM_WORD(0xE1F000),
    FRF_FROM_WORD(0xE20000),
    FRF_FROM_WORD(0xE21000),
    FRF_FROM_WORD(0xE22000),
    FRF_FROM_WORD(0xE23000),
    FRF_FROM_WORD(0xE24000),
    FRF_FROM_WORD(0xE25000),
    FRF_FROM_WORD(0xE26000),
    FRF_FROM_WORD(0xE27000),
    FRF_FROM_WORD(0xE28000),
    FRF_FROM_WORD(0xE29000),
    FRF_FROM_WORD(0xE2A000),
    FRF_FROM_WORD(0xE2B000),
    FRF_FROM_WORD(0xE2C000),
    FRF_FROM_WORD(0xE2D000),
    FRF_FROM_WORD(0xE2E000),
    FRF_FROM_WORD(0xE2F000),
    FRF_FROM_WORD(0xE30000),
    FRF_FROM_WORD(0xE31000),
    FRF_FROM_WORD(0xE32000),
    FRF_FROM_WORD(0xE33000),
    FRF_FROM_WORD(0xE34000),
    FRF_FROM_WORD(0xE35000),
    FRF_FROM_WORD(0xE36000),
    FRF_FROM_WORD(0xE37000),
    FRF_FROM_WORD(0xE38000),
    FRF_FROM_WORD(0xE39000),
    FRF_FROM_WORD(0xE3A000),
    FRF_FROM_WORD(0xE3B000),
    FRF_FROM_WORD(0xE3C000),
    FRF_FROM_WORD(0xE3D000),
    FRF_FROM_WORD(0xE3E000),
    FRF_FROM_WORD(0xE3F000),
    FRF_FROM_WORD(0xE40000),
    FRF_FROM_WORD(0xE41000),
    FRF_FROM_WORD(0xE42000),
    FRF_FROM_WORD(0xE43000),
    FRF_FROM_WORD(0xE44000),
    FRF_FROM_WORD(0xE45000),
    FRF_FROM_WORD(0xE46000),
    FRF_FROM_WORD(0xE47000),
    FRF_FROM_WORD(0xE48000),
    FRF_FROM_WORD(0xE49000),
    FRF_FROM_WORD(0xE4A000),
    FRF_FROM_WORD(0xE4B000),
    FRF_FROM_WORD(0xE4C000),
    FRF_FROM_WORD(0xE4D000),
    FRF_FROM_WORD(0xE4E000)
};

// -----------------------------------------------------------------------------
// EU 868 MHz hop table placeholder
// (you can later replace with real EU hop pattern if needed)
// -----------------------------------------------------------------------------
const FRF hopTable_EU_868[1] = {
    FRF_FROM_WORD(0xD90000) // example 868 MHz-ish placeholder
};

// -----------------------------------------------------------------------------
// DavisConfig methods
// -----------------------------------------------------------------------------
DavisConfig::DavisConfig(DavisRegion region) {
    loadRegion(region);
}

void DavisConfig::loadRegion(DavisRegion region) {
    if (region == DAVIS_REGION_US_915) {
        hopTable = hopTable_US_915;
        hopCount = sizeof(hopTable_US_915) / sizeof(FRF);
        // bitrate / fdev / sync / txId same as original DavisConfig.cpp
// COPY THESE FROM THE ORIGINAL TRANSMITTER DavisConfig.cpp
        bitrateMsb = 0x03;
        bitrateLsb = 0x41;

        fdevMsb = 0x01;
        fdevLsb = 0x99;

        rxBw  = 0x55;   // placeholder – use real value
        afcBw = 0x8B;   // placeholder – use real value

        syncConfig = 0x88;  // sync on, 2 bytes
        syncValue1 = 0xAA;
        syncValue2 = 0xAA;

        packetConfig1 = 0x00;  // fixed length, no whitening (adjust to match TX)
        packetConfig2 = 0x00;

        payloadLength = 10;    // Davis ISS payload length

        txId = 0x1234;         // not needed for RX, keep for completeness

    } 
    
    else {
        // ---------------- EU 868 MHz ----------------
        hopTable = hopTable_EU_868;
        hopCount = sizeof(hopTable_EU_868) / sizeof(FRF);

        // COPY EU VALUES FROM ORIGINAL DavisConfig.cpp
        bitrateMsb = 0x03;
        bitrateLsb = 0x41;

        fdevMsb = 0x01;
        fdevLsb = 0x99;

        rxBw  = 0x55;   // placeholder – use real value
        afcBw = 0x8B;   // placeholder – use real value

        syncConfig = 0x88;
        syncValue1 = 0xAA;
        syncValue2 = 0xAA;

        packetConfig1 = 0x00;
        packetConfig2 = 0x00;

        payloadLength = 10;

        txId = 0x1234;
    }
}
