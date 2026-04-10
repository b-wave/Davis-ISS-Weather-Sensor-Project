#include "DavisRF69.h"

DavisRF69::DavisRF69(uint8_t csPin, uint8_t irqPin)
    : _csPin(csPin), _irqPin(irqPin)
{
}

void DavisRF69::select()   { digitalWrite(_csPin, LOW); }
void DavisRF69::unselect() { digitalWrite(_csPin, HIGH); }

uint8_t DavisRF69::readReg(uint8_t addr) {
    select();
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    unselect();
    return val;
}

void DavisRF69::writeReg(uint8_t addr, uint8_t value) {
    select();
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    unselect();
}

void DavisRF69::setMode(uint8_t mode) {
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
}

void DavisRF69::setFrequency(uint32_t frf) {
    Serial.println(">>> setFrequency() CALLED <<<");

    writeReg(REG_FRFMSB, (frf >> 16) & 0xFF);
    writeReg(REG_FRFMID, (frf >> 8) & 0xFF);
    writeReg(REG_FRFLSB, frf & 0xFF);
}

void DavisRF69::initialize() {
    Serial.println(">>> USING THIS DavisRF69.cpp <<<");
    pinMode(_csPin, OUTPUT);
    unselect();
    SPI.begin();

    // Davis-specific register table (your recovered CONFIG[] block)
    const uint8_t config[][2] = {
    //    { REG_OPMODE,       RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
        { REG_DATAMODUL,    RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_1_0 },
        { REG_BITRATEMSB,   RF_BITRATEMSB_19200 },
        { REG_BITRATELSB,   RF_BITRATELSB_19200 },
        { REG_FDEVMSB,      RF_FDEVMSB_9900 },
        { REG_FDEVLSB,      RF_FDEVLSB_9900 },
        { REG_AFCCTRL,      RF_AFCLOWBETA_OFF },
        { REG_PARAMP,       RF_PARAMP_25 },
        { REG_LNA,          RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO },
        { REG_RXBW,         RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_4 },
        { REG_AFCBW,        RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_3 },
        { REG_AFCFEI,       RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_AFCAUTO_ON },
        { REG_DIOMAPPING1,  RF_DIOMAPPING1_DIO0_01 },
        { REG_IRQFLAGS2,    RF_IRQFLAGS2_FIFOOVERRUN },
        { REG_RSSITHRESH,   190 },
        { REG_PREAMBLELSB,  0x04 },
        { REG_SYNCCONFIG,   RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_2 },
        { REG_SYNCVALUE1,   0xCB },
        { REG_SYNCVALUE2,   0x89 },
        { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
        { REG_PAYLOADLENGTH, 8 },
        { REG_FIFOTHRESH,   RF_FIFOTHRESH_TXSTART_FIFOTHRESH | (8 + 13 - 1) },
        { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF },
        { REG_TESTDAGC,     RF_DAGC_IMPROVED_LOWBETA0 },
        { REG_TESTAFC,      0 },
        { 255, 0 }
    };

    for (uint8_t i = 0; config[i][0] != 255; i++)
        writeReg(config[i][0], config[i][1]);

   // setMode(RF_OPMODE_STANDBY);
}
