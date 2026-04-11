#ifndef DAVIS_RFM69_REGISTERS_H
#define DAVIS_RFM69_REGISTERS_H

#define RF69_MODE_SLEEP        0x00
#define RF69_MODE_STANDBY      0x04
#define RF69_MODE_FS           0x08
#define RF69_MODE_TX           0x0C
#define RF69_MODE_RX           0x10

#define RF_RSSI_START          0x01
#define RF_RSSI_DONE           0x02

#define RF_IRQFLAGS2_PAYLOADREADY 0x04

// ============================================================================
// Core RFM69 register addresses (from Semtech SX1231 datasheet)
// ============================================================================

#define REG_FIFO                0x00
#define REG_OPMODE              0x01
#define REG_DATAMODUL           0x02
#define REG_BITRATEMSB          0x03
#define REG_BITRATELSB          0x04
#define REG_FDEVMSB             0x05
#define REG_FDEVLSB             0x06
#define REG_FRFMSB              0x07
#define REG_FRFMID              0x08
#define REG_FRFLSB              0x09
#define REG_OSC1                0x0A
#define REG_AFCCTRL             0x0B
#define REG_PARAMP              0x0C
#define REG_LNA                 0x18
#define REG_RXBW                0x19
#define REG_AFCBW               0x1A
#define REG_OOKPEAK             0x1B
#define REG_OOKAVG              0x1C
#define REG_OOKFIX              0x1D
#define REG_AFCFEI              0x1E
#define REG_RSSICONFIG          0x23
#define REG_RSSIVALUE           0x24
#define REG_DIOMAPPING1         0x25
#define REG_DIOMAPPING2         0x26
#define REG_IRQFLAGS1           0x27
#define REG_IRQFLAGS2           0x28
#define REG_RSSITHRESH          0x29
#define REG_PREAMBLEMSB         0x2C
#define REG_PREAMBLELSB         0x2D
#define REG_SYNCCONFIG          0x2E
#define REG_SYNCVALUE1          0x2F
#define REG_SYNCVALUE2          0x30
#define REG_SYNCVALUE3          0x31
#define REG_SYNCVALUE4          0x32
#define REG_SYNCVALUE5          0x33
#define REG_SYNCVALUE6          0x34
#define REG_SYNCVALUE7          0x35
#define REG_SYNCVALUE8          0x36
#define REG_PACKETCONFIG1       0x37
#define REG_PAYLOADLENGTH       0x38
#define REG_NODEADRS            0x39
#define REG_BROADCASTADRS       0x3A
#define REG_FIFOTHRESH          0x3C
#define REG_PACKETCONFIG2       0x3D
#define REG_AESKEY1             0x3E
#define REG_TEMP1               0x4E
#define REG_TEMP2               0x4F
#define REG_TESTLNA             0x58
#define REG_TESTPA1             0x5A
#define REG_TESTPA2             0x5C
#define REG_TESTDAGC            0x6F
#define REG_TESTAFC             0x71
// ============================================================================
// Operating modes (REG_OPMODE)
// ============================================================================

#define RF_OPMODE_SEQUENCER_ON        0x00
#define RF_OPMODE_SEQUENCER_OFF       0x80

#define RF_OPMODE_LISTEN_ON           0x40
#define RF_OPMODE_LISTEN_OFF          0x00

#define RF_OPMODE_STANDBY             0x04
#define RF_OPMODE_SYNTHESIZER         0x08
#define RF_OPMODE_TRANSMITTER         0x10
#define RF_OPMODE_RECEIVER            0x0C

// ============================================================================
// Data modulation (FSK, packet mode, shaping 1.0)
// ============================================================================

#define RF_DATAMODUL_DATAMODE_PACKET          0x00
#define RF_DATAMODUL_MODULATIONTYPE_FSK       0x00
#define RF_DATAMODUL_MODULATIONSHAPING_1_0    0x03

// ============================================================================
// Davis ISS–specific values
// ============================================================================

// Bitrate: 19200 bps
#define RF_BITRATEMSB_19200     0x06
#define RF_BITRATELSB_19200     0x83

// Frequency deviation: 9900 Hz
#define RF_FDEVMSB_9900         0x00
#define RF_FDEVLSB_9900         0xA4

// AFC Low Beta mode
#define RF_AFCLOWBETA_OFF       0x00
#define RF_AFCLOWBETA_ON        0x20

// ============================================================================
// RXBW / AFCBW
// ============================================================================

#define RF_RXBW_DCCFREQ_010     0x40
#define RF_RXBW_MANT_20         0x08
#define RF_RXBW_EXP_4           0x04
#define RF_RXBW_EXP_3           0x03

// ============================================================================
// AFCFEI
// ============================================================================

#define RF_AFCFEI_AFCAUTOCLEAR_ON   0x20
#define RF_AFCFEI_AFCAUTO_ON        0x10

// ============================================================================
// LNA
// ============================================================================

#define RF_LNA_ZIN_50               0x08
#define RF_LNA_GAINSELECT_AUTO      0x00

// ============================================================================
// DIO Mapping
// ============================================================================

#define RF_DIOMAPPING1_DIO0_01      0x40

// ============================================================================
// IRQ Flags
// ============================================================================

