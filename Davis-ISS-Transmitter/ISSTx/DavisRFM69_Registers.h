#pragma once

// ---------------------------------------------------------------------------
// Davis ISS RFM69 Register Addresses (minimal set)
// ---------------------------------------------------------------------------

#define REG_FIFO            0x00
#define REG_OPMODE          0x01
#define REG_DATAMODUL       0x02
#define REG_BITRATEMSB      0x03
#define REG_BITRATELSB      0x04
#define REG_FDEVMSB         0x05
#define REG_FDEVLSB         0x06
#define REG_FRFMSB          0x07
#define REG_FRFMID          0x08
#define REG_FRFLSB          0x09
#define REG_AFCCTRL         0x0B
#define REG_LNA             0x18
#define REG_RXBW            0x19
#define REG_AFCBW           0x1A
#define REG_DIOMAPPING1     0x25
#define REG_DIOMAPPING2     0x26
#define REG_IRQFLAGS1       0x27
#define REG_IRQFLAGS2       0x28
#define REG_RSSITHRESH      0x29
#define REG_PREAMBLELSB     0x2D
#define REG_SYNCCONFIG      0x2E
#define REG_SYNCVALUE1      0x2F
#define REG_SYNCVALUE2      0x30
#define REG_PACKETCONFIG1   0x37
#define REG_PAYLOADLENGTH   0x38
#define REG_FIFOTHRESH      0x3C
#define REG_PACKETCONFIG2   0x3D
#define REG_TESTDAGC        0x6F
#define REG_RSSICONFIG      0x23
#define REG_RSSIVALUE       0x24
#define REG_PALEVEL         0x11
#define REG_OCP             0x13

// ---------------------------------------------------------------------------
// Bit masks used by DavisRFM69.cpp
// ---------------------------------------------------------------------------

// RegOpMode
#define RF_OPMODE_SEQUENCER_ON        0x00
#define RF_OPMODE_LISTEN_OFF          0x00
#define RF_OPMODE_STANDBY             0x04
#define RF_OPMODE_TRANSMITTER         0x0C
#define RF_OPMODE_RECEIVER            0x10

// RegDataModul
#define RF_DATAMODUL_DATAMODE_PACKET          0x00
#define RF_DATAMODUL_MODULATIONTYPE_FSK       0x00
#define RF_DATAMODUL_MODULATIONSHAPING_10     0x02

// Bitrate 19200
#define RF_BITRATEMSB_19200           0x06
#define RF_BITRATELSB_19200           0x83

// Fdev 4800 (Davis ISS)
#define RF_FDEVMSB_4800               0x00
#define RF_FDEVLSB_4800               0x4E

// AFC
#define RF_AFCCTRL_LOWBETA_OFF        0x00

// LNA
#define RF_LNA_ZIN_50                 0x00
#define RF_LNA_GAINSELECT_AUTO        0x00

// RXBW
#define RF_RXBW_DCCFREQ_010           0x40
#define RF_RXBW_MANT_20               0x08
#define RF_RXBW_EXP_4                 0x04
#define RF_RXBW_EXP_3                 0x03

// IRQ Flags
#define RF_IRQFLAGS2_FIFOOVERRUN      0x10
#define RF_IRQFLAGS2_PAYLOADREADY     0x04
#define RF_IRQFLAGS1_MODEREADY        0x80

// RSSI
#define RF_RSSI_START                 0x01
#define RF_RSSI_DONE                  0x02

// Sync
#define RF_SYNC_ON                    0x80
#define RF_SYNC_FIFOFILL_AUTO         0x40
#define RF_SYNC_SIZE_2                0x08
#define RF_SYNC_TOL_2                 0x02

// Packet config
#define RF_PACKET1_FORMAT_FIXED       0x00
#define RF_PACKET1_DCFREE_OFF         0x00
#define RF_PACKET1_CRC_OFF            0x00
#define RF_PACKET1_CRCAUTOCLEAR_OFF   0x00
#define RF_PACKET1_ADRSFILTERING_OFF  0x00

