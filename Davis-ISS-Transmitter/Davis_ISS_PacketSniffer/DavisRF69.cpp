#include "DavisRF69.h"
#include "DavisRFM69registers.h"
#include <SPI.h>


volatile bool DavisRF69::packetReceived_ = false;

DavisRF69::DavisRF69() :
    csPin_(0),
    irqPin_(0),
    resetPin_(0),
//    packetReceived_(false),
    DATALEN(0)
{
}

void DavisRF69::onDio0() {
    packetReceived_ = true;
}

void DavisRF69::initialize(uint8_t cs, uint8_t irq, uint8_t rst) {
    csPin_ = cs;
    irqPin_ = irq;
    resetPin_ = rst;

    //pinMode(csPin_, OUTPUT);
    pinMode(irqPin_, INPUT);
    pinMode(resetPin_, OUTPUT);

    //digitalWrite(csPin_, HIGH);

    // Reset pulse
    digitalWrite(resetPin_, HIGH);
    delay(5);
    digitalWrite(resetPin_, LOW);
    delay(10);
  Serial.println(F("reset")); 
    // Load Davis-specific register table
    configureDavisRegisters();
    Serial.println(F("config regs")); 
    // Attach IRQ
    attachInterrupt(digitalPinToInterrupt(irqPin_), DavisRF69::onDio0, RISING);
      Serial.println(F("irq set")); 
    // Enter RX mode
    setMode(RF69_MODE_RX);
      Serial.println(F("rx mode set")); 
}

void DavisRF69::initialize() {
    // Legacy no-arg version (optional)
}

void DavisRF69::setMode(uint8_t mode) {
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
}

int16_t DavisRF69::readRSSI(bool forceTrigger) {
    if (forceTrigger) {
        writeReg(REG_RSSICONFIG, RF_RSSI_START);
        while ((readReg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0);
    }
    return -((int16_t)readReg(REG_RSSIVALUE) / 2);
}

bool DavisRF69::receiveDone() {
    if (!packetReceived_)
        return false;

    uint8_t irq2 = readReg(REG_IRQFLAGS2);
    if (!(irq2 & RF_IRQFLAGS2_PAYLOADREADY))
        return false;

    // Read FIFO
    DATALEN = readReg(REG_FIFO);
    if (DATALEN > 66) DATALEN = 66;

    for (uint8_t i = 0; i < DATALEN; i++) {
        DATA[i] = readReg(REG_FIFO);
    }

    packetReceived_ = false;

    // Return to RX
    setMode(RF69_MODE_RX);

    return true;
}

void DavisRF69::dumpRegisters() {
    for (uint8_t addr = 0; addr <= 0x71; addr++) {
        uint8_t val = readReg(addr);
        Serial.print("0x");
        if (addr < 0x10) Serial.print("0");
        Serial.print(addr, HEX);
        Serial.print(": 0x");
        if (val < 0x10) Serial.print("0");
        Serial.println(val, HEX);
    }
}

uint8_t DavisRF69::readReg(uint8_t addr) {
    digitalWrite(csPin_, LOW);
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    digitalWrite(csPin_, HIGH);
    return val;
}

void DavisRF69::writeReg(uint8_t addr, uint8_t value) {
    digitalWrite(csPin_, LOW);
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    digitalWrite(csPin_, HIGH);
}

void DavisRF69::configureDavisRegisters() {
    // This uses your existing register table from DavisRFM69registers.h
    // (tab 2026834355). No changes needed here.
    for (uint8_t i = 0; DavisRegTable[i][0] != 0xFF; i++) {
    writeReg(DavisRegTable[i][0], DavisRegTable[i][1]);
    }

}
