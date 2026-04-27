// ============================================================================
// Davis ISS Packet Sniffer
// REV: 042526
// VERS: 1.0
//
// Notes:
//   • First fully working version with correct Davis CRC logic.
//   • Implements the 6-byte CRC16-CCITT check used by real ISS hardware.
//   • Fixed the classic “CRC FAIL on every packet” mistake caused by
//     computing CRC over 8 bytes instead of 6.
//   • Radio backend (DavisRFM69.cpp/.h) considered stable and frozen.
//   • TODO: Add non-blocking LED subsystem (GOOD / MISSED / RESYNC).
//   • TODO: UI command system and transmitter development.
//
// Dev Note - CRC Gotcha Summary:
//   The Davis ISS packet is 8 bytes total:
//       Bytes 0–5 : Payload (6 bytes)
//       Bytes 6–7 : CRC16-CCITT (big-endian)
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
#include "DavisRFM69_Portable.h"

// -----------------------------------------------------------------------------
// Version + Auto‑Generated Revision
// -----------------------------------------------------------------------------
#define VERSION_TEXT   "VERS: 1.2  (Use the new libraries  )"
#define REVISION_TEXT  __DATE__ " " __TIME__

// Pin assignments
#define RFM69_RST      3    // RFM69 RES pin  
#define RFM69_CS      10    // default SCK pin on teensy
#define RF69_IRQ_PIN   2    // RFM69 IO0 pin
#define RF69_IRQ_NUM   2    // Same as pin number on teensy
//NOTE: builtin LED 13 tied to SCK on teensy
//#define GREEN_LED     14    // CRC OK
//#define YELLOW_LED    15    // Missed Packet / CRC Error
//#define RED_LED       16    // Resync 
//----- END TEENSY CONFIG

const int LED_GREEN  = A0;  // OK packet
const int LED_YELLOW = A1;  // missed packet
const int LED_RED    = A2;  // resync / error

struct LedState {
    int pin;
    bool active;
    unsigned long offTime;
};

LedState ledGreen  = { LED_GREEN,  false, 0 };
LedState ledYellow = { LED_YELLOW, false, 0 };
LedState ledRed    = { LED_RED,    false, 0 };

void setupLeds() {
    pinMode(LED_GREEN,  OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED,    OUTPUT);
    digitalWrite(LED_GREEN,  LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED,    LOW);
}

void flashLed(LedState &led, unsigned long durationMs) {
    digitalWrite(led.pin, HIGH);
    led.active  = true;
    led.offTime = millis() + durationMs;
}

void updateLeds() {
    unsigned long now = millis();

    auto update = [&](LedState &led) {
        if (led.active && (long)(now - led.offTime) >= 0) {
            digitalWrite(led.pin, LOW);
            led.active = false;
        }
    };

    update(ledGreen);
    update(ledYellow);
    update(ledRed);
}
void printBanner() {
    Serial.println();
    Serial.println(F("=============================================="));
    Serial.println(F("      Davis ISS Packet Sniffer (Teensy)       "));
    Serial.println(F("=============================================="));
    Serial.print  (F("Version:   "));
    Serial.println(VERSION_TEXT);
    Serial.print  (F("Revision:  "));
    Serial.println(REVISION_TEXT);
    Serial.print  (F("Compiled:  "));
    Serial.print  (__DATE__);
    Serial.print  (F("  "));
    Serial.println(__TIME__);
    Serial.println(F("----------------------------------------------"));
    Serial.println(F("LEDs:  GREEN=A0  YELLOW=A1  RED=A2"));
    Serial.println(F("Keys:  I/i=ChUp/Down  C/c=Filter  r=Regs  S/s=Stats"));
    Serial.println(F("----------------------------------------------"));
    Serial.println();
}


//#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!

//DavisRFM69 radio;
DavisRFM69 radio(10, 2, false, 2);   // example: CS=10, IRQ pin=2, interrupt #=2

//console vars
uint32_t lastRxTime = 0;     // ms
uint8_t hopCount = 0;
float PACKET_INTERVAL = 0;   // ms
byte TRANSMITTER_STATION_ID = 3;   // your ISS ID


// ---------------------------------------------------------
// SETUP
// ---------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);
// Davis ISS Packet Sniffer start up
    printBanner();

    // ---------------------------------------------------------
    // Teensy hardware bring-up
    // ---------------------------------------------------------
    setupLeds();

    pinMode(RFM69_CS, OUTPUT);
    pinMode(RF69_IRQ_PIN, INPUT);
    pinMode(RFM69_RST, OUTPUT);

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
    radio.initialize(FREQ_BAND_US);

    // ---------------------------------------------------------
    // Dump registers BEFORE entering RX
    // ---------------------------------------------------------
    // radio.readAllRegs();   // <-- use built-in helper

    // ---------------------------------------------------------
    // Start on a known hop (same as VP2_TFT)
    // ---------------------------------------------------------
    uint8_t startHop = 30;  //near the center of US band
    radio.setChannel(startHop);

    delay(5);
Serial.print("IRQ pin = ");
Serial.println(RF69_IRQ_PIN);

Serial.print("IRQ num = ");
Serial.println(RF69_IRQ_NUM);

    // ---------------------------------------------------------
    // Dump registers AFTER init
    // ---------------------------------------------------------
    radio.readAllRegs();
}

// ---------------------------------------------------------
// LOOP
// ---------------------------------------------------------

void loop() {
    static uint32_t lastRxTime = 0;       // ms timestamp of last frame
    static uint8_t hopCount = 0;          // streak of missed frames
    static uint8_t currentHop = 0;        // our own hop index tracker
    static float PACKET_INTERVAL = 0;     // ms between ISS frames

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
            // Print good packet info and data
            Serial.print("PACKET hop=");
            Serial.print(currentHop);
            Serial.print(" RSSI=");
            Serial.print(rssi);
            Serial.print(" CRC OK  ");
            flashLed(ledGreen, 50);

            for (uint8_t i = 0; i < len; i++) {
                if (buf[i] < 16) Serial.print('0');
                Serial.print(buf[i], HEX);
                Serial.print(' ');
            }
            Serial.println();

            hopCount = 1;
        } else {
            // Print bad packet info and data anyway
            Serial.print("PACKET hop=");
            Serial.print(currentHop);
            Serial.print(" RSSI=");
            Serial.print(rssi);
            Serial.print(" CRC FAIL ");
            flashLed(ledYellow, 150);

            for (uint8_t i = 0; i < len; i++) {
                if (buf[i] < 16) Serial.print('0');
                Serial.print(buf[i], HEX);
                Serial.print(' ');
            }
            Serial.println();
            flashLed(ledRed, 300);
            hopCount = 0;
}


        // Timestamp this hop boundary
        lastRxTime = micros() / 1000UL;

        // VP2_TFT behavior: hop immediately
        currentHop = (currentHop + 1) % FREQ_TABLE_LENGTH_US;
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
        // Missed expected packet
        flashLed(ledYellow, 150);

        if (hopCount > 12) {
            hopCount = 0;   // force resync
        }

        currentHop = (currentHop + 1) % FREQ_TABLE_LENGTH_US;
        radio.setChannel(currentHop);
        //radio.receiveBegin();
    }
    updateLeds();
}
