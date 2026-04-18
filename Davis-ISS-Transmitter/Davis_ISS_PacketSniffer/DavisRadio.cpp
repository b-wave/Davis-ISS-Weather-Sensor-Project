#include "DavisRadio.h"
//Version 4/15/2026
DavisRadio::DavisRadio(DavisConfig& config,
                       uint8_t csPin,
                       uint8_t irqPin,
                       uint8_t resetPin)
    : _config(config),
      _rx(csPin, irqPin, resetPin, config),
      _hopIndex(0)
{

    //Setup pins 
    pinMode(irqPin, INPUT);
    pinMode(csPin, OUTPUT);
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    SPI.endTransaction();

}

bool DavisRadio::begin() {
    if (!_rx.begin()) {
        return false;
    }
//Version 4/15/2026

    _hopIndex = 0;
    if (_config.hopCount > 0) {
        _rx.setChannel(_hopIndex);
    }

    return true;
}

void DavisRadio::setHop(uint8_t hopIndex) {
    if (_config.hopCount == 0) {
        _hopIndex = 0;
        return;
    }

    if (hopIndex >= _config.hopCount) {
        hopIndex = 0;
    }

    _hopIndex = hopIndex;
    _rx.setChannel(_hopIndex);
}

void DavisRadio::nextHop() {
    if (_config.hopCount == 0) {
        _hopIndex = 0;
        return;
    }

    _hopIndex++;
    if (_hopIndex >= _config.hopCount) {
        _hopIndex = 0;
    }

    _rx.setChannel(_hopIndex);
}

bool DavisRadio::receiveFrame(uint8_t* buf, uint8_t& len, int16_t& rssi) {
    uint8_t raw[64];
    uint8_t rawLen = 0;
    int16_t rawRssi = 0;

    if (!_rx.receive(raw, rawLen, rawRssi)) {
        return false;
    }

    if (rawLen == 0 || rawLen > 64) {
        return false;
    }

    len = rawLen;
    rssi = rawRssi;

    for (uint8_t i = 0; i < rawLen; i++) {
        buf[i] = raw[i];
    }

    return true;
}

bool DavisRadio::parseFrame(uint8_t* buf, uint8_t& len) {
    (void)buf;
    (void)len;
    return true;
}

int16_t DavisRadio::rawRSSI() {
    return _rx.getRSSI();
}

uint8_t DavisRadio::readReg(uint8_t addr) {
    return _rx.getReg(addr);

}

void DavisRadio::applyConfig() {
    _config.applyRX(_rx);
    _rx.enterRX();
}

void DavisRadio::enterRX() {
    _rx.enterRX();
}

void DavisRadio::beginRXOnly() {
    _rx.begin();
}
