/* Version 4/15/2025 PHASE1 /1A
1) Compiles
2)  Fixes Double RESET
3) Verifies RSSI
4) TODO Needs Complete RFM69/Davis Defines
5) TODO Packet Tests
 */
#include <Arduino.h>
#include "DavisConfig.h"
#include "DavisRadio.h"
//Version 4/15/2026
// --- Pin wiring (adjust if needed) ---
static const uint8_t PIN_CS    = 10;
static const uint8_t PIN_IRQ   = 2;
static const uint8_t PIN_RESET = 3;

// --- Global RF objects ---
DavisConfig config(DAVIS_REGION_US_915);
DavisRadio  radio(config, PIN_CS, PIN_IRQ, PIN_RESET);
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

Serial.println("Davis ISS Packet Sniffer (Phase 1 Build)");
Serial.println("--------------------------------------");

// Start Receive mode only 
radio.beginRXOnly();

Serial.print("Thresh after begin = 0x");
Serial.println(radio.readReg(0x58), HEX);

// Configure channel + RX parameters BEFORE entering RX
radio.setHop(30);   // park on channel 30
radio.applyConfig();

// NOW enter RX — last step
radio.enterRX();

// Optional: wait for ModeReady
delay(5);

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

for (int i = 0; i < 10; i++) {
    Serial.print("RSSI: ");
    Serial.println(radio.rawRSSI());
    delay(50);
}

dumpRegisters();

}



/*
void loop() {
    uint8_t buf[64];
    uint8_t len = 0;
    int16_t rssi = 0;
 //   static uint32_t last = 0;
 //   if (millis() - last > 1000) {
 //       last = millis();
 //       Serial.print("RSSI: ");
 //       Serial.println(radio.rawRSSI());   //
  //  }

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
*/
void loop() {
    static uint32_t last = 0;
    if (millis() - last > 200) {
        last = millis();

        int16_t rssi = radio.rawRSSI();
        uint8_t irq1 = radio.readReg(0x27);  // REG_IRQFLAGS1
        uint8_t irq2 = radio.readReg(0x28);  // REG_IRQFLAGS2
        uint8_t thresh = radio.readReg(0x58); // RegRssiThresh

        Serial.print("RSSI=");
        Serial.print(rssi);
        Serial.print("  IRQ1=0x");
        Serial.print(irq1, HEX);
        Serial.print("  IRQ2=0x");
        Serial.print(irq2, HEX);
        Serial.print("  Thresh=0x");
        Serial.println(thresh, HEX);
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