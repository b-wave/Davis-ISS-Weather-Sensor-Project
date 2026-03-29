
#include <RFM69registers.h>
#include <RFM69.h>
#include "DavisRadio.h"
#include "DavisConfig.h"

RFM69 radio;
DavisRadio davis(radio);

uint8_t hopIndex = 0;
uint32_t lastTx = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("Davis ISS Test Transmitter Starting...");
    davis.begin();
}

void loop() {
    uint32_t now = millis();

    if (now - lastTx >= DAVIS_TX_INTERVAL_MS) {
        lastTx = now;

        uint8_t packet[10];
        davis.buildTestPacket(packet, 12, 270);   // wind 12 mph, dir 270°

        davis.sendPacket(packet, 10, hopIndex);

        Serial.print("TX hop ");
        Serial.print(hopIndex);
        Serial.println(" sent.");

        hopIndex++;
        if (hopIndex >= 51)
            hopIndex = 0;
    }
}