#define RF_IRQFLAGS2_FIFOOVERRUN    0x10
// ============================================================================
// Sync configuration
// ============================================================================

#define RF_SYNC_ON                  0x80
#define RF_SYNC_FIFOFILL_AUTO       0x40
#define RF_SYNC_SIZE_2              0x08
#define RF_SYNC_TOL_2               0x02

// ============================================================================
// FIFO Threshold
// ============================================================================

#define RF_FIFOTHRESH_TXSTART_FIFOTHRESH 0x80

// ============================================================================
// PacketConfig2
// ============================================================================

#define RF_PACKET2_RXRESTARTDELAY_2BITS 0x10
#define RF_PACKET2_AUTORXRESTART_ON     0x02
#define RF_PACKET2_AES_OFF              0x00

// ============================================================================
// DAGC
// ============================================================================

#define RF_DAGC_IMPROVED_LOWBETA0   0x20

// ============================================================================
// ============================================================================
// PaRamp (REG_PARAMP)
// ============================================================================

#define RF_PARAMP_3400           0x00
#define RF_PARAMP_2000           0x01
#define RF_PARAMP_1000           0x02
#define RF_PARAMP_500            0x03
#define RF_PARAMP_250            0x04
#define RF_PARAMP_125            0x05
#define RF_PARAMP_100            0x06
#define RF_PARAMP_62             0x07
#define RF_PARAMP_50             0x08
#define RF_PARAMP_40             0x09
#define RF_PARAMP_31             0x0A
#define RF_PARAMP_25             0x0B
#define RF_PARAMP_20             0x0C
#define RF_PARAMP_15             0x0D
#define RF_PARAMP_12             0x0E
#define RF_PARAMP_10             0x0F

// ============================================================================
// PacketConfig1 (REG_PACKETCONFIG1)
// ============================================================================

#define RF_PACKET1_FORMAT_FIXED              0x00
#define RF_PACKET1_FORMAT_VARIABLE           0x80

#define RF_PACKET1_DCFREE_OFF                0x00
#define RF_PACKET1_DCFREE_MANCHESTER         0x20
#define RF_PACKET1_DCFREE_WHITENING          0x40

#define RF_PACKET1_CRC_OFF                   0x00
#define RF_PACKET1_CRC_ON                    0x10

#define RF_PACKET1_CRCAUTOCLEAR_OFF          0x00
#define RF_PACKET1_CRCAUTOCLEAR_ON           0x08

#define RF_PACKET1_ADRSFILTERING_OFF         0x00
#define RF_PACKET1_ADRSFILTERING_NODE        0x02
#define RF_PACKET1_ADRSFILTERING_NODEBROADCAST 0x04

// ============================================================================
// DavisRegTable — Davis ISS–specific RFM69 register configuration
// ============================================================================
// This table configures the RFM69 to behave like a Davis ISS receiver.
// It is used by DavisRF69::configureDavisRegisters().
// Terminator: {0xFF, 0x00}
// ============================================================================

// ============================================================================
// DavisRegTable — Davis ISS–specific RFM69 register configuration
// ============================================================================

static const uint8_t DavisRegTable[][2] = {

    // --- Basic mode control ---
    { REG_OPMODE,            0x04 },   // Standby

    // --- Data modulation ---
    { REG_DATAMODUL,         0x00 },   // Packet mode, FSK, no shaping

    // --- Bitrate: 19200 bps ---
    { REG_BITRATEMSB,        0x0D },
    { REG_BITRATELSB,        0x05 },

    // --- Frequency deviation: ~38.4 kHz ---
    { REG_FDEVMSB,           0x01 },
    { REG_FDEVLSB,           0x48 },

    // --- RF frequency (placeholder; hop table sets actual freq) ---
    { REG_FRFMSB,            0xE4 },   // 902.3 MHz base
    { REG_FRFMID,            0xC0 },
    { REG_FRFLSB,            0x00 },

    // --- Receiver configuration ---
    { REG_RXBW,              0x55 },   // ~100 kHz BW
    { REG_AFCBW,             0x8B },   // AFC BW ~125 kHz

    // --- AFC ---
    { REG_AFCCTRL,           0x00 },   // Standard AFC

    // --- LNA ---
    { REG_LNA,               0x88 },   // 200 ohm, gain auto

    // --- RSSI ---
    { REG_RSSITHRESH,        0xE4 },   // RSSI threshold ~ -114 dBm

    // --- Sync word ---
    { REG_SYNCCONFIG,        0x88 },   // Sync on, 2 bytes
    { REG_SYNCVALUE1,        0x2D },
    { REG_SYNCVALUE2,        0xD4 },

    // --- Packet config ---
    { REG_PACKETCONFIG1,     0x90 },   // Variable length, no CRC, no whitening
    { REG_PAYLOADLENGTH,     0x40 },   // Max 64 bytes
    { REG_FIFOTHRESH,        0x8F },   // TxStart=0, FIFO threshold=15

    // --- Auto modes (unused but safe) ---
    //{ REG_AUTOMODES,         0x00 },
    { REG_PACKETCONFIG2,     0x00 },

    // --- FIFO ---
    { REG_TESTDAGC,          0x30 },   // Improved DAGC

    // --- End of table ---
    { 0xFF,                  0x00 }
};

#endif  // DAVIS_RFM69_REGISTERS_H

