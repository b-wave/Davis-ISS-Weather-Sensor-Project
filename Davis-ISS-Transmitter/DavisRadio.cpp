#include <RFM69registers.h>
#include "DavisRadio.h"
#include "DavisConfig.h"
#include "TeensyFixes.h"


// -----------------------------------------------------------------------------
// CRC-CCITT (poly 0x1021, init 0xFFFF) used by Davis ISS
// -----------------------------------------------------------------------------
uint16_t davis_crc(const uint8_t* data, uint8_t len) {
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}



// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
DavisRadio::DavisRadio(RFM69& radio)
    : _radio(radio)
{
}

// -----------------------------------------------------------------------------
// Initialize RFM69 for Davis ISS protocol
// -----------------------------------------------------------------------------
void DavisRadio::begin() {
    // Basic init (node/network irrelevant for Davis)
    _radio.initialize(RF69_915MHZ, 1, 1);
    _radio.setMode(RF69_MODE_STANDBY);

    // Apply Davis-specific RF settings
    _radio.writeReg(REG_BITRATEMSB, DAVIS_BITRATE_MSB);
    _radio.writeReg(REG_BITRATELSB, DAVIS_BITRATE_LSB);

    _radio.writeReg(REG_FDEVMSB, DAVIS_FDEV_MSB);
    _radio.writeReg(REG_FDEVLSB, DAVIS_FDEV_LSB);

    _radio.writeReg(REG_SYNCVALUE1, DAVIS_SYNC1);
    _radio.writeReg(REG_SYNCVALUE2, DAVIS_SYNC2);

    // Additional Davis register tweaks can go here
}

// -----------------------------------------------------------------------------
// Build a Davis ISS packet (simple test version) w/crc
// -----------------------------------------------------------------------------

void DavisRadio::buildTestPacket(uint8_t* buf, uint8_t windSpeed, uint16_t windDir) {
    buf[0] = 0x40 | (DAVIS_TX_ID & 0x0F);  // TX ID + battery OK
    buf[1] = windSpeed;
    buf[2] = windDir >> 8;
    buf[3] = windDir & 0xFF;

    // Zero-fill remaining sensor bytes for now
    for (int i = 4; i < 8; i++)
        buf[i] = 0x00;

    // Compute CRC over first 8 bytes
    uint16_t crc = davis_crc(buf, 8);
    buf[8] = crc >> 8;
    buf[9] = crc & 0xFF;
}

// -----------------------------------------------------------------------------
// Transmit a Davis packet on a specific hop frequency
// -----------------------------------------------------------------------------
void DavisRadio::sendPacket(const uint8_t* data, uint8_t len, uint8_t hopIndex) {
    // Set hop frequency
    _radio.setFrequency(DAVIS_FREQ_TABLE[hopIndex]);

    // Teensy timing fix (if needed)
    TEENSY_SPI_DELAY();

    // Send packet using upstream RFM69 driver
    _radio.send(0, data, len, false);

}

