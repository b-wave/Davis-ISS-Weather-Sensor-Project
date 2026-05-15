#pragma once
#include <Arduino.h>
#include "DavisRFM69.h"
#include "TxPacketEngine.h"

class SerialShell {
public:
    SerialShell(DavisRFM69* r, TxPacketEngine* p);

    void poll();

private:
    DavisRFM69* radio;
    TxPacketEngine* engine;

    void handleCommand(char cmd);
};
