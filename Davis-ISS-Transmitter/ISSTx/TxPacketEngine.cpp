#include "TxPacketEngine.h"
#include <Arduino.h>
#include "DavisCRC.h"

// ---------------------------------------------------------------------------
// Davis TX packet sequence
// (TEMP, HUM, WIND, RAIN, RAIN RATE, BATTERY)
// TODO: Verify and sync this sequence with the sniffer / real ISS behavior.
// ---------------------------------------------------------------------------
namespace {
    const uint8_t davisTxSequence[] = {
        0x80, // Temp
        0xA0, // Humidity
        0x90, // Wind
        0xE0, // Rain total
        0x50, // Rain rate
        0x20  // Battery
    };

    const uint8_t DAVIS_TX_SEQUENCE_LEN =
        sizeof(davisTxSequence) / sizeof(davisTxSequence[0]);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void TxPacketEngine::init(uint8_t id) {
    txID = id;
    seqIndex = 0;
}

// ---------------------------------------------------------------------------
// Build next packet
// Davis ISS: 8 bytes total
//   Byte 0: upper nibble = type, lower nibble = (TXID-1)
//   Bytes 1–5: payload (type-dependent)
//   Bytes 6–7: CRC16 over bytes 0–5 (Davis polynomial)
// ---------------------------------------------------------------------------
void TxPacketEngine::buildNextPacket(uint8_t* pkt) {
    // Clear packet
    for (int i = 0; i < 8; i++) {
        pkt[i] = 0;
    }

    // Byte 0: Type nibble + TX ID nibble
    uint8_t type = davisTxSequence[seqIndex];
    pkt[0] = (type & 0xF0) | ((txID - 1) & 0x0F);

    // Encode payload based on type
    switch (type) {
        case 0x80: // Temp
            encodeTemp(pkt);
            break;
        case 0xA0: // Humidity
            encodeHumidity(pkt);
            break;
        case 0x90: // Wind
            encodeWind(pkt);
            break;
        case 0xE0: // Rain total
            encodeRain(pkt);
            break;
        case 0x50: // Rain rate
            encodeRainRate(pkt);
            break;
        case 0x20: // Battery
            encodeBattery(pkt);
            break;
        default:
            // Unknown type — leave zeros
            break;
    }

    // -----------------------------------------------------------------------
    // REAL DAVIS CRC16 — computed over bytes 0–5 ONLY
    // -----------------------------------------------------------------------
    uint16_t crc = davis_crc16(pkt, 6);
    pkt[6] = (crc >> 8) & 0xFF;
    pkt[7] = crc & 0xFF;

    // Advance sequence
    seqIndex = (seqIndex + 1) % DAVIS_TX_SEQUENCE_LEN;
}

// ---------------------------------------------------------------------------
// Encoders (currently stubbed with placeholder values)
// TODO: Wire these to ISS_Drivers (BME280Driver, WindSpeedDriver, etc.)
//       using Davis units that match the sniffer.
// ---------------------------------------------------------------------------
void TxPacketEngine::encodeTemp(uint8_t* pkt) {
    int16_t t = 0;  // TODO: replace with Davis-unit temperature
    pkt[1] = (t >> 8) & 0xFF;
    pkt[2] = t & 0xFF;
}

void TxPacketEngine::encodeHumidity(uint8_t* pkt) {
    uint8_t h = 0;  // TODO: replace with humidity in Davis units
    pkt[1] = h;
}

void TxPacketEngine::encodeWind(uint8_t* pkt) {
    uint8_t speed = 0;   // TODO: wind speed in MPH (Davis encoding)
    uint16_t dir = 0;    // TODO: direction 0–359

    pkt[1] = speed;
    pkt[2] = (dir >> 8) & 0xFF;
    pkt[3] = dir & 0xFF;
}

void TxPacketEngine::encodeRain(uint8_t* pkt) {
    uint16_t count = 0;  // TODO: rain bucket count in Davis units
    pkt[1] = (count >> 8) & 0xFF;
    pkt[2] = count & 0xFF;
}

void TxPacketEngine::encodeRainRate(uint8_t* pkt) {
    uint16_t rate = 0;   // TODO: rain rate in Davis units
    pkt[1] = (rate >> 8) & 0xFF;
    pkt[2] = rate & 0xFF;
}

void TxPacketEngine::encodeBattery(uint8_t* pkt) {
    uint16_t mv = 0;     // TODO: battery millivolts -> Davis encoding
    pkt[1] = (mv >> 8) & 0xFF;
    pkt[2] = mv & 0xFF;
}

// ---------------------------------------------------------------------------
// Sensor dump (for serial shell) — stub for now
// ---------------------------------------------------------------------------
void TxPacketEngine::dumpSensors() {
    Serial.println("Sensor dump not yet wired to ISS_Drivers.");
}
