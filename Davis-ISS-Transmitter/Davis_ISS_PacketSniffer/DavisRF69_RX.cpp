#include "DavisRF69_RX.h"
#include <Arduino.h>
#include <SPI.h>
//Version 4/15/2026
DavisRF69_RX::DavisRF69_RX(uint8_t csPin,
                           uint8_t irqPin,
                           uint8_t resetPin,
                           DavisConfig& config)
    : _csPin(csPin),
      _irqPin(irqPin),
      _resetPin(resetPin),
      _config(config),
      _currentChannel(0)
{
}

bool DavisRF69_RX::begin() {

    pinMode(_resetPin, OUTPUT);
    Serial.println("RX begin() running");
    Serial.print("Using CS pin: ");
    Serial.println(_csPin);

    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    Serial.print("CS after init: ");
    Serial.println(digitalRead(_csPin));

    // Hardware reset timing
    delay(10);
    delay(10);

    // --- STEP 1: Enter NORMAL Standby (Sequencer ON) ---
    writeReg(REG_OPMODE, RF_OPMODE_STANDBY);
    while (!(readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY));

    // --- STEP 2: Apply all RX configuration ---
    configureCommon();
    configureRX();

    // --- STEP 3: Disable Sequencer (still in Standby) ---
    //writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_OFF | RF_OPMODE_STANDBY);

    return true;
}


void DavisRF69_RX::sleep() {
    setMode(RF69_MODE_SLEEP);
}

void DavisRF69_RX::standby() {
    setMode(RF69_MODE_STANDBY);
}

bool DavisRF69_RX::setChannel(uint8_t channelIndex) {
    if (channelIndex >= _config.hopCount)
        return false;

    _currentChannel = channelIndex;
    setFrequencyFromConfig(channelIndex);
    return true;
}

bool DavisRF69_RX::receive(uint8_t* buffer, uint8_t& len) {
    int16_t dummyRSSI;
    return receive(buffer, len, dummyRSSI);
}

bool DavisRF69_RX::receive(uint8_t* buffer, uint8_t& len, int16_t& rssi) {
    setMode(RF69_MODE_STANDBY);
    setMode(RF_OPMODE_RECEIVER);

    if (!waitForPayload(50))
        return false;

    len = readReg(REG_FIFO);
    if (len == 0 || len > 64)
        return false;

    for (uint8_t i = 0; i < len; i++)
        buffer[i] = readReg(REG_FIFO);

    rssi = readRSSI();
    return true;
}

void DavisRF69_RX::setMode(uint8_t mode) {
    
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
}

 
bool DavisRF69_RX::writeReg(uint8_t addr, uint8_t value) {
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    return true;
}

uint8_t DavisRF69_RX::readReg(uint8_t addr) {
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    return val;
}

int16_t DavisRF69_RX::readRSSI() {
    // Start a new RSSI measurement
    writeReg(REG_RSSICONFIG, 0x01);

    // Wait for RSSI_DONE
    while (readReg(REG_RSSICONFIG) & 0x01);

    // Read raw register
    uint8_t raw = readReg(REG_RSSIVALUE);

    // Convert to dBm per datasheet
    return -(raw / 2);
}


void DavisRF69_RX::configureCommon() {
    Serial.println("configureCommon() running");
    Serial.print("CS pin state before write: ");
Serial.println(digitalRead(_csPin));

Serial.print("bitrateMsb = ");
Serial.println(_config.bitrateMsb, HEX);

Serial.print("bitrateLsb = ");
Serial.println(_config.bitrateLsb, HEX);

Serial.print("fdevMsb = ");
Serial.println(_config.fdevMsb, HEX);

Serial.print("fdevLsb = ");
Serial.println(_config.fdevLsb, HEX);

    writeReg(REG_DATAMODUL,
             RF_DATAMODUL_DATAMODE_PACKET |
             RF_DATAMODUL_MODULATIONTYPE_OOK |
             RF_DATAMODUL_MODULATIONSHAPING_00);

Serial.print("After OOK write, DataModul = ");
Serial.println(readReg(REG_DATAMODUL), HEX);

Serial.print("After OOK write, SyncConfig = ");
Serial.println(readReg(REG_SYNCCONFIG), HEX);


    writeReg(REG_BITRATEMSB, _config.bitrateMsb);
    writeReg(REG_BITRATELSB, _config.bitrateLsb);

    writeReg(REG_FDEVMSB, _config.fdevMsb);
    writeReg(REG_FDEVLSB, _config.fdevLsb);

    writeReg(REG_SYNCCONFIG, 0x00);


    //writeReg(REG_SYNCVALUE1, _config.syncValue);
//writeReg(REG_SYNCVALUE1, _config.syncValue1);
//writeReg(REG_SYNCVALUE2, _config.syncValue2);

    writeReg(REG_PACKETCONFIG1,
             RF_PACKET1_FORMAT_VARIABLE |
             RF_PACKET1_DCFREE_OFF |
             RF_PACKET1_CRC_OFF |
             RF_PACKET1_CRCAUTOCLEAR_OFF |
             RF_PACKET1_ADRSFILTERING_OFF);
}

void DavisRF69_RX::configureRX() {

    // Disable Listen Mode FIRST 
    writeReg(0x0D, 0x00);  // RegListen1
    writeReg(0x0E, 0x00);  // RegListen2
    writeReg(0x0F, 0x00);  // RegListen3
    // Disable Sequencer
    //writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_OFF | RF_OPMODE_STANDBY);

    writeReg(REG_RXBW,
             RF_RXBW_DCCFREQ_010 |
             RF_RXBW_MANT_20 |
             RF_RXBW_EXP_3);

    writeReg(REG_FIFOTHRESH,
             RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | 0x0F);
}

void DavisRF69_RX::setFrequencyFromConfig(uint8_t channelIndex) {
    const FRF& frf = _config.hopTable[channelIndex];
    writeReg(REG_FRFMSB, frf.msb);
    writeReg(REG_FRFMID, frf.mid);
    writeReg(REG_FRFLSB, frf.lsb);
}

bool DavisRF69_RX::waitForPayload(uint16_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
            return true;
    }
    return false;
}

int16_t DavisRF69_RX::getRSSI() {
    return readRSSI();
}
uint8_t DavisRF69_RX::getReg(uint8_t addr) {
    return readReg(addr);
}

void DavisRF69_RX::enterRX() {

    // Force Sequencer OFF + RX mode
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_OFF | RF_OPMODE_RECEIVER);

    // Wait for ModeReady
    while (!(readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY)) {
        // spin
    }
}


