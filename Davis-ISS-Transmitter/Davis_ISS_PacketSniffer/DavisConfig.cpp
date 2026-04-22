#include "DavisConfig.h"
#include "DavisRF69_RX.h"

// -----------------------------------------------------------------------------
// Base Davis US 915 MHz hop table as FRF words
// -----------------------------------------------------------------------------
const uint32_t DAVIS_FREQ_TABLE[51] = {
    0xE1C000,0xE1D000,0xE1E000,0xE1F000,0xE20000,0xE21000,0xE22000,0xE23000,
    0xE24000,0xE25000,0xE26000,0xE27000,0xE28000,0xE29000,0xE2A000,0xE2B000,
    0xE2C000,0xE2D000,0xE2E000,0xE2F000,0xE30000,0xE31000,0xE32000,0xE33000,
    0xE34000,0xE35000,0xE36000,0xE37000,0xE38000,0xE39000,0xE3A000,0xE3B000,
    0xE3C000,0xE3D000,0xE3E000,0xE3F000,0xE40000,0xE41000,0xE42000,0xE43000,
    0xE44000,0xE45000,0xE46000,0xE47000,0xE48000,0xE49000,0xE4A000,0xE4B000,
    0xE4C000,0xE4D000,0xE4E000
};

#define FRF_FROM_WORD(w) { (uint8_t)((w)>>16), (uint8_t)((w)>>8), (uint8_t)(w) }

// -----------------------------------------------------------------------------
// US 915 MHz hop table as FRF triplets
// -----------------------------------------------------------------------------
const FRF hopTable_US_915[51] = {
    FRF_FROM_WORD(0xE1C000), FRF_FROM_WORD(0xE1D000), FRF_FROM_WORD(0xE1E000),
    FRF_FROM_WORD(0xE1F000), FRF_FROM_WORD(0xE20000), FRF_FROM_WORD(0xE21000),
    FRF_FROM_WORD(0xE22000), FRF_FROM_WORD(0xE23000), FRF_FROM_WORD(0xE24000),
    FRF_FROM_WORD(0xE25000), FRF_FROM_WORD(0xE26000), FRF_FROM_WORD(0xE27000),
    FRF_FROM_WORD(0xE28000), FRF_FROM_WORD(0xE29000), FRF_FROM_WORD(0xE2A000),
    FRF_FROM_WORD(0xE2B000), FRF_FROM_WORD(0xE2C000), FRF_FROM_WORD(0xE2D000),
    FRF_FROM_WORD(0xE2E000), FRF_FROM_WORD(0xE2F000), FRF_FROM_WORD(0xE30000),
    FRF_FROM_WORD(0xE31000), FRF_FROM_WORD(0xE32000), FRF_FROM_WORD(0xE33000),
    FRF_FROM_WORD(0xE34000), FRF_FROM_WORD(0xE35000), FRF_FROM_WORD(0xE36000),
    FRF_FROM_WORD(0xE37000), FRF_FROM_WORD(0xE38000), FRF_FROM_WORD(0xE39000),
    FRF_FROM_WORD(0xE3A000), FRF_FROM_WORD(0xE3B000), FRF_FROM_WORD(0xE3C000),
    FRF_FROM_WORD(0xE3D000), FRF_FROM_WORD(0xE3E000), FRF_FROM_WORD(0xE3F000),
    FRF_FROM_WORD(0xE40000), FRF_FROM_WORD(0xE41000), FRF_FROM_WORD(0xE42000),
    FRF_FROM_WORD(0xE43000), FRF_FROM_WORD(0xE44000), FRF_FROM_WORD(0xE45000),
    FRF_FROM_WORD(0xE46000), FRF_FROM_WORD(0xE47000), FRF_FROM_WORD(0xE48000),
    FRF_FROM_WORD(0xE49000), FRF_FROM_WORD(0xE4A000), FRF_FROM_WORD(0xE4B000),
    FRF_FROM_WORD(0xE4C000), FRF_FROM_WORD(0xE4D000), FRF_FROM_WORD(0xE4E000)
};

// -----------------------------------------------------------------------------
// EU 868 placeholder
// -----------------------------------------------------------------------------
const FRF hopTable_EU_868[1] = {
    FRF_FROM_WORD(0xD90000)
};

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
DavisConfig::DavisConfig(DavisRegion region) {
    loadRegion(region);
}

// -----------------------------------------------------------------------------
// Region loader
// -----------------------------------------------------------------------------
void DavisConfig::loadRegion(DavisRegion region) {

    if (region == DAVIS_REGION_US_915) {
        hopTable = hopTable_US_915;
        hopCount = 51;

        bitrateMsb = 0x1A;
        bitrateLsb = 0x0B;

        fdevMsb = 0x00;
        fdevLsb = 0x52;

        rxBw  = 0x55;
        afcBw = 0x8B;

        packetConfig1 = 0x00;
        packetConfig2 = 0x00;
        payloadLength = 0x0A;

        txId = 0x1234;
    }
    else {
        hopTable = hopTable_EU_868;
        hopCount = 1;

        bitrateMsb = 0x03;
        bitrateLsb = 0x41;

        fdevMsb = 0x01;
        fdevLsb = 0x99;

        rxBw  = 0x55;
        afcBw = 0x8B;

        packetConfig1 = 0x00;
        packetConfig2 = 0x00;
        payloadLength = 0x0A;

        txId = 0x1234;
    }
}

// -----------------------------------------------------------------------------
// Apply RX configuration (OOK, NO SYNC, NO FSK)
// -----------------------------------------------------------------------------
void DavisConfig::applyRX(DavisRF69_RX& radio) {

    // OOK packet mode
    radio.writeReg(0x02, 0x18);

    // Bitrate
    radio.writeReg(0x03, bitrateMsb);
    radio.writeReg(0x04, bitrateLsb);

    // Deviation (ignored in OOK)
    radio.writeReg(0x05, fdevMsb);
    radio.writeReg(0x06, fdevLsb);

    // RX bandwidth
    radio.writeReg(0x19, rxBw);
    radio.writeReg(0x1A, afcBw);

    // Sync OFF
    radio.writeReg(0x2E, 0x00);

    // Packet config
    radio.writeReg(0x37, packetConfig1);
    radio.writeReg(0x38, packetConfig2);

    // Payload length
    radio.writeReg(0x39, payloadLength);

    // RSSI threshold
    radio.writeReg(0x58, 0xE4);
}
