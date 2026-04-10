#include <Arduino.h>
#include "DavisRF69.h"
#include "DavisRFM69registers.h"
#include "DavisConfig.h"   // for DAVIS_FREQ_TABLE[]

DavisRF69 radio(10, 2);    // CS = 10, DIO0 = 2

const uint8_t LED_PIN   = 14;
const uint8_t RESET_PIN = 3;

volatile bool packetReady = false;

uint8_t  hopIndex        = 0;
uint32_t lastGoodMillis  = 0;
const uint32_t HOP_TIMEOUT_MS = 2800;   // Vue ~2.56s, give a little margin

// -----------------------------------------------------------------------------
// Simple CRC-CCITT (0x1021, init 0xFFFF) used by Davis
// -----------------------------------------------------------------------------
uint16_t davis_crc(const uint8_t* data, uint8_t len) {
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// -----------------------------------------------------------------------------
// Interrupt: DIO0 -> PayloadReady
// -----------------------------------------------------------------------------
void dio0ISR() {
    packetReady = true;
}

// -----------------------------------------------------------------------------
// Set RF frequency from DAVIS_FREQ_TABLE
// -----------------------------------------------------------------------------
void setChannelFromTable(uint8_t idx) {
    if (idx >= 51) idx = 0;
    uint32_t frf = DAVIS_FREQ_TABLE[idx];
Serial.println(">>> setChannelFromTable() CALLED <<<");

    radio.writeReg(REG_FRFMSB, (frf >> 16) & 0xFF);
    radio.writeReg(REG_FRFMID, (frf >> 8)  & 0xFF);
    radio.writeReg(REG_FRFLSB,  frf        & 0xFF);
}

// -----------------------------------------------------------------------------
// Dump key RF69 registers for sanity checking
// -----------------------------------------------------------------------------
void dumpRegisters() {
    Serial.println(F("=== RFM69 Register Dump ==="));
    for (uint8_t reg = 0x01; reg <= 0x4F; reg++) {
        uint8_t val = radio.readReg(reg);
        if (val < 16) Serial.print('0');
        Serial.print(val, HEX);
        Serial.print(' ');
        if ((reg & 0x0F) == 0x0F) Serial.println();
    }
    Serial.println();
}

// -----------------------------------------------------------------------------
// Optional: hard reset pulse to RF69 (RESET_PIN -> RF69 RESET)
// -----------------------------------------------------------------------------
void hardResetRF69() {
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    delay(1);
    digitalWrite(RESET_PIN, HIGH);
    delay(5);
    digitalWrite(RESET_PIN, LOW);
    delay(10);
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.begin(115200);
    while (!Serial) { }

    Serial.println();
    Serial.println(F("Davis Vantage Vue ISS Sniffer - Phase 1C (Auto-hop + Park)"));
    Serial.println(F("CS=10, DIO0=2, RESET=3, LED=14"));

    hardResetRF69();

    // Basic radio init (DavisRF69 should configure OOK + Davis settings)
    radio.initialize();
//    radio.setMode(RF_OPMODE_RECEIVER);
//    radio.writeReg(0x01, 0x10);   // RX mode
    radio.writeReg(0x11, 0x00);   // REG_PALEVEL = 0x11, disable all PA stages


    // Start on channel 0
    hopIndex = 0;
//   setChannelFromTable(hopIndex);


  //  radio.writeReg(0x01, 0x10);   // Force RX mode

    // Attach interrupt on DIO0
    attachInterrupt(digitalPinToInterrupt(2), dio0ISR, RISING);
  

    lastGoodMillis = millis();
    Serial.println(F("Forced RX Mode with 0x14"));
    Serial.println(F("Sniffer ready. Starting on channel 0 and waiting for packets..."));
    Serial.println(F("Type 'd' in Serial Monitor to dump registers."));
    // Force Enter RX mode
    radio.writeReg(0x01, 0x14);
    Serial.print("Forced Reg  =0x");
    Serial.println(radio.readReg(0x01), HEX);

}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------
void loop() {
static uint32_t lastPrint = 0;
if (millis() - lastPrint > 500) {
    lastPrint = millis();

    // Read OpMode (0x01)
    uint8_t opmode = radio.readReg(0x01);

    // Read raw RSSI (0x24)
    uint8_t raw = radio.readReg(0x24);
    int rssi = -((int)raw) / 2;

    Serial.print("OpMode=0x");
    Serial.print(opmode, HEX);
    Serial.print("  RSSI=");
    Serial.println(rssi);
}


    // Simple serial command: 'd' = dump registers
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'd' || c == 'D') {
            dumpRegisters();
        }
    }
/*
    // If we haven't seen a good packet in a while, park on channel 0
    uint32_t now = millis();
    if ((now - lastGoodMillis) > HOP_TIMEOUT_MS) {
        if (hopIndex != 0) {
            Serial.println(F("[TIMEOUT] Lost sync, parking on channel 0"));
            hopIndex = 0;
            setChannelFromTable(hopIndex);
            //radio.setMode(RF_OPMODE_RECEIVER);
        }
        lastGoodMillis = now;  // avoid spamming timeout message
    }
*/
    if (!packetReady) {
        return;
    }

    packetReady = false;

    // Blink LED briefly on packet
    digitalWrite(LED_PIN, HIGH);

    // Read FIFO: first byte = length
    uint8_t len = radio.readReg(REG_FIFO);
    if (len == 0 || len > 64) {
        digitalWrite(LED_PIN, LOW);
        return;
    }

    uint8_t buf[64];
    for (uint8_t i = 0; i < len && i < sizeof(buf); i++) {
        buf[i] = radio.readReg(REG_FIFO);
    }

    int16_t rssi = radio.readReg(REG_RSSIVALUE);  // raw RSSI register

    Serial.print(F("Packet on hopIndex="));
    Serial.print(hopIndex);
    Serial.print(F("  RSSIraw="));
    Serial.print(rssi);
    Serial.print(F("  Len="));
    Serial.println(len);

    Serial.print(F("Data: "));
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 16) Serial.print('0');
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();

    // CRC check (last two bytes are CRC)
    if (len >= 3) {
        uint16_t calc = davis_crc(buf, len - 2);
        uint16_t rxCrc = ((uint16_t)buf[len - 2] << 8) | buf[len - 1];

        if (calc == rxCrc) {
            Serial.println(F("CRC OK"));
            lastGoodMillis = millis();

            // Advance hop index and tune to next channel
            hopIndex = (hopIndex + 1) % 51;
 //           setChannelFromTable(hopIndex);
           // radio.setMode(RF_OPMODE_RECEIVER);
        } else {
            Serial.print(F("CRC FAIL (calc="));
            Serial.print(calc, HEX);
            Serial.print(F(" rx="));
            Serial.print(rxCrc, HEX);
            Serial.println(')');
        }
    } else {
        Serial.println(F("Packet too short for CRC check"));
    }

    Serial.println();
    digitalWrite(LED_PIN, LOW);
}

// ==== END OF FILE Davis_ISS_Phase1C_Sniffer.ino ====
