#include <DavisRF69.h>
#include <DavisRFM69registers.h>

DavisRF69 radio(10, 2);

void setup() {
    Serial.begin(115200);
    radio.initialize();
    radio.setMode(RF_OPMODE_RECEIVER);
}

void loop() {
    uint8_t rssi = radio.readReg(REG_RSSIVALUE);
    Serial.println(rssi);
}
