// =============================================================================
// Davis ISS Packet Transmitter — V1.0.0
// =============================================================================
// Transmits Davis ISS Weather station compatable packets
// File:  ISSTx.ino
// REV:   051526
// VERS:  1.0.0
// License: MIT License Copyright (c) 2026 Steve, Brainwave Labs 
// ------------------------------------------------------------------------------

// CHANGELOG:
//  Revision: 051526 
//  Changes: 
//    • First complete version.  Compiles, Not tested.
//  Notes:   
//    •  Uses local files not custom libraries. 
//      - Place all the included files with this .ino in the sketch folder.  
//    •  Uses Teensy 3.2 Not optimized for Teensy 4.x but is compatible with both. 
//    •  See hardware section for modules and datasheets. 
//    •  Frequencies are currently only for US 915MHz, change as needed.
//      - Experimental version, use at your own risk. 
//      - Please check with local restrictions and frequency bands.
//    •  Compiles on Teensey 3.2! 
//       Sketch uses 13088 bytes (4%) of program storage space. Maximum is 262144 bytes.
//       Global variables use 2996 bytes (4%) of dynamic memory, leaving 62540 bytes for local
//       variables. Maximum is 65536 bytes.
// ==============================================================================

#include "DavisRFM69.h"
#include "TxPacketEngine.h"
#include "HopScheduler.h"
#include "SerialShell.h"
#include "LEDHeartbeat.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
DavisRFM69 radio;
TxPacketEngine packetEngine;
HopScheduler hop;
LEDHeartbeat led;
SerialShell shell(&radio, &packetEngine);

// Davis TX ID (1–8)
uint8_t txID = 4;

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);

    led.init();

    packetEngine.init(txID);

   // radio.initialize(RF69_915MHZ, txID, 0);
    radio.initialize();
    radio.setHighPower();  // Teensy + RFM69HW


    hop.init(txID);

    Serial.println("TX Ready");
}

// ---------------------------------------------------------------------------
// Main Loop
// ---------------------------------------------------------------------------
void loop() {
    hop.tick();

    if (hop.readyToTransmit()) {
        uint8_t pkt[8];
        packetEngine.buildNextPacket(pkt);

        radio.setChannel(hop.currentChannel());
        radio.send(pkt, 8);

        led.flash();
    }

    shell.poll();   // Non-blocking
}
