#ifndef DAVIS_RFM69_REGISTERS_H
#define DAVIS_RFM69_REGISTERS_H

// -----------------------------------------------------------------------------
// Core RFM69 register addresses used by the Davis sniffer
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

#define REG_RXBW            0x19
#define REG_OOKPEAK         0x1B
#define REG_OOKAVG          0x1C
#define REG_OOKFIX          0x1D

#define REG_AFCFEI          0x1E
#define REG_RSSICONFIG      0x23
#define REG_RSSIVALUE       0x24

#define REG_IRQFLAGS1       0x27
#define REG_IRQFLAGS2       0x28

#define REG_FIFOTHRESH      0x3C
#define REG_PACKETCONFIG2   0x3D

#define REG_VERSION         0x10

// -----------------------------------------------------------------------------
// REG_OPMODE bits
// -----------------------------------------------------------------------------
#define RF_OPMODE_SEQUENCER_ON      0x00
#define RF_OPMODE_SEQUENCER_OFF     0x80
#define RF_OPMODE_LISTEN_OFF        0x00
#define RF_OPMODE_LISTEN_ON         0x40

#define RF_OPMODE_SLEEP             0x00
#define RF_OPMODE_STANDBY           0x04
#define RF_OPMODE_SYNTHESIZER       0x08
#define RF_OPMODE_RECEIVER          0x10
#define RF_OPMODE_TRANSMITTER       0x0C

// -----------------------------------------------------------------------------
// REG_DATAMODUL bits
// -----------------------------------------------------------------------------
#define RF_DATAMODUL_DATAMODE_PACKET        0x00
#define RF_DATAMODUL_DATAMODE_CONTINUOUS    0x40
#define RF_DATAMODUL_DATAMODE_CONTINUOUSNOBSYNC 0x60

#define RF_DATAMODUL_MODULATIONTYPE_FSK     0x00
#define RF_DATAMODUL_MODULATIONTYPE_OOK     0x08

// -----------------------------------------------------------------------------
// REG_RXBW bits (DCC, MANT, EXP)
// -----------------------------------------------------------------------------
#define RF_RXBW_DCCFREQ_010     0x40
#define RF_RXBW_MANT_16         0x00
#define RF_RXBW_MANT_20         0x08
#define RF_RXBW_MANT_24         0x10

#define RF_RXBW_EXP_2           0x02
#define RF_RXBW_EXP_3           0x03
#define RF_RXBW_EXP_4           0x04

// -----------------------------------------------------------------------------
// REG_OOKPEAK bits
// -----------------------------------------------------------------------------
#define RF_OOKPEAK_THRESHTYPE_FIXED     0x00
#define RF_OOKPEAK_THRESHTYPE_PEAK      0x40
#define RF_OOKPEAK_THRESHTYPE_AVERAGE   0x80

#define RF_OOKPEAK_PEAKTHRESHDEC_000    0x00
#define RF_OOKPEAK_PEAKTHRESHDEC_001    0x08
#define RF_OOKPEAK_PEAKTHRESHDEC_010    0x10
#define RF_OOKPEAK_PEAKTHRESHDEC_011    0x18

// -----------------------------------------------------------------------------
// REG_FIFOTHRESH bits
// -----------------------------------------------------------------------------
#define RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY   0x80
#define RF_FIFOTHRESH_TXSTART_FIFOTHRESH     0x00

// -----------------------------------------------------------------------------
// REG_IRQFLAGS2 bits
// -----------------------------------------------------------------------------
#define RF_IRQFLAGS2_FIFONOTEMPTY    0x40
#define RF_IRQFLAGS2_FIFOOVERRUN     0x10
#define RF_IRQFLAGS2_PACKETSENT      0x08
#define RF_IRQFLAGS2_PAYLOADREADY    0x04

#endif
