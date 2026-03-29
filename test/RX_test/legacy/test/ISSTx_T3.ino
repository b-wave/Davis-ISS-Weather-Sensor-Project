/*
This is a test to demonstrate sending Davis ISS Style Packets 
using an RFM69HCW and a Teensy 3.2 

Version: 0.1, 03/22/2026 s. botts

Revisions:
0.2  
0.1  Skeleton code only compiles with Teensy 03/22/2026 s. botts

*/

#include <DavisRFM69_Teensy.h>

#define TX_ID 2   // safe, since your real ISS uses ID 1

DavisRFM69 radio;

void setDavisPowerLevel(DavisRFM69 &radio, uint8_t level) {
    if (level > 31) level = 31;
    uint8_t palevel = 0x40 | 0x20 | (level & 0x1F);
    radio.writeReg(0x11, palevel);
}

uint8_t pkt[6];

void setup() {
    Serial.begin(9600);
  delay(3000);
  Serial.println("Teensy RFM69 TX Test!");
    radio.initialize();
    radio.setHighPower(false);
    setDavisPowerLevel(radio, 5);

    // Build minimal Davis packet
    pkt[0] = 0x40 | TX_ID;  // battery flag + ID
    pkt[1] = 0x00;
    pkt[2] = 0x00;
    pkt[3] = 0x00;
    pkt[4] = 0x00;
    pkt[5] = 0x00;
}

void loop() {
    radio.hop();                 // Davis hop
    radio.send(pkt, 6);    // library adds CRC + framing
    delay(2500);           // hop interval
}


