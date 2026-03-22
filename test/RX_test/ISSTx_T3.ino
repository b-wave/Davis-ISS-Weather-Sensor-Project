/*
This is a test to demonstrate sending Davis ISS Style Packets 
using an RFM69HCW and a Teensy 3.2 

Version: 0.1, 03/22/2026 s. botts

Revisions:
0.1  Test with Teensy 

*/

#include <DavisRFM69_Teensy.h>

DavisRFM69 radio;

void setDavisPowerLevel(DavisRFM69 &radio, uint8_t level) {
    if (level > 31) level = 31;

    uint8_t palevel = 0x40 | 0x20 | (level & 0x1F);  // PA1 + PA2 + power

    radio.writeReg(0x11, palevel);   // REG_PALEVEL = 0x11
}

void setup() {
  // initialize radio
  radio.initialize();
  // set safe power level for RFM69HW @ 3.3V
  radio.setHighPower(false);   // safe for bench tests
  setDavisPowerLevel(radio, 5); // 0–31, low power

}

void loop() {
  // placeholder for canned packet send
}



