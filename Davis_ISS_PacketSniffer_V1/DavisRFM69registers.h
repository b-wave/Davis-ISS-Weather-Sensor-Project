// DavisRFM69registers.h
// Minimal RFM69/SX1231 register and bitfield definitions
// needed by DavisRFM69_Portable.{h,cpp} for Davis ISS work.

#pragma once

// -----------------------------------------------------------------------------
// Register addresses
// -----------------------------------------------------------------------------
#define REG_FIFO          0x00
#define REG_OPMODE        0x01
#define REG_DATAMODUL     0x02
#define REG_BITRATEMSB    0x03
#define REG_BITRATELSB    0x04
#define REG_FDEVMSB       0x05
#define REG_FDEVLSB       0x06
#define REG_FRFMSB        0x07
#define REG_FRFMID        0x08
#define REG_FRFLSB        0x09
#define REG_AFCCTRL       0x0B
#define REG_LNA           0x18
#define REG_RXBW          0x19
#define REG_AFCBW         0x1A
#define REG_AFCFEI        0x1E
#define REG_FEIMSB        0x21
#define REG_FEILSB        0x22
#define REG_RSSICONFIG    0x23
#define REG_RSSIVALUE     0x24
#define REG_DIOMAPPING1   0x25
#define REG_IRQFLAGS1     0x27
#define REG_IRQFLAGS2     0x28
#define REG_RSSITHRESH    0x29
#define REG_PREAMBLELSB   0x2D
#define REG_SYNCCONFIG    0x2E
#define REG_SYNCVALUE1    0x2F
#define REG_SYNCVALUE2    0x30
#define REG_PACKETCONFIG1 0x37
#define REG_PAYLOADLENGTH 0x38
#define REG_FIFOTHRESH    0x3C
#define REG_PACKETCONFIG2 0x3D
#define REG_TEMP1         0x4E
#define REG_TEMP2         0x4F
#define REG_TESTPA1       0x5A
#define REG_TESTPA2       0x5C
#define REG_TESTDAGC      0x6F
#define REG_TESTAFC       0x71
#define REG_OSC1          0x0A
#define REG_PARAMP        0x12
#define REG_OCP           0x13
#define REG_PALEVEL       0x11

// -----------------------------------------------------------------------------
// RegOpMode
// -----------------------------------------------------------------------------
#define RF_OPMODE_SEQUENCER_ON        0x00
#define RF_OPMODE_SEQUENCER_OFF       0x80

#define RF_OPMODE_LISTEN_OFF          0x00
#define RF_OPMODE_LISTEN_ON           0x40

#define RF_OPMODE_SLEEP               0x00
#define RF_OPMODE_STANDBY             0x04
#define RF_OPMODE_SYNTHESIZER         0x08
#define RF_OPMODE_TRANSMITTER         0x0C
#define RF_OPMODE_RECEIVER            0x10

// -----------------------------------------------------------------------------
// RegDataModul
// -----------------------------------------------------------------------------
#define RF_DATAMODUL_DATAMODE_PACKET            0x00
#define RF_DATAMODUL_MODULATIONTYPE_FSK         0x00
#define RF_DATAMODUL_MODULATIONSHAPING_10       0x02

// -----------------------------------------------------------------------------
// RegBitRate (only 19.2 kbps used)
// -----------------------------------------------------------------------------
#define RF_BITRATEMSB_19200           0x06
#define RF_BITRATELSB_19200           0x83

// -----------------------------------------------------------------------------
// RegFdev (only 9.9 kHz and 10 kHz used)
// -----------------------------------------------------------------------------
#define RF_FDEVMSB_9900               0x00
#define RF_FDEVLSB_9900               0xA1

#define RF_FDEVMSB_10000              0x00
#define RF_FDEVLSB_10000              0xA4

// -----------------------------------------------------------------------------
// RegAfcCtrl
// -----------------------------------------------------------------------------
#define RF_AFCLOWBETA_OFF             0x00
#define RF_AFCLOWBETA_ON              0x20

// -----------------------------------------------------------------------------
// RegLna
// -----------------------------------------------------------------------------
#define RF_LNA_ZIN_50                 0x00
#define RF_LNA_GAINSELECT_AUTO        0x00

