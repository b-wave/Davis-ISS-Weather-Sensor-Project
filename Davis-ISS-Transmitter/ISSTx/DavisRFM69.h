#pragma once
#include <Arduino.h>
#include <SPI.h>


// NOTE FOR FUTURE STEVE:
// This driver is UNIVERSAL for Teensy 3.x and 4.x.
// It is NOT optimized for Teensy 4.x maximum SPI speed.
// If you ever need higher throughput, adjust SPISettings()
// in select()/unselect() accordingly.

#define DAVIS_PACKET_LEN 10

class DavisRFM69 {
public:
    DavisRFM69(uint8_t slaveSelectPin = SS,
               uint8_t interruptPin = 2,
               bool isRFM69HW = false);

    void initialize();
    void setChannel(uint8_t channel);
    void hop();
    bool receiveDone();
    void send(const void* buffer, uint8_t bufferSize);

    void readAllRegs();
    int16_t readRSSI(bool forceTrigger = false);
    void setPowerLevel(uint8_t level);
    void setHighPower(bool onOff = true);

    static volatile uint8_t DATA[DAVIS_PACKET_LEN];
    static volatile uint8_t CHANNEL;
    static volatile int16_t RSSI;
    static volatile bool _packetReceived;

protected:
    void interruptHandler();
    void receiveBegin();
    void sendFrame(const void* buffer, uint8_t size);
    void setMode(uint8_t mode);

    uint8_t readReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t val);

    void select();
    void unselect();

    static void isr0();

    uint8_t _slaveSelectPin;
    uint8_t _interruptPin;
    uint8_t _interruptNum;
    uint8_t _powerLevel;
    bool _isRFM69HW;
};
