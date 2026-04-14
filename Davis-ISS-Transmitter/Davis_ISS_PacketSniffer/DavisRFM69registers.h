#ifndef DAVIS_RFM69_REGISTERS_H
#define DAVIS_RFM69_REGISTERS_H
//Version 4/13/2026
// -----------------------------------------------------------------------------
// Core RFM69 register addresses
// -----------------------------------------------------------------------------
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

#define REG_PALEVEL         0x11
#define REG_PARAMP          0x12

// -----------------------------------------------------------------------------
// REG_RXBW bits (DCC, MANT, EXP)
// -----------------------------------------------------------------------------
#define REG_RXBW                0x19
#define RF_RXBW_DCCFREQ_010     0x40
#define RF_RXBW_MANT_16         0x00
#define RF_RXBW_MANT_20         0x08
#define RF_RXBW_MANT_24         0x10

#define RF_RXBW_EXP_2           0x02
#define RF_RXBW_EXP_3           0x03
#define RF_RXBW_EXP_4           0x04

#define REG_AFCBW           0x1A
#define REG_OOKPEAK         0x1B
#define REG_OOKAVG          0x1C
#define REG_OOKFIX          0x1D

#define REG_AFCFEI          0x1E
#define REG_RSSICONFIG      0x23
#define REG_RSSIVALUE       0x24

#define REG_IRQFLAGS1       0x27
#define REG_IRQFLAGS2       0x28

#define REG_SYNCCONFIG      0x2E
#define REG_SYNCVALUE1      0x2F

// -----------------------------------------------------------------------------
// REG_SYNCCONFIG bits
// -----------------------------------------------------------------------------
#define RF_SYNCCONFIG_SYNCON            0x10
#define RF_SYNCCONFIG_SYNCSIZE_1        0x00  // 1-byte sync


#define REG_PACKETCONFIG1   0x37
#define REG_PAYLOADLENGTH   0x38
#define REG_FIFOTHRESH      0x3C
#define REG_PACKETCONFIG2   0x3D
// -----------------------------------------------------------------------------
// REG_FIFOTHRESH bits
// -----------------------------------------------------------------------------
#define RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY   0x80

#define REG_VERSION         0x10

// -----------------------------------------------------------------------------
// REG_OPMODE bits
// -----------------------------------------------------------------------------
#define RF_OPMODE_SEQUENCER_ON      0x00
#define RF_OPMODE_SEQUENCER_OFF     0x80
#define RF_OPMODE_LISTEN_ON         0x40
#define RF_OPMODE_LISTEN_OFF        0x00

#define RF_OPMODE_SLEEP             0x00
#define RF_OPMODE_STANDBY           0x04
#define RF_OPMODE_SYNTHESIZER       0x08
#define RF_OPMODE_RECEIVER          0x10
#define RF_OPMODE_TRANSMITTER       0x0C

// Aliases used by TX.cpp
#define RF69_MODE_SLEEP             RF_OPMODE_SLEEP
#define RF69_MODE_STANDBY           RF_OPMODE_STANDBY
#define RF69_MODE_TX                RF_OPMODE_TRANSMITTER

// -----------------------------------------------------------------------------
// REG_DATAMODUL bits
// -----------------------------------------------------------------------------
#define RF_DATAMODUL_DATAMODE_PACKET        0x00
#define RF_DATAMODUL_DATAMODE_CONTINUOUS    0x40
#define RF_DATAMODUL_DATAMODE_CONTINUOUSNOBSYNC 0x60

#define RF_DATAMODUL_MODULATIONTYPE_FSK     0x00
#define RF_DATAMODUL_MODULATIONTYPE_OOK     0x08

#define RF_DATAMODUL_MODULATIONSHAPING_00   0x00
#define RF_DATAMODUL_MODULATIONSHAPING_01   0x01
#define RF_DATAMODUL_MODULATIONSHAPING_10   0x02
#define RF_DATAMODUL_MODULATIONSHAPING_11   0x03

// -----------------------------------------------------------------------------
// REG_PACKETCONFIG1 bits
// -----------------------------------------------------------------------------
#define RF_PACKET1_FORMAT_FIXED             0x00
#define RF_PACKET1_FORMAT_VARIABLE          0x80

#define RF_PACKET1_DCFREE_OFF               0x00
#define RF_PACKET1_DCFREE_MANCHESTER        0x20
#define RF_PACKET1_DCFREE_WHITENING         0x40

#define RF_PACKET1_CRC_ON                   0x10
#define RF_PACKET1_CRC_OFF                  0x00

#define RF_PACKET1_CRCAUTOCLEAR_ON          0x00
#define RF_PACKET1_CRCAUTOCLEAR_OFF         0x08

#define RF_PACKET1_ADRSFILTERING_OFF        0x00
#define RF_PACKET1_ADRSFILTERING_NODE       0x02
#define RF_PACKET1_ADRSFILTERING_NODEBROADCAST 0x04

// -----------------------------------------------------------------------------
// REG_PALEVEL bits
// -----------------------------------------------------------------------------
#define RF_PALEVEL_PA0_ON                   0x80
#define RF_PALEVEL_PA1_ON                   0x40
#define RF_PALEVEL_PA2_ON                   0x20

// -----------------------------------------------------------------------------
// REG_PARAMP bits
// -----------------------------------------------------------------------------
#define RF_PARAMP_40                        0x09  // 40 µs ramp

// -----------------------------------------------------------------------------
// REG_IRQFLAGS2 bits
// -----------------------------------------------------------------------------
#define RF_IRQFLAGS2_FIFONOTEMPTY           0x40
#define RF_IRQFLAGS2_FIFOOVERRUN            0x10
#define RF_IRQFLAGS2_PACKETSENT             0x08
#define RF_IRQFLAGS2_PAYLOADREADY           0x04

#endif
