#include <Arduino.h>
#include <SPI.h>
#include "DavisRF69.h"
#include "DavisRFM69registers.h"

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
DavisRF69::DavisRF69()
{
    _csPin = 255;
    _irqPin = 255;
    _resetPin = 255;
}

// -----------------------------------------------------------------------------
// Select / Unselect helpers
// -----------------------------------------------------------------------------
inline void DavisRF69::select()
{
    digitalWrite(_csPin, LOW);
}

inline void DavisRF69::unselect()
{
    digitalWrite(_csPin, HIGH);
}

// -----------------------------------------------------------------------------
// Reset pulse
// -----------------------------------------------------------------------------
void DavisRF69::hardwareReset()
{
    digitalWrite(_resetPin, HIGH);
    delay(1);
    digitalWrite(_resetPin, LOW);
    delay(5);
}

// -----------------------------------------------------------------------------
// SPI register helpers
// -----------------------------------------------------------------------------
uint8_t DavisRF69::readReg(uint8_t addr)
{
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    select();
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    unselect();
    SPI.endTransaction();
    return val;
}

void DavisRF69::writeReg(uint8_t addr, uint8_t value)
{
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    select();
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    unselect();
    SPI.endTransaction();
}

// -----------------------------------------------------------------------------
// Initialize radio
// -----------------------------------------------------------------------------
bool DavisRF69::initialize(uint8_t cs, uint8_t irq, uint8_t rst)
{
    _csPin = cs;
    _irqPin = irq;
    _resetPin = rst;

    // --- SPI bring-up BEFORE touching CS ---
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    pinMode(_irqPin, INPUT);
    pinMode(_resetPin, OUTPUT);

    hardwareReset();

    // --- Receiver safety: disable PA chain ---
    writeReg(0x11, 0x00);   // REG_PALEVEL = 0x00

    Serial.println("DavisRF69: SPI started, CS high, reset complete");

    // --- Verify chip is responding ---
    uint8_t version = readReg(REG_VERSION);
    Serial.print("RFM69 Version: 0x");
    Serial.println(version, HEX);

    if (version == 0x00 || version == 0xFF)
    {
        Serial.println("ERROR: RFM69 not responding");
        return false;
    }

    // -------------------------------------------------------------------------
    // Davis OOK RX configuration
    // -------------------------------------------------------------------------
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY);
    delay(5);

    // OOK mode
    writeReg(REG_DATAMODUL, RF_DATAMODUL_DATAMODE_CONTINUOUS | RF_DATAMODUL_MODULATIONTYPE_OOK);
        Serial.println("Frequency 902.3 forced!");
    // Davis frequency: 915 MHz (center)
    writeReg(REG_FRFMSB, 0xE1); // for 902.3 MHz
    writeReg(REG_FRFMID, 0x8C);
    writeReg(REG_FRFLSB, 0xCC);


    // AFC / AGC
    writeReg(REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2);

    // OOK threshold
    writeReg(REG_OOKPEAK, RF_OOKPEAK_THRESHTYPE_PEAK | RF_OOKPEAK_PEAKTHRESHDEC_000);
    writeReg(REG_OOKAVG, 0x40);
    writeReg(REG_OOKFIX, 0x06);

    // FIFO
    writeReg(REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | 0x0F);

    // IRQ
    writeReg(REG_IRQFLAGS2, 0x00);

    // Enter RX
    writeReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_RECEIVER);

    Serial.println("DavisRF69: Configured for Davis OOK RX");

    return true;
}

// -----------------------------------------------------------------------------
// Check for packet (IRQ-driven or polled)
// -----------------------------------------------------------------------------
bool DavisRF69::packetAvailable()
{
    uint8_t irq = readReg(REG_IRQFLAGS2);
    return (irq & RF_IRQFLAGS2_PAYLOADREADY);
}

// -----------------------------------------------------------------------------
// Read FIFO into buffer
// -----------------------------------------------------------------------------
uint8_t DavisRF69::readFifo(uint8_t* buf, uint8_t maxLen)
{
    uint8_t len = 0;

    while (len < maxLen)
    {
        uint8_t irq = readReg(REG_IRQFLAGS2);
        if (!(irq & RF_IRQFLAGS2_FIFONOTEMPTY))
            break;

        buf[len++] = readReg(REG_FIFO);
    }

    return len;
}
