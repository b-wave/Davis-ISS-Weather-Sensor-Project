/*
This is a test to demonstrate sending Davis ISS Style Packets 
using an RFM69HCW and a Teensy 3.2 

Version: 0.1, 03/22/2026 s. botts

Revisions:
0.2  
0.1  Skeleton code only compiles with Teensy 03/22/2026 s. botts

*/

#include <DavisRFM69_Teensy.h>
DavisRFM69 radio;

//Defines
// 0..7, Davis transmitter ID
#define TX_ID 6  //set to a different value than all other transmitters it is  ID+1

//Constants
unsigned long lastTx;  // last time a wind data radio transmission started
byte seqIndex;         // current packet type index in txseq_vp2
byte txPacket[DAVIS_PACKET_LEN]; // radio packet data goes here
bool txDataAvailable = false;


/*
//Test Packet:
uint8_t pkt[6];
pkt[0] = 0x40 | TX_ID;   // battery flag + ID
pkt[1] = 0x00;           // dummy wind direction
pkt[2] = 0x00;           // dummy wind speed
pkt[3] = 0x00;
pkt[4] = 0x00;
pkt[5] = 0x00;
*/
// Type values for a standard Vue ISS:
static const byte txseq_vue[20] =
{
  0x80, 0x50, 0xE0, 0x20,
  0x80, 0x50, 0xE0, 0x70,
  0x80, 0x50, 0xE0, 0x30,
  0x80, 0x50, 0xE0, 0x90,
  0x80, 0x50, 0xE0, 0xA0
};
/*
NOTE: 
// "No sensor" packets for Davis protocols:
wind:           packet[1:2] (ww, dd) is 0 for all packet types
temp:           80 ww dd FF C1 00
rh:             A0 ww dd 00 01 00
rain:           E0 ww dd 80 01 00
rainrate:       50 ww dd FF 71 00
UV:             40 ww dd FF C5 00
solar:          60 ww dd FF C5 00
gust:           90 ww dd 00 05 00
unknown (VP2):  c0 ww dd 00 xx 00 - xx: 0-3 (ATK) or 5 (temp/hum), choose freely
unknown (Vue):  30 ww dd F9 40 xx - xx: BE or B9 (other values possible), choose freely
*/


void setPowerLevel(DavisRFM69 &radio, uint8_t level) {
    if (level > 31) level = 31;

    uint8_t palevel = 0x40 | 0x20 | (level & 0x1F);  // PA1 + PA2 + power

    radio.writeReg(0x11, palevel);   // REG_PALEVEL = 0x11
}

void setup() {
  // initialize radio
  radio.initialize();
  // set safe power level for RFM69HW @ 3.3V
  radio.setHighPower(false);   // safe for bench tests
  setPowerLevel(radio, 5); // 0–31, low power
  //radio.enableTx(preparePacket, TX_ID);
  seqIndex = 0;
}

void loop() {
  
  if (!txDataAvailable) return;
  txDataAvailable = false;

}

// ----- Payload assembly -----

// Fill radio packet payload containing the wind data and the transmitter ID.
// Every packet contains dummy data on other sensors in the proper transmit sequence.
// This function is called from the transmission timer ISR.
void preparePacket(byte* packet) {
  packet[0] = txseq_vue[seqIndex] | TX_ID; // station ID is set in the ISR, but we need it here for logging
  if (++seqIndex >= sizeof(txseq_vue)) seqIndex = 0;
  // store wind speed/direction in every packet
  packet[1] = 0x01;
  packet[2] = 0x02;
  // dummy data
  packet[3] = 0x03;
  packet[4] = 0x04;
  packet[5] = 0x05;
  memcpy((void*)txPacket, packet, 10);
  txDataAvailable = true;
}


