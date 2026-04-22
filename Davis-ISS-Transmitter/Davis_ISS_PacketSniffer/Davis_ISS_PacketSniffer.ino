/* Phase 1 – Clean Build (4/22/2026)
   - Uses DavisRadio high-level API
   - No direct RX driver calls
   - Clean register dump, hop loop, RSSI, packet receive
*/

#include <Arduino.h>
#include "DavisConfig.h"
#include "DavisRadio.h"

// --- Pin wiring ---
static const uint8_t PIN_CS    = 10;
static const uint8_t PIN_IRQ   = 2;
static const uint8_t PIN_RESET = 3;

// --- Global objects ---
DavisConfig config(DAVIS_REGION_US_915);
DavisRadio  radio(config, PIN_CS, PIN_IRQ, PIN_RESET);

// Dump all registers using DavisRadio wrapper
void dumpRegisters() {
    Serial.println("---- RFM69 Register Dump ----");
    for (uint8_t addr = 0x01; addr <= 0x71; addr++) {
        uint8_t val = radio.readReg(addr);
        Serial.print("0x");
        if (addr < 0x10) Serial.print('0');
        Serial.print(addr, HEX);
        Serial.print(": 0x");
        if (val < 0x10) Serial.print('0');
        Serial.println(val, HEX);
    }
    Serial.println("-----------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("Davis ISS Packet Sniffer (Phase 1)");
    Serial.println("----------------------------------");

    // Initialize radio
    radio.begin();

    // Park on hop 30 for initial testing
    radio.setHop(30);
    radio.enterRX();
    delay(5);

    // Basic register checks
    Serial.print("Version: "); Serial.println(radio.readReg(0x10), HEX);
    Serial.print("OP Mode: "); Serial.println(radio.readReg(0x01), HEX);

    Serial.print("FRF: ");
    Serial.print(radio.readReg(0x07), HEX); Serial.print(" ");
    Serial.print(radio.readReg(0x08), HEX); Serial.print(" ");
    Serial.println(radio.readReg(0x09), HEX);

    Serial.print("IRQ1: "); Serial.println(radio.readReg(0x27), HEX);
    Serial.print("IRQ2: "); Serial.println(radio.readReg(0x28), HEX);

    Serial.print("DataModul (0x02): ");
    Serial.println(radio.readReg(0x02), HEX);

    // Quick RSSI samples
    for (int i = 0; i < 5; i++) {
        Serial.print("RSSI: ");
        Serial.println(radio.rawRSSI());
        delay(50);
    }

    dumpRegisters();
}

void loop() {
    static uint32_t lastHop = 0;
    static uint8_t channel = 0;

    // Hop every 250 ms
    if (millis() - lastHop > 250) {
        lastHop = millis();
        channel = (channel + 1) % config.hopCount;
        radio.setHop(channel);
        radio.enterRX();
        Serial.print("Hop to channel ");
        Serial.println(channel);
    }

    // Read RSSI
    int16_t rssi = radio.rawRSSI();
    Serial.print("RSSI: ");
    Serial.println(rssi);

    // Try to receive a packet
    uint8_t buf[64];
    uint8_t len = 0;
    int16_t pktRSSI = 0;

    if (radio.receiveFrame(buf, len, pktRSSI)) {
        Serial.print("PACKET on channel ");
        Serial.print(channel);
        Serial.print(" len=");
        Serial.print(len);
        Serial.print(" RSSI=");
        Serial.println(pktRSSI);

        for (uint8_t i = 0; i < len; i++) {
            if (buf[i] < 16) Serial.print('0');
            Serial.print(buf[i], HEX);
            Serial.print(' ');
        }
        Serial.println();
    }
}
