#include "DavisRF69_RX.h"
#include <Arduino.h>
#include <SPI.h>
#include "RFM69registers.h"

//Version 4/21/2026
#define DAVIS_PACKET_LEN    10 // ISS has fixed packet length of 10 bytes, including CRC and retransmit CRC
#define RF69_SPI_CS         SS // SS is the SPI slave select pin, for instance D10 on ATmega328

// Davis ISS FSK Configuration Table (Teensy Verified)
const uint8_t CONFIG[][2] =
{
  { REG_OPMODE,       RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
  { REG_DATAMODUL,    RF_DATAMODUL_DATAMODE_PACKET |
                       RF_DATAMODUL_MODULATIONTYPE_FSK |
                       RF_DATAMODUL_MODULATIONSHAPING_10 },

  { REG_BITRATEMSB,   RF_BITRATEMSB_19200 },
  { REG_BITRATELSB,   RF_BITRATELSB_19200 },

  { REG_FDEVMSB,      RF_FDEVMSB_4800 },
  { REG_FDEVLSB,      RF_FDEVLSB_4800 },

  { REG_AFCCTRL,      RF_AFCCTRL_LOWBETA_OFF },

  { REG_LNA,          RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO },

  { REG_RXBW,         RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_4 },   // 25 kHz
  { REG_AFCBW,        RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_3 },   // 50 kHz

  { REG_AFCFEI,       RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_AFCAUTO_ON },

  { REG_DIOMAPPING1,  RF_DIOMAPPING1_DIO0_01 },

  { REG_IRQFLAGS2,    RF_IRQFLAGS2_FIFOOVERRUN },

  { REG_RSSITHRESH,   0xAA },   // –85 dBm threshold

  { REG_PREAMBLELSB,  4 },      // 4 bytes of 0xAA

  { REG_SYNCCONFIG,   RF_SYNC_ON |
                       RF_SYNC_FIFOFILL_AUTO |
                       RF_SYNC_SIZE_2 |
                       RF_SYNC_TOL_2 },

  { REG_SYNCVALUE1,   0xCB },
  { REG_SYNCVALUE2,   0x89 },

  { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED |
                        RF_PACKET1_DCFREE_OFF |
                        RF_PACKET1_CRC_OFF |
                        RF_PACKET1_CRCAUTOCLEAR_OFF |
                        RF_PACKET1_ADRSFILTERING_OFF },

  { REG_PAYLOADLENGTH, 10 },   // Davis ISS = 10 bytes

  { REG_FIFOTHRESH,    RF_FIFOTHRESH_TXSTART_FIFOTHRESH | 0x09 },

  { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS |
                        RF_PACKET2_AUTORXRESTART_ON |
                        RF_PACKET2_AES_OFF },

  { REG_TESTDAGC,      RF_DAGC_IMPROVED_LOWBETA0 },
  { REG_TESTAFC,       0x00 },

  { 255, 0 }
};

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

    // --- Hard reset (single, clean) ---
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, LOW);
    delay(1);
    digitalWrite(_resetPin, HIGH);
    delay(10);
    digitalWrite(_resetPin, LOW);
    delay(10);

    // --- CS idle high ---
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    // --- SPI sanity check (same as you had) ---
    do { writeReg(REG_SYNCVALUE1, 0xAA); } while (readReg(REG_SYNCVALUE1) != 0xAA);
    do { writeReg(REG_SYNCVALUE1, 0x55); } while (readReg(REG_SYNCVALUE1) != 0x55);

    // --- Go to STANDBY with SEQUENCER ON (DeKay style) ---
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_STANDBY);
    delay(5);

    // --- Apply Davis FSK CONFIG table (including REG_OPMODE->STANDBY) ---
    for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
        writeReg(CONFIG[i][0], CONFIG[i][1]);
    }

    // --- Start on channel 0 from hop table ---
    _currentChannel = 0;
    setFrequencyFromConfig(_currentChannel);

    // --- Finally: enter RX with SEQUENCER ON ---
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_RECEIVER);
    delay(10);  // let ModeReady + RxReady settle

    return true;
}




void DavisRF69_RX::sleep() {
    //setMode(RF69_MODE_SLEEP);
}

void DavisRF69_RX::standby() {
    //setMode(RF69_MODE_STANDBY);
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
    //setMode(RF69_MODE_STANDBY);
    //setMode(RF_OPMODE_RECEIVER);

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

//void DavisRF69_RX::setMode(uint8_t mode) {
    
 //   writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
//}

 
bool DavisRF69_RX::writeReg(uint8_t addr, uint8_t value) {
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();

    delayMicroseconds(5);   
    return true;
}

uint8_t DavisRF69_RX::readReg(uint8_t addr) {
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    
    delayMicroseconds(5);   
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

    // Ensure frequency is set for current hop
    setFrequencyFromConfig(_currentChannel);

    // SEQUENCER ON + RECEIVER (critical for FSK)
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_RECEIVER);
    delay(10);
}




