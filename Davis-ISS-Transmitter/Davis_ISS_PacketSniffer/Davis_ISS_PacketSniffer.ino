// Davis_ISS_PacketSniffer.ino
// Clean, quiet, manual-channel Davis ISS sniffer

#include <SPI.h>
#include "DavisRF69.h"
#include "DavisRFM69registers.h"
#include "DavisConfig.h"   // for DAVIS_FREQ_TABLE[]

// -----------------------------
// Pin + radio config
// -----------------------------
const uint8_t PIN_CS    = 10;
const uint8_t PIN_DIO0  = 2;
const uint8_t PIN_RESET = 3;
const uint8_t PIN_LED   = 14;

DavisRF69 radio(PIN_CS, PIN_DIO0);  //was 
//DavisRF69 radio();

// Davis hop table + channel count should already exist in your project
extern const uint32_t DAVIS_FREQ_TABLE[];
const uint8_t CHANNEL_COUNT = 51;

// -----------------------------
// Runtime state
// -----------------------------
uint8_t currentChannel = 0;

// Debug toggles
bool DEBUG_SEARCH = false;   // RSSI / OpMode spam
bool DEBUG_INIT   = true;    // One-time init prints

// -----------------------------
// Helpers
// -----------------------------
void setChannelFromTable(uint8_t idx) {
  if (idx >= CHANNEL_COUNT) idx = 0;
  currentChannel = idx;

  uint32_t frf = DAVIS_FREQ_TABLE[idx];

  radio.writeReg(REG_FRFMSB, (frf >> 16) & 0xFF);
  radio.writeReg(REG_FRFMID, (frf >> 8)  & 0xFF);
  radio.writeReg(REG_FRFLSB,  frf        & 0xFF);
}

void showChannel() {
  Serial.print(F("Channel: "));
  Serial.println(currentChannel);
}

void showRSSI() {
  int16_t rssi = radio.readRSSI();
  Serial.print(F("RSSI on CH "));
  Serial.print(currentChannel);
  Serial.print(F(" = "));
  Serial.println(rssi);
}

// -----------------------------
// Serial command handler
// -----------------------------
void handleSerialCommand() {
  if (!Serial.available()) return;
  char c = Serial.read();

  switch (c) {
    case 'n':   // next channel
      currentChannel = (currentChannel + 1) % CHANNEL_COUNT;
      setChannelFromTable(currentChannel);
      showChannel();
      break;

    case 'p':   // previous channel
      currentChannel = (currentChannel == 0) ? (CHANNEL_COUNT - 1) : (currentChannel - 1);
      setChannelFromTable(currentChannel);
      showChannel();
      break;

    case 'r':   // read RSSI
      showRSSI();
      break;

    case 's':   // show channel
      showChannel();
      break;

    case 'd':   // dump registers (assumes you already have this in DavisRF69.cpp)
      radio.dumpRegisters();
      break;

    case 'q':   // toggle search debug
      DEBUG_SEARCH = !DEBUG_SEARCH;
      Serial.print(F("DEBUG_SEARCH = "));
      Serial.println(DEBUG_SEARCH ? F("ON") : F("OFF"));
      break;

    default:
      // ignore unknown
      break;
  }
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  while (!Serial) { ; }

  if (DEBUG_INIT) {
    Serial.println();
    Serial.println(F("Davis Vantage Vue ISS Sniffer - Clean Manual-Channel Mode"));
    Serial.print(F("CS="));    Serial.print(PIN_CS);
    Serial.print(F(", DIO0=")); Serial.print(PIN_DIO0);
    Serial.print(F(", RESET="));Serial.print(PIN_RESET);
    Serial.print(F(", LED="));  Serial.println(PIN_LED);
  }

  radio.initialize(PIN_CS, PIN_DIO0, PIN_RESET);

  // Start on channel 0 by default
  setChannelFromTable(0);

  // Force RX mode and confirm
  radio.setMode(RF_OPMODE_RECEIVER);
  delay(5);
  uint8_t op = radio.readReg(REG_OPMODE);

  if (DEBUG_INIT) {
    Serial.print(F("OPMODE after init = 0x"));
    Serial.println(op, HEX);
    Serial.println(F("Sniffer ready. Commands: n/p (ch), r (RSSI), s (show ch), d (dump), q (toggle search debug)"));
  }
}

// -----------------------------
// Main loop
// -----------------------------
void loop() {
  handleSerialCommand();

  // Optional: light search debug (RSSI + OpMode)
  if (DEBUG_SEARCH) {
    uint8_t op = radio.readReg(REG_OPMODE);
    int16_t rssi = radio.readRSSI();
    Serial.print(F("OpMode=0x"));
    Serial.print(op, HEX);
    Serial.print(F("  RSSI="));
    Serial.println(rssi);
    delay(200);
  }

  // Check for packet
  if (radio.receiveDone()) {
    digitalWrite(PIN_LED, HIGH);

    int16_t rssi = radio.readRSSI();

    Serial.print(F("PKT CH="));
    Serial.print(currentChannel);
    Serial.print(F(" RSSI="));
    Serial.print(rssi);
    Serial.print(F(" LEN="));
    Serial.print(radio.DATALEN);
    Serial.print(F("  "));

    for (uint8_t i = 0; i < radio.DATALEN; i++) {
      if (radio.DATA[i] < 0x10) Serial.print('0');
      Serial.print(radio.DATA[i], HEX);
      Serial.print(' ');
    }
    Serial.println();

    digitalWrite(PIN_LED, LOW);
  }
}
