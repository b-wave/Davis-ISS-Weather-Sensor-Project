#include <Arduino.h>
#include "DavisRadio.h"
#include "DavisConfig.h"

#define LED_PIN 14
#define RFM69_INT 2

#define REG_FRFMSB  0x07
#define REG_FRFMID  0x08
#define REG_FRFLSB  0x09
// North American Davis ISS hop table (50 channels)
const uint32_t hopTable[] = {
  0xE1C000, 0xE24000, 0xE30000, 0xE38000, 0xE40000,
  0xE48000, 0xE50000, 0xE58000, 0xE60000, 0xE68000,
  0xE70000, 0xE78000, 0xE80000, 0xE88000, 0xE90000,
  0xE98000, 0xEA0000, 0xEA8000, 0xEB0000, 0xEB8000,
  0xEC0000, 0xEC8000, 0xED0000, 0xED8000, 0xEE0000,
  0xEE8000, 0xEF0000, 0xEF8000, 0xF00000, 0xF08000,
  0xF10000, 0xF18000, 0xF20000, 0xF28000, 0xF30000,
  0xF38000, 0xF40000, 0xF48000, 0xF50000, 0xF58000,
  0xF60000, 0xF68000, 0xF70000, 0xF78000, 0xF80000,
  0xF88000, 0xF90000, 0xF98000, 0xFA0000, 0xFA8000
};

const uint8_t HOP_TABLE_SIZE = sizeof(hopTable) / sizeof(hopTable[0]);

volatile bool packetReady = false;



void dio0ISR() {
  packetReady = true;
}

void dumpRegisters() {
  Serial.println(F("=== RFM69 Register Dump ==="));
  for (uint8_t reg = 0x01; reg <= 0x4F; reg++) {
    uint8_t val = radio.readReg(reg);
    if (val < 16) Serial.print('0');
    Serial.print(val, HEX);
    Serial.print(' ');
    if ((reg % 16) == 0) Serial.println();
  }
  Serial.println();
}

void setup() {

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  while (!Serial) {}

  Serial.println();
  Serial.println(F("Davis ISS Packet Sniffer (Phase 1 - fixed channel, RX only)"));
  Serial.println(F("Bringing up RFM69 + DavisRadio..."));

  resetRadio();
  davis.begin();
  radio.setMode(RF69_MODE_RX);

  uint8_t channel = 0;
  uint32_t frf = DAVIS_FREQ_TABLE[channel];
Serial.print(F("FRF from table: 0x"));
Serial.println(frf, HEX);

  radio.writeReg(0x07, (frf >> 16) & 0xFF);
  radio.writeReg(0x08, (frf >> 8) & 0xFF);
  radio.writeReg(0x09, frf & 0xFF);

  dumpRegisters();

  pinMode(RFM69_INT, INPUT);
  attachInterrupt(digitalPinToInterrupt(RFM69_INT), dio0ISR, RISING);

  Serial.print(F("Using fixed channel: "));
  Serial.println(channel);
  Serial.println(F("Sniffer ready. Waiting for packets..."));
}
/*
void loop() {
  static uint8_t ch = 0;

  // Set frequency from your hop table
  uint32_t frf = hopTable[ch];
  radio.writeReg(REG_FRFMSB, (frf >> 16) & 0xFF);
  radio.writeReg(REG_FRFMID, (frf >> 8) & 0xFF);
  radio.writeReg(REG_FRFLSB, frf & 0xFF);

  delay(5); // settle

  // Sample RSSI a few times and take the minimum (quietest)
  int16_t minRSSI = 0;
  for (int i = 0; i < 5; i++) {
    int16_t r = radio.readRSSI();
    if (i == 0 || r < minRSSI) minRSSI = r;
    delay(20);
  }

  Serial.print("CH ");
  Serial.print(ch);
  Serial.print("  FRF=0x");
  Serial.print(frf, HEX);
  Serial.print("  RSSI=");
  Serial.println(minRSSI);

  ch++;
  if (ch >= HOP_TABLE_SIZE) {
    Serial.println("=== sweep complete ===");
    delay(2000); // pause before next sweep
    ch = 0;
  }
}
*/

void loop() {
  if (!packetReady) {
    static uint32_t last = 0;
    if (millis() - last > 500) {
      last = millis();
      int16_t rssi = radio.readRSSI();
      Serial.print(F("idle RSSI="));
      Serial.println(rssi);
    }
    return;
  }

  packetReady = false;

  // Blink LED on packet detect
  digitalWrite(LED_PIN, HIGH);
  delay(30);
  digitalWrite(LED_PIN, LOW);

  // Read FIFO
  uint8_t len = radio.readReg(0x00);  // FIFO register
  uint8_t buf[64];
  for (uint8_t i = 0; i < len && i < sizeof(buf); i++) {
    buf[i] = radio.readReg(0x00);
  }

  Serial.print(F("Packet! RSSI="));
  Serial.print(radio.readRSSI());
  Serial.print(F(" Len="));
  Serial.println(len);

  Serial.print(F("Data: "));
  for (uint8_t i = 0; i < len; i++) {
    if (buf[i] < 16) Serial.print('0');
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  // CRC check (Davis CRC-CCITT)
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len - 2; i++) {
    crc ^= (uint16_t)buf[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }

  uint16_t rxCrc = (buf[len - 2] << 8) | buf[len - 1];

  if (crc == rxCrc) {
    Serial.println(F("CRC OK"));
  } else {
    Serial.print(F("CRC FAIL (calc="));
    Serial.print(crc, HEX);
    Serial.print(F(" rx="));
    Serial.print(rxCrc, HEX);
    Serial.println(')');
  }

  Serial.println();
}

