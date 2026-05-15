#include <Arduino.h>
#include "HopScheduler.h"
#include "DavisHopTable.h"

void HopScheduler::init(uint8_t txID) {
    hopIndex = 0;
    nextTxMillis = millis() + 2500;
}

void HopScheduler::tick() {
    if (millis() >= nextTxMillis) {
        nextTxMillis += 2500;
        hopIndex = (hopIndex + 1) % DAVIS_HOP_COUNT;
    }
}

bool HopScheduler::readyToTransmit() {
    return (millis() >= nextTxMillis - 5);
}

uint8_t HopScheduler::currentChannel() {
    return davisHopTable[hopIndex];
}
