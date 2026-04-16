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
// Assert reset (active HIGH)
//digitalWrite(_resetPin, HIGH);
delay(10);   // VPTools uses 10 ms

// Release reset
//digitalWrite(_resetPin, LOW);
delay(10);   // allow oscillator + registers to settle

    
    configureCommon();
    configureRX();

    setMode(RF69_MODE_STANDBY);
    delay(100);

    setMode(RF_OPMODE_RECEIVER);
    delay(100);

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
    writeReg(REG_RSSICONFIG, 0x01);
    while (readReg(REG_RSSICONFIG) & 0x01);
    return -readReg(REG_RSSIVALUE);
}

void DavisRF69_RX::configureCommon() {
    Serial.println("configureCommon() running");
    Serial.print("CS pin state before write: ");
Serial.println(digitalRead(_csPin));

Serial.println("Writing test value to 0x03...");
writeReg(0x03, 0x12);
uint8_t v = readReg(0x03);
Serial.print("Readback 0x03 = ");
Serial.println(v, HEX);
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
}

void DavisRF69_RX::configureRX() {
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
