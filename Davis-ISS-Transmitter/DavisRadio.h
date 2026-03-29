#pragma once

#include <Arduino.h>
#include "RFM69.h"

// Forward declarations
class DavisRadio {
public:
    void buildTestPacket(uint8_t* buf, uint8_t windSpeed, uint16_t windDir);

    DavisRadio(RFM69& radio);

    void begin();                                 // Configure radio for Davis ISS
    void sendPacket(const uint8_t* data,
                    uint8_t len,
                    uint8_t hopIndex);            // Send one Davis packet on hopIndex

private:
    RFM69& _radio;
};
