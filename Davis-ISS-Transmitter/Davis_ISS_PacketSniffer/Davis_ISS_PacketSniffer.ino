#include <Arduino.h>
#include <SPI.h>
#include "DavisRF69.h"
#include "DavisRFM69registers.h"

// -----------------------------------------------------------------------------
// Pin configuration (adjust CS/IRQ if needed)
// -----------------------------------------------------------------------------
#define RF69_CS_PIN     10    // Chip Select (Teensy)
#define RF69_IRQ_PIN    2     // DIO0 / IRQ (Teensy)
#define RF69_RESET_PIN  3     // RESET (Teensy, per your wiring)

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------
DavisRF69 radio;

static uint8_t packetBuf[64];

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
void printHex(const uint8_t* data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        if (data[i] < 0x10) Serial.print('0');
        Serial.print(data[i], HEX);
        Serial.print(' ');
    }
}

void dumpPacket(const uint8_t* data, uint8_t len)
{
    Serial.print("LEN=");
    Serial.print(len);
    Serial.print("  DATA: ");
    printHex(data, len);
    Serial.println();
}
// -----------------------------------------------------------------------------
// Force RFM69 frequency to 902.3 MHz using raw SPI writes
// This bypasses the sniffer driver's incomplete register map.
// -----------------------------------------------------------------------------
void forceFRF()
{
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

    digitalWrite(RF69_CS_PIN, LOW);
    SPI.transfer(0x07 | 0x80);
    SPI.transfer(0x39);
    digitalWrite(RF69_CS_PIN, HIGH);

    digitalWrite(RF69_CS_PIN, LOW);
    SPI.transfer(0x08 | 0x80);
    SPI.transfer(0xC6);

    pinMode(RF69_RESET_PIN , OUTPUT);
    digitalWrite(RF69_RESET_PIN , HIGH);
    delay(100);
    digitalWrite(RF69_RESET_PIN , LOW);
    delay(100);


    SPI.endTransaction();

    Serial.println("Forced FRF via raw SPI");
}


// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    while (!Serial) { /* wait for USB serial */ }

    Serial.println();
    Serial.println(F("Davis ISS Packet Sniffer - starting up"));

    if (!radio.initialize(RF69_CS_PIN, RF69_IRQ_PIN, RF69_RESET_PIN))
    {
        Serial.println(F("Radio init FAILED - check wiring and power"));
        while (1) { delay(1000); }
    }
  Serial.println(F("Radio init OK - listening for Davis packets..."));

    // >>> CALL RAW SPI OVERRIDE LAST <<<
    forceFRF();
    
    Serial.print("FRFMSB: 0x");
Serial.println(radio.readReg(REG_FRFMSB), HEX);

Serial.print("FRFMID: 0x");
Serial.println(radio.readReg(REG_FRFMID), HEX);

Serial.print("FRFLSB: 0x");
Serial.println(radio.readReg(REG_FRFLSB), HEX);

Serial.print("OPMODE: 0x");
Serial.println(radio.readReg(REG_OPMODE), HEX);

Serial.print("DATAMODUL: 0x");
Serial.println(radio.readReg(REG_DATAMODUL), HEX);

Serial.print("IRQFLAGS1: 0x");
Serial.println(radio.readReg(REG_IRQFLAGS1), HEX);

Serial.print("IRQFLAGS2: 0x");
Serial.println(radio.readReg(REG_IRQFLAGS2), HEX);

Serial.print("RSSIVALUE: 0x");
Serial.println(radio.readReg(REG_RSSIVALUE), HEX);

}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------
void loop()
{
uint8_t flags2 = radio.readReg(REG_IRQFLAGS2);  // 0x28
if (flags2 & 0x40) { // FIFO not empty
    uint8_t buf[64];
    uint8_t len = radio.readFifo(buf, sizeof(buf));

    Serial.print("POLLED Packet: LEN=");
    Serial.print(len);
    Serial.print("  DATA: ");
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}


    // Simple polled loop: check if a packet is available
    if (radio.packetAvailable())
    {
        uint8_t len = radio.readFifo(packetBuf, sizeof(packetBuf));

        if (len > 0)
        {
            Serial.print(F("Packet: "));
            dumpPacket(packetBuf, len);
        }
    }

    // Small delay to avoid hammering SPI/Serial
    delay(5);
}
