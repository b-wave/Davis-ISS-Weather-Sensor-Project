#pragma once
#include <stdint.h>

class HopScheduler {
public:
    void init(uint8_t txID);

    void tick();
    bool readyToTransmit();

    uint8_t currentChannel();

private:
    uint32_t nextTxMillis = 0;
    uint8_t hopIndex = 0;
};
