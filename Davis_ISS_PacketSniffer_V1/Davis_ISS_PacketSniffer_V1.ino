// ============================================================================
// Davis ISS Packet Sniffer
// =============================================================================

// REV: 042726
// VERS: 1.1
// 
// Notes:
//   - Funcional
//   - Added non-blocking Status LEDs
//   - Working serialcommand processor
//   - TODO: add packet stats and Channel Filter
 
// REV: 042526
// VERS: 1.0
// RECOVERED: 4/27/26 - missing LED
// Notes:
//   - First fully working version with correct Davis CRC logic.
//   - Implements the 6‑byte CRC16‑CCITT check used by real ISS hardware.
//   - Fixed the classic “CRC FAIL on every packet” mistake caused by
//     computing CRC over 8 bytes instead of 6.
//   - Radio backend (DavisRFM69.cpp/.h) considered stable and frozen.
//   - Added non‑blocking LED subsystem (GOOD / MISSED / RESYNC).
//   - Preparing for UI command system and transmitter development.
//
// CRC Gotcha Summary:
//   The Davis ISS packet is 8 bytes total:
//       Bytes 0–5 : Payload (6 bytes)
//       Bytes 6–7 : CRC16‑CCITT (big‑endian)
//
//   The CRC is computed ONLY over the first 6 bytes.
//   The CRC must NOT be zero.
//   Correct check:
//
//       uint16_t calc = radio.crc16_ccitt(packet, 6);
//       uint16_t recv = (packet[6] << 8) | packet[7];
//       bool davisCrcOk = (calc == recv) && (calc != 0);
//
//   Computing CRC over 8 bytes will ALWAYS fail.
//   This mistake produces “CRC OK (radio) but CRC FAIL (Davis)”
//   even when packets are perfectly valid.
//
// ============================================================================
//


#include <Arduino.h>
#include <SPI.h>
#include "DavisRFM69.h" 
#include "Davisdef.h"
#include "RFM69registers.h"

#define RFM69_RST      3    // RFM69 RES pin  
#define RFM69_CS      10    // default SCK pin on teensy
#define RF69_IRQ_PIN   2    // RFM69 IO0 pin
#define RF69_IRQ_NUM   2    // Same as pin number on teensy
//#define LED           14    // builtin LED 13 tied to SCK on teensy
// LED pins 
#define LED_GOOD    14  //Green
#define LED_MISSED  15  //Yellow
#define LED_RESYNC  16  //Red

// LED pulse duration in ms
#define LED_PULSE_MS  60

//----- END TEENSY CONFIG


//#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!

//DavisRFM69 radio;
DavisRFM69 radio(10, 2, false, 2);   // example: CS=10, IRQ pin=2, interrupt #=2

//console vars
uint32_t lastRxTime = 0;     // ms
uint8_t hopCount = 0;
float PACKET_INTERVAL = 0;   // ms
byte TRANSMITTER_STATION_ID = 3;   // your ISS ID
//Command processor 
uint8_t currentHop = 0;
uint8_t stationID = 3;          // test your ISS uses - test only not used 
uint16_t packetIntervalMs = 2563; //

// LED state machine
uint32_t ledGoodUntil   = 0;
uint32_t ledMissedUntil = 0;
uint32_t ledResyncUntil = 0;
 
//LED INdicator helpers
void ledGood() {
    ledGoodUntil = millis() + LED_PULSE_MS;
}

void ledMissed() {
    ledMissedUntil = millis() + LED_PULSE_MS;
}

void ledResync() {
    ledResyncUntil = millis() + LED_PULSE_MS;
}

//Non-blocking flashes
void updateLEDs() {
    uint32_t now = millis();

    digitalWrite(LED_GOOD,   now < ledGoodUntil);
    digitalWrite(LED_MISSED, now < ledMissedUntil);
    digitalWrite(LED_RESYNC, now < ledResyncUntil);
}
char cmd = 0;
//Serial Command Processor
// Pretty much just a framework - add as needed 
void processCommand(char c) {
    switch (c) {

    case '?':
        Serial.println(F("Commands:"));
        Serial.println(F("  ?  - help"));
        Serial.println(F("  r  - force resync"));
        Serial.println(F("  h  - print hop info"));
        Serial.println(F("  s  - radio status"));
        Serial.println(F("  d  - dump registers"));
        Serial.println(F("  i  - ISS info"));
        Serial.println(F("  t  - toggle TX test mode (stub)"));
        break;

    case 'r':
        hopCount = 0;
        ledResync();
        Serial.println(F("Resync forced."));
        break;

    case 'h':
        Serial.print(F("Current hop: "));
        Serial.println(currentHop);
        Serial.print(F("Next hop: "));
        Serial.println((currentHop + 1) % 51);
        Serial.print(F("FRF: "));
        Serial.println(radio.readReg(REG_FRFMSB), HEX);
        break;

    case 's':
        Serial.println(F("Radio status:"));
        Serial.print(F("RSSI: "));
        Serial.println(radio.readRSSI());
        Serial.print(F("Mode: "));
        Serial.println(radio.readReg(REG_OPMODE), HEX);
        Serial.print(F("IRQ Flags 1: "));
        Serial.println(radio.readReg(REG_IRQFLAGS1), HEX);
        Serial.print(F("IRQ Flags 2: "));
        Serial.println(radio.readReg(REG_IRQFLAGS2), HEX);
        break;

    case 'd':
        Serial.println(F("Register dump:"));
        radio.readAllRegs();
        break;

    case 'i':
        Serial.println(F("ISS Info:"));
        Serial.print(F("Station ID: "));
        Serial.println(stationID);
        Serial.print(F("Packet interval: "));
        Serial.println(packetIntervalMs);
        break;

    case 't':
        Serial.println(F("TX test mode not implemented yet."));
        // placeholder for transmitter tests
        break;

    default:
        Serial.println(F("Unknown command. Type ? for help."));
        break;
    }
}