#define RF_FIFOTHRESH_TXSTART_FIFOTHRESH 0x80

#define RF_PACKET2_RXRESTARTDELAY_2BITS  0x10
#define RF_PACKET2_AUTORXRESTART_ON      0x02
#define RF_PACKET2_AES_OFF               0x00

// DAGC
#define RF_DAGC_IMPROVED_LOWBETA0     0x20

// ---------------------------------------------------------------------------
// DIO Mapping (RegDioMapping1)
// ---------------------------------------------------------------------------
// DIO0 = 01 → PayloadReady (used by Davis sniffer and TX)
#define RF_DIOMAPPING1_DIO0_01        0x40


// ---------------------------------------------------------------------------
// Overcurrent Protection (RegOcp)
// ---------------------------------------------------------------------------
#define RF_OCP_OFF                    0x0F   // Disable OCP (required for RFM69HW high power)
#define RF_OCP_ON                     0x1A   // Enable OCP (default)

// ---------------------------------------------------------------------------
// FRF Table -- 51 channels for Davis ISS compatibility
// ---------------------------------------------------------------------------

static const uint8_t FRF[][3] PROGMEM = {
    {0xE1,0x80,0x00}, // 902.3
    {0xE1,0xA0,0x00}, // 902.5
    {0xE1,0xC0,0x00}, // 902.7
    {0xE1,0xE0,0x00}, // 902.9
    {0xE2,0x00,0x00}, // 903.1
    {0xE2,0x20,0x00}, // 903.3
    {0xE2,0x40,0x00}, // 903.5
    {0xE2,0x60,0x00}, // 903.7
    {0xE2,0x80,0x00}, // 903.9
    {0xE2,0xA0,0x00}, // 904.1
    {0xE2,0xC0,0x00}, // 904.3
    {0xE2,0xE0,0x00}, // 904.5
    {0xE3,0x00,0x00}, // 904.7
    {0xE3,0x20,0x00}, // 904.9
    {0xE3,0x40,0x00}, // 905.1
    {0xE3,0x60,0x00}, // 905.3
    {0xE3,0x80,0x00}, // 905.5
    {0xE3,0xA0,0x00}, // 905.7
    {0xE3,0xC0,0x00}, // 905.9
    {0xE3,0xE0,0x00}, // 906.1
    {0xE4,0x00,0x00}, // 906.3
    {0xE4,0x20,0x00}, // 906.5
    {0xE4,0x40,0x00}, // 906.7
    {0xE4,0x60,0x00}, // 906.9
    {0xE4,0x80,0x00}, // 907.1
    {0xE4,0xA0,0x00}, // 907.3
    {0xE4,0xC0,0x00}, // 907.5
    {0xE4,0xE0,0x00}, // 907.7
    {0xE5,0x00,0x00}, // 907.9
    {0xE5,0x20,0x00}, // 908.1
    {0xE5,0x40,0x00}, // 908.3
    {0xE5,0x60,0x00}, // 908.5
    {0xE5,0x80,0x00}, // 908.7
    {0xE5,0xA0,0x00}, // 908.9
    {0xE5,0xC0,0x00}, // 909.1
    {0xE5,0xE0,0x00}, // 909.3
    {0xE6,0x00,0x00}, // 909.5
    {0xE6,0x20,0x00}, // 909.7
    {0xE6,0x40,0x00}, // 909.9
    {0xE6,0x60,0x00}, // 910.1
    {0xE6,0x80,0x00}, // 910.3
    {0xE6,0xA0,0x00}, // 910.5
    {0xE6,0xC0,0x00}, // 910.7
    {0xE6,0xE0,0x00}, // 910.9
    {0xE7,0x00,0x00}, // 911.1
    {0xE7,0x20,0x00}, // 911.3
    {0xE7,0x40,0x00}, // 911.5
    {0xE7,0x60,0x00}, // 911.7
    {0xE7,0x80,0x00}, // 911.9
    {0xE7,0xA0,0x00}, // 912.1
    {0xE7,0xC0,0x00}, // 912.3
};
