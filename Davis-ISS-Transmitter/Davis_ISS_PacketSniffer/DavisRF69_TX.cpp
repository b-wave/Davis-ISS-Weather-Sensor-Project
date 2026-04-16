#include "DavisRF69_TX.h"
#include "DavisRFM69registers.h"
#include <Arduino.h>
#include <SPI.h>
//Version 4/15/2026


DavisRF69_TX::DavisRF69_TX(uint8_t csPin,
                           uint8_t irqPin,
                           uint8_t resetPin,
                           DavisConfig& config)
    : _csPin(csPin),
      _irqPin(irqPin),
      _resetPin(resetPin),
      _config(config),
      _currentChannel(0)
{}

bool DavisRF69_TX::begin() {
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, HIGH);
    delay(1);
    digitalWrite(_resetPin, LOW);
    delay(10);

    SPI.begin();

    configureCommon();
    configureTX();

    // Default to channel 0
    if (_config.hopCount > 0) {
        setChannel(0);
    }

    setMode(RF69_MODE_STANDBY);
    return true;
}

void DavisRF69_TX::sleep() {
    setMode(RF69_MODE_SLEEP);
}

void DavisRF69_TX::standby() {
    setMode(RF69_MODE_STANDBY);
}

bool DavisRF69_TX::setChannel(uint8_t channelIndex) {
    if (channelIndex >= _config.hopCount || _config.hopTable == nullptr) {
        return false;
    }
    setFrequencyFromConfig(channelIndex);
    _currentChannel = channelIndex;
    return true;
}

bool DavisRF69_TX::sendPacket(const uint8_t* data, uint8_t len) {
    if (!data || len == 0) return false;

    setMode(RF69_MODE_STANDBY);

    // Clear FIFO
    writeReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

    // Write FIFO
    digitalWrite(_csPin, LOW);
    SPI.transfer(REG_FIFO | 0x80);   // write FIFO
    SPI.transfer(len);               // length byte
    for (uint8_t i = 0; i < len; ++i) {
        SPI.transfer(data[i]);
    }
    digitalWrite(_csPin, HIGH);

    // Transmit
    setMode(RF69_MODE_TX);

    bool ok = waitForPacketSent(20); // Davis frames are short
    setMode(RF69_MODE_STANDBY);

    return ok;
}

bool DavisRF69_TX::sendISSFrame(const uint8_t* payload, uint8_t len, uint8_t channel) {
    // Placeholder — DavisRadio will eventually build the full ISS frame
    setChannel(channel);
    return sendPacket(payload, len);
}

void DavisRF69_TX::configureCommon() {
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY);

    writeReg(REG_DATAMODUL,
             RF_DATAMODUL_DATAMODE_PACKET |
             RF_DATAMODUL_MODULATIONTYPE_FSK |
             RF_DATAMODUL_MODULATIONSHAPING_00);

    writeReg(REG_BITRATEMSB, _config.bitrateMsb);
    writeReg(REG_BITRATELSB, _config.bitrateLsb);

    writeReg(REG_FDEVMSB, _config.fdevMsb);
    writeReg(REG_FDEVLSB, _config.fdevLsb);

    writeReg(REG_SYNCCONFIG,
             RF_SYNCCONFIG_SYNCON |
             RF_SYNCCONFIG_SYNCSIZE_1);

    //writeReg(REG_SYNCVALUE1, _config.syncValue);
writeReg(REG_SYNCVALUE1, _config.syncValue1);
writeReg(REG_SYNCVALUE2, _config.syncValue2);

    writeReg(REG_PACKETCONFIG1,
             RF_PACKET1_FORMAT_VARIABLE |
             RF_PACKET1_DCFREE_OFF |
             RF_PACKET1_CRC_OFF |
             RF_PACKET1_CRCAUTOCLEAR_OFF |
             RF_PACKET1_ADRSFILTERING_OFF);

    writeReg(REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | 0x0F);
}

void DavisRF69_TX::configureTX() {
    // PA settings — these match Davis power levels well
    writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | 0x1F); // Max PA0
    writeReg(REG_PARAMP, RF_PARAMP_40);              // 40 µs ramp
}

void DavisRF69_TX::setFrequencyFromConfig(uint8_t channelIndex) {
    const FRF& frf = _config.hopTable[channelIndex];
    writeReg(REG_FRFMSB, frf.msb);
    writeReg(REG_FRFMID, frf.mid);
    writeReg(REG_FRFLSB, frf.lsb);
}

bool DavisRF69_TX::waitForPacketSent(uint16_t timeoutMs) {
    uint32_t start = millis();
    while ((millis() - start) < timeoutMs) {
        uint8_t flags2 = readReg(REG_IRQFLAGS2);
        if (flags2 & RF_IRQFLAGS2_PACKETSENT) {
            return true;
        }
    }
    return false;
}

void DavisRF69_TX::setMode(uint8_t mode) {
    uint8_t opmode = readReg(REG_OPMODE) & 0xE3;
    writeReg(REG_OPMODE, opmode | mode);
}

bool DavisRF69_TX::writeReg(uint8_t addr, uint8_t value) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    digitalWrite(_csPin, HIGH);
    return true;
}

uint8_t DavisRF69_TX::readReg(uint8_t addr) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    digitalWrite(_csPin, HIGH);
    return val;
}
