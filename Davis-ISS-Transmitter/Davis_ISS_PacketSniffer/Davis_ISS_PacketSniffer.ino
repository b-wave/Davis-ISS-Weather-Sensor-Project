
#include <Arduino.h>
#include "DavisConfig.h"
#include "DavisRadio.h"
//Version 4/13/2026
// --- Pin wiring (adjust if needed) ---
static const uint8_t PIN_CS    = 10;
static const uint8_t PIN_IRQ   = 2;
static const uint8_t PIN_RESET = 3;

// --- Global RF objects ---
DavisConfig config(DAVIS_REGION_US_915);
DavisRadio  radio(config, PIN_CS, PIN_IRQ, PIN_RESET);

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("Davis ISS Packet Sniffer (Clean Build)");
    Serial.println("--------------------------------------");

    if (!radio.begin()) {
        Serial.println("Radio init FAILED");
        while (1);
    }

    Serial.println("Radio init OK");
    radio.setHop(30);   // park on channel 30

}

void loop() {
    uint8_t buf[64];
    uint8_t len = 0;
    int16_t rssi = 0;
    static uint32_t last = 0;
    if (millis() - last > 1000) {
        last = millis();
        Serial.print("RSSI: ");
        Serial.println(radio.rawRSSI());   // if your RX class has a getter
    }
    if (radio.receiveFrame(buf, len, rssi)) {
        Serial.print("PKT LEN ");
        Serial.print(len);
        Serial.print(" RSSI ");
        Serial.println(rssi);

        for (uint8_t i = 0; i < len; i++) {
            if (buf[i] < 16) Serial.print('0');
            Serial.print(buf[i], HEX);
            Serial.print(' ');
        }
        Serial.println();
    }
}



   
/*
    // Try to receive a frame on the current hop
    if (radio.receiveFrame(buf, len, rssi)) {
        Serial.print("RX hop=");
        Serial.print(radio.currentHop());
        Serial.print(" len=");
        Serial.print(len);
        Serial.print(" RSSI=");
        Serial.println(rssi);

        Serial.print("Payload: ");
        for (uint8_t i = 0; i < len; i++) {
            if (buf[i] < 16) Serial.print('0');
            Serial.print(buf[i], HEX);
            Serial.print(' ');
        }
        Serial.println();
    }

    // Move to next hop for the next listen
    radio.nextHop();
    
}
*/