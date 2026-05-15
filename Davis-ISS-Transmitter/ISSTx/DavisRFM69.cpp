#include "DavisRFM69.h"
#include "DavisRFM69_Registers.h"

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------
volatile uint8_t DavisRFM69::DATA[DAVIS_PACKET_LEN];
volatile uint8_t DavisRFM69::CHANNEL = 0;
volatile int16_t DavisRFM69::RSSI = 0;
volatile bool DavisRFM69::_packetReceived = false;

static DavisRFM69* selfPointer = nullptr;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
DavisRFM69::DavisRFM69(uint8_t slaveSelectPin, uint8_t interruptPin, bool isRFM69HW)
{
    _slaveSelectPin = slaveSelectPin;
    _interruptPin = interruptPin;
    _interruptNum = digitalPinToInterrupt(interruptPin);
    _isRFM69HW = isRFM69HW;
    _powerLevel = 31;
}

// ---------------------------------------------------------------------------
// SPI Helpers (Teensy‑safe)
// ---------------------------------------------------------------------------
void DavisRFM69::select() {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_slaveSelectPin, LOW);
}

void DavisRFM69::unselect() {
    digitalWrite(_slaveSelectPin, HIGH);
    SPI.endTransaction();
}

// ---------------------------------------------------------------------------
// Initialize radio with Davis ISS register settings
// ---------------------------------------------------------------------------
void DavisRFM69::initialize()
{
    pinMode(_slaveSelectPin, OUTPUT);
    digitalWrite(_slaveSelectPin, HIGH);

    SPI.begin();

    // Sync register test
    select();
    SPI.transfer(REG_SYNCVALUE1 | 0x80);
    SPI.transfer(0xAA);
    unselect();

    // ------------------------------------------------------------
    // CONFIG TABLE — identical to sniffer version
    // ------------------------------------------------------------
    const uint8_t CONFIG[][2] =
    {
        { REG_OPMODE,     RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
        { REG_DATAMODUL,  RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_10 },
        { REG_BITRATEMSB, RF_BITRATEMSB_19200 },
        { REG_BITRATELSB, RF_BITRATELSB_19200 },
        { REG_FDEVMSB,    RF_FDEVMSB_4800 },
        { REG_FDEVLSB,    RF_FDEVLSB_4800 },
        { REG_AFCCTRL,    RF_AFCCTRL_LOWBETA_OFF },
        { REG_LNA,        RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO },
        { REG_RXBW,       RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_4 },
        { REG_AFCBW,      RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_3 },
        { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 },
        { REG_IRQFLAGS2,  RF_IRQFLAGS2_FIFOOVERRUN },
        { REG_RSSITHRESH, 170 },
        { REG_PREAMBLELSB, 4 },
        { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_2 },
        { REG_SYNCVALUE1, 0xCB },
        { REG_SYNCVALUE2, 0x89 },
        { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
        { REG_PAYLOADLENGTH, DAVIS_PACKET_LEN },
        { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFOTHRESH | 0x09 },
        { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF },
        { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 },
        { 255, 0 }
    };

    for (uint8_t i = 0; CONFIG[i][0] != 255; i++)
        writeReg(CONFIG[i][0], CONFIG[i][1]);

    setHighPower(_isRFM69HW);
    setMode(RF_OPMODE_STANDBY);

    attachInterrupt(_interruptNum, DavisRFM69::isr0, RISING);

    selfPointer = this;
}

// ---------------------------------------------------------------------------
// Interrupt handler
// ---------------------------------------------------------------------------
void DavisRFM69::interruptHandler()
{
    RSSI = readRSSI();

    if (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
    {
        setMode(RF_OPMODE_STANDBY);

        select();
        SPI.transfer(REG_FIFO & 0x7F);
        for (uint8_t i = 0; i < DAVIS_PACKET_LEN; i++)
            DATA[i] = SPI.transfer(0);
        unselect();

        _packetReceived = true;
    }
}

void DavisRFM69::isr0() {
    if (selfPointer) selfPointer->interruptHandler();
}

// ---------------------------------------------------------------------------
// Receive
// ---------------------------------------------------------------------------
void DavisRFM69::receiveBegin()
{
    _packetReceived = false;
    setMode(RF_OPMODE_RECEIVER);
}

bool DavisRFM69::receiveDone()
{
    return _packetReceived;
}

// ---------------------------------------------------------------------------
// Send
// ---------------------------------------------------------------------------
void DavisRFM69::send(const void* buffer, uint8_t bufferSize)
{
    sendFrame(buffer, bufferSize);
}

void DavisRFM69::sendFrame(const void* buffer, uint8_t size)
{
    setMode(RF_OPMODE_STANDBY);

    select();
    SPI.transfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < size; i++)
        SPI.transfer(((uint8_t*)buffer)[i]);
    unselect();

    setMode(RF_OPMODE_TRANSMITTER);
    while (digitalRead(_interruptPin) == LOW);
    setMode(RF_OPMODE_STANDBY);
}

// ---------------------------------------------------------------------------
// Channel / Hop
// ---------------------------------------------------------------------------
void DavisRFM69::setChannel(uint8_t channel)
{
    CHANNEL = channel;

    writeReg(REG_FRFMSB, pgm_read_byte(&FRF[channel][0]));
    writeReg(REG_FRFMID, pgm_read_byte(&FRF[channel][1]));
    writeReg(REG_FRFLSB, pgm_read_byte(&FRF[channel][2]));

    receiveBegin();
}

void DavisRFM69::hop()
{
    setChannel(++CHANNEL);
}

// ---------------------------------------------------------------------------
// RSSI
// ---------------------------------------------------------------------------
int16_t DavisRFM69::readRSSI(bool forceTrigger)
{
    if (forceTrigger) {
        writeReg(REG_RSSICONFIG, RF_RSSI_START);
        while (!(readReg(REG_RSSICONFIG) & RF_RSSI_DONE));
    }
    return -(readReg(REG_RSSIVALUE) >> 1);
}

// ---------------------------------------------------------------------------
// Power
// ---------------------------------------------------------------------------
void DavisRFM69::setPowerLevel(uint8_t level)
{
    _powerLevel = level;
    writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | level);
}

void DavisRFM69::setHighPower(bool onOff)
{
    _isRFM69HW = onOff;
    writeReg(REG_OCP, onOff ? RF_OCP_OFF : RF_OCP_ON);
}

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------
void DavisRFM69::setMode(uint8_t mode)
{
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
    while (!(readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY));
}

// ---------------------------------------------------------------------------
// Register access
// ---------------------------------------------------------------------------
uint8_t DavisRFM69::readReg(uint8_t addr)
{
    select();
    SPI.transfer(addr & 0x7F);
    uint8_t val = SPI.transfer(0);
    unselect();
    return val;
}

void DavisRFM69::writeReg(uint8_t addr, uint8_t val)
{
    select();
    SPI.transfer(addr | 0x80);
    SPI.transfer(val);
    unselect();
}

void DavisRFM69::readAllRegs()
{
    for (uint8_t i = 1; i <= 0x4F; i++) {
        uint8_t v = readReg(i);
        Serial.print(i, HEX); Serial.print(": ");
        Serial.println(v, HEX);
    }
}