// -----------------------------------------------------------------------------
// RegRxBw / RegAfcBw
// -----------------------------------------------------------------------------
#define RF_RXBW_DCCFREQ_010           0x40
#define RF_RXBW_MANT_20               0x08
#define RF_RXBW_EXP_4                 0x04
#define RF_RXBW_EXP_3                 0x03
#define RF_RXBW_EXP_2                 0x02

// -----------------------------------------------------------------------------
// RegAfcFei
// -----------------------------------------------------------------------------
#define RF_AFCFEI_AFCAUTOCLEAR_ON     0x08
#define RF_AFCFEI_AFCAUTO_ON          0x04

// -----------------------------------------------------------------------------
// RegDioMapping1
// -----------------------------------------------------------------------------
#define RF_DIOMAPPING1_DIO0_00        0x00  // PacketSent
#define RF_DIOMAPPING1_DIO0_01        0x40  // PayloadReady

// -----------------------------------------------------------------------------
// RegIrqFlags1 / RegIrqFlags2
// -----------------------------------------------------------------------------
#define RF_IRQFLAGS1_MODEREADY        0x80

#define RF_IRQFLAGS2_FIFOOVERRUN      0x10
#define RF_IRQFLAGS2_PAYLOADREADY     0x04

// -----------------------------------------------------------------------------
// RegSyncConfig / RegSyncValue
// -----------------------------------------------------------------------------
#define RF_SYNC_ON                    0x80
#define RF_SYNC_OFF                   0x00
#define RF_SYNC_FIFOFILL_AUTO         0x00
#define RF_SYNC_SIZE_2                0x08
#define RF_SYNC_TOL_2                 0x02

// -----------------------------------------------------------------------------
// RegPacketConfig1
// -----------------------------------------------------------------------------
#define RF_PACKET1_FORMAT_FIXED       0x00
#define RF_PACKET1_DCFREE_OFF         0x00
#define RF_PACKET1_CRC_OFF            0x00
#define RF_PACKET1_CRCAUTOCLEAR_OFF   0x00
#define RF_PACKET1_ADRSFILTERING_OFF  0x00

// -----------------------------------------------------------------------------
// RegFifoThresh
// -----------------------------------------------------------------------------
#define RF_FIFOTHRESH_TXSTART_FIFOTHRESH 0x80

// -----------------------------------------------------------------------------
// RegPacketConfig2
// -----------------------------------------------------------------------------
#define RF_PACKET2_RXRESTARTDELAY_2BITS   0x10
#define RF_PACKET2_AUTORXRESTART_ON       0x02
#define RF_PACKET2_AES_OFF                0x00
#define RF_PACKET2_RXRESTART              0x04

// -----------------------------------------------------------------------------
// RegTestDagc
// -----------------------------------------------------------------------------
#define RF_DAGC_IMPROVED_LOWBETA0     0x30

// -----------------------------------------------------------------------------
// RegTemp1
// -----------------------------------------------------------------------------
#define RF_TEMP1_MEAS_START           0x08
#define RF_TEMP1_MEAS_RUNNING         0x04

// -----------------------------------------------------------------------------
// RegRssiConfig
// -----------------------------------------------------------------------------
#define RF_RSSI_START                 0x01
#define RF_RSSI_DONE                  0x02

// -----------------------------------------------------------------------------
// Modes used by DavisRFM69
// -----------------------------------------------------------------------------
#define RF69_MODE_SLEEP               0
#define RF69_MODE_STANDBY             1
#define RF69_MODE_SYNTH               2
#define RF69_MODE_RX                  3
#define RF69_MODE_TX                  4

// -----------------------------------------------------------------------------
// RegOcp / RegPaLevel / High power
// -----------------------------------------------------------------------------
#define RF_OCP_OFF                    0x0F
#define RF_OCP_ON                     0x1A

#define RF_PALEVEL_PA0_ON             0x80
#define RF_PALEVEL_PA0_OFF            0x00
#define RF_PALEVEL_PA1_ON             0x40
#define RF_PALEVEL_PA1_OFF            0x00
#define RF_PALEVEL_PA2_ON             0x20
#define RF_PALEVEL_PA2_OFF            0x00

// RegPaRamp
#define RF_PARAMP_25              0x0B

// RegOsc1
#define RF_OSC1_RCCAL_START       0x80
#define RF_OSC1_RCCAL_DONE        0x40

// -----------------------------------------------------------------------------
// Convenience constants used elsewhere
// -----------------------------------------------------------------------------
#define COURSE_TEMP_COEF              -90   // same as original libs
