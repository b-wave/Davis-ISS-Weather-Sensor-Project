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

    // -------------------------------
    // Davis ISS modulation parameters
    // -------------------------------

    // Bitrate = 19200 bps
    _radio.writeReg(REG_BITRATEMSB, 0x03);
    _radio.writeReg(REG_BITRATELSB, 0x41);

    // Frequency deviation = 19.2 kHz
    _radio.writeReg(REG_FDEVMSB, 0x01);
    _radio.writeReg(REG_FDEVLSB, 0x99);

    // Sync word = 0xAA 0xAA (Davis preamble)
    _radio.writeReg(REG_SYNCVALUE1, 0xAA);
    _radio.writeReg(REG_SYNCVALUE2, 0xAA);

    // Sync size = 2 bytes, no tolerance
    _radio.writeReg(REG_SYNCCONFIG, 0b00010010);

    // Preamble length = 0x0C (96 bits)
    _radio.writeReg(REG_PREAMBLEMSB, 0x00);
    _radio.writeReg(REG_PREAMBLELSB, 0x0C);

    // Packet mode: fixed length, no whitening, no CRC, no address filtering
    _radio.writeReg(REG_PACKETCONFIG1,
        0b00000000);  // Fixed, no DC-free, no CRC, no addr filter

    // Payload length = 10 bytes
    _radio.writeReg(REG_PAYLOADLENGTH, 10);

    // Modulation: FSK, no shaping
    _radio.writeReg(REG_DATAMODUL, 0b00000000);

    // PA level (low power is fine for bench)
    _radio.writeReg(REG_PALEVEL, 0x70);

    // FIFO threshold
    _radio.writeReg(REG_FIFOTHRESH, 0x8F);

    // AFC + RX bandwidth (not used for TX but keeps radio sane)
    _radio.writeReg(REG_RXBW, 0b01001010);   // 125 kHz
    _radio.writeReg(REG_AFCBW, 0b10001010);  // 125 kHz

    // Ready to go
    _radio.setMode(RF69_MODE_STANDBY);
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

    TEENSY_SPI_DELAY();

    // Put radio into TX mode
    _radio.setMode(RF69_MODE_TX);

    // Send raw payload (no header)
    _radio.send(data, len, false);

    // Return to standby
    _radio.setMode(RF69_MODE_STANDBY);
}