// ---------------------------------------------------------
// SETUP
// ---------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);
// Davis ISS Packet Sniffer start up
    Serial.println("------------------------");
    Serial.println("Davis ISS Packet Sniffer");
    Serial.println("REV: 042726,   VERS: 1.1");
    Serial.println("------------------------");

    // ---------------------------------------------------------
    // Teensy hardware bring-up
    // ---------------------------------------------------------
    pinMode(RFM69_CS, OUTPUT);
    pinMode(RF69_IRQ_PIN, INPUT);
    pinMode(RFM69_RST, OUTPUT);

    pinMode(LED_GOOD, OUTPUT);
    pinMode(LED_MISSED, OUTPUT);
    pinMode(LED_RESYNC, OUTPUT);

    digitalWrite(LED_GOOD, LOW);
    digitalWrite(LED_MISSED, LOW);
    digitalWrite(LED_RESYNC, LOW);

    digitalWrite(RFM69_CS, HIGH);
    digitalWrite(RFM69_RST, LOW);

    // Reset pulse
    digitalWrite(RFM69_RST, HIGH);
    delay(100);
    digitalWrite(RFM69_RST, LOW);
    delay(5);

    // SPI setup
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);

    // ---------------------------------------------------------
    // Initialize radio 
    // ---------------------------------------------------------
   radio.initialize(); // <-- correct method from DavisRFM69.cpp

    // ---------------------------------------------------------
    // Dump registers BEFORE entering RX
    // ---------------------------------------------------------
    // radio.readAllRegs();   // <-- use built-in helper

    // ---------------------------------------------------------
    // Start on a known hop (same as VP2_TFT)
    // ---------------------------------------------------------
    uint8_t startHop = 30;
    radio.setChannel(startHop);

    delay(5);

    // ---------------------------------------------------------
    // Dump registers AFTER init
    // ---------------------------------------------------------
   // radio.readAllRegs();

       Serial.println("LEDs: Green= CRC OK, Yellow= Missed Packet, Red= Re-sync");
       Serial.println("Enter: '?' for commands");
       Serial.println("Syncing...this may take a few mins");  
       
}

// ---------------------------------------------------------
// LOOP
// ---------------------------------------------------------

void loop() {
    static uint32_t lastRxTime = 0;       // ms timestamp of last frame
    static uint8_t hopCount = 0;          // streak of missed frames
    static uint8_t currentHop = 0;        // our own hop index tracker
    static float PACKET_INTERVAL = 0;     // ms between ISS frames
    
    if (Serial.available()) {
        cmd = Serial.read();
        processCommand(cmd);
    }

    // Compute packet interval based on ISS ID (same as VP2_TFT)
    if (TRANSMITTER_STATION_ID == 0)
        PACKET_INTERVAL = (40.0 + 1) / 16.0 * 1000.0;   // default ID=1
    else
        PACKET_INTERVAL = (40.0 + TRANSMITTER_STATION_ID) / 16.0 * 1000.0;

    // ---------------------------------------------------------
    // 1. If ANY frame is received (CRC OK or FAIL), hop immediately
    // ---------------------------------------------------------
    if (radio.receiveDone()) {

        int16_t rssi = radio.RSSI;
        volatile uint8_t* buf = radio.DATA;
        uint8_t len = DAVIS_PACKET_LEN;
 
        // CRC check
        uint16_t calc = radio.crc16_ccitt(buf, 6);
        uint16_t recv = (buf[6] << 8) | buf[7];
        bool davisCrcOk = (calc == recv) && (calc != 0);
      
        if (davisCrcOk) {
            Serial.print("Channel=");
            Serial.print(currentHop);
            Serial.print(" RSSI=");
            Serial.print(rssi);
            Serial.print(" CRC OK  ");
            ledGood(); //Green
            for (uint8_t i = 0; i < len; i++) {
                if (buf[i] < 16) Serial.print('0');
                Serial.print(buf[i], HEX);
                Serial.print(' ');
            }
            Serial.println();

            hopCount = 1;
        } else {
            Serial.print("Channel=");
            Serial.print(currentHop);
            Serial.print(" RSSI=");
            Serial.print(rssi);
            Serial.print(" CRC FAIL ");
            ledMissed();  //
            for (uint8_t i = 0; i < len; i++) {
                if (buf[i] < 16) Serial.print('0');
                Serial.print(buf[i], HEX);
                Serial.print(' ');
            }
            Serial.println();

            hopCount = 0;
        }

        // Timestamp this hop boundary
        lastRxTime = micros() / 1000UL;

        // VP2_TFT behavior: hop immediately
        currentHop = (currentHop + 1) % DAVIS_FREQ_TABLE_LENGTH;
        radio.setChannel(currentHop);
        //radio.receiveBegin();
        return;
    }

    // ---------------------------------------------------------
    // 2. Predictive hop when packets are missed (VP2_TFT logic)
    // ---------------------------------------------------------
    uint32_t now = micros() / 1000UL;

    if ((hopCount > 0) &&
        ((now - lastRxTime) > (hopCount * PACKET_INTERVAL + 200)))
    {
        hopCount++;

        if (hopCount > 12) {
            hopCount = 0;   // force resync
            ledResync();    
        }

        currentHop = (currentHop + 1) % DAVIS_FREQ_TABLE_LENGTH;
        radio.setChannel(currentHop);
        //radio.receiveBegin();
    }
    updateLEDs();
}





