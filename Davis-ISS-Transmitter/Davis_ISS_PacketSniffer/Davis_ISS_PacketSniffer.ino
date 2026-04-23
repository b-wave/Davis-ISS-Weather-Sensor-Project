/* Phase 4 – Build (4/22/2026)
   - Uses DavisRadio high-level API
   - No direct RX driver calls
   - Clean register dump, hop loop, RSSI, packet receive
*/

#include <Arduino.h>
#include "DavisConfig.h"
#include "DavisRadio.h"

// --- Pin wiring ---
static const uint8_t PIN_CS    = 10;
static const uint8_t PIN_IRQ   = 2;
static const uint8_t PIN_RESET = 3;



struct HopEvent {
    uint32_t t;       // micros() when packet was seen
    uint8_t  channel; // hop index
    int8_t   rssi;    // dBm
    uint8_t  len;     // payload length
};

static const uint8_t HOP_HISTORY_SIZE = 8;
HopEvent hopHistory[HOP_HISTORY_SIZE];
uint8_t  hopHistoryIndex = 0;
bool     hopHistoryFull  = false;

float    framePeriodUs   = 0;   // estimated ISS frame period (µs)
uint8_t  lastHopIndex    = 0;

// Phase 4B predictive timing
uint32_t lastHopTimeUs   = 0;
uint32_t nextHopTimeUs   = 0;
bool     havePrediction  = false;
uint8_t goodEstimates = 0;

// --- Global objects ---
DavisConfig config(DAVIS_REGION_US_915);
DavisRadio  radio(config, PIN_CS, PIN_IRQ, PIN_RESET);

// Dump all registers using DavisRadio wrapper
void dumpRegisters() {
    Serial.println("---- RFM69 Register Dump ----");
    for (uint8_t addr = 0x01; addr <= 0x71; addr++) {
        uint8_t val = radio.readReg(addr);
        Serial.print("0x");
        if (addr < 0x10) Serial.print('0');
        Serial.print(addr, HEX);
        Serial.print(": 0x");
        if (val < 0x10) Serial.print('0');
        Serial.println(val, HEX);
    }
    Serial.println("-----------------------------");
}

void recordHopEvent(uint8_t hop, uint8_t len, int16_t rssi) {
    static uint32_t lastTime = 0;
    static uint8_t  lastHop  = 0;

    uint32_t now = micros();

    // 1) First packet or long gap? Just reset baseline, don't estimate.
    if (lastTime == 0 || (now - lastTime) > 5'000'000UL) { // > 5 s
        lastTime       = now;
        lastHop        = hop;
        goodEstimates  = 0;
        havePrediction = false;
        return;
    }

    uint32_t dt = now - lastTime;
    uint8_t dh = (hop + config.hopCount - lastHop) % config.hopCount;

    // 2) Require true sequential hops
    if (dh != 1) {
        lastTime       = now;
        lastHop        = hop;
        goodEstimates  = 0;
        havePrediction = false;
        return;
    }

    // 3) Ignore obviously wrong dt (too small or too big)
        // was: 1.5–4.0 s
        if (dt < 1'200'000UL || dt > 4'500'000UL) {
            lastTime = now;
            lastHop  = hop;
            return;
        }

 

    uint32_t est = dt; // dh == 1, so est = dt

    // 4) Estimator + good-count logic
    
    if (est > 2'200'000UL && est < 3'000'000UL) {
        if (framePeriodUs == 0) framePeriodUs = est;
        else framePeriodUs = (framePeriodUs * 7 + est) / 8;

        goodEstimates++;

        Serial.print("Frame period estimate: ");
        Serial.print(framePeriodUs);
        Serial.print(" us  (");
        Serial.print(goodEstimates);
        Serial.println(" good)");
        
        if (goodEstimates >= 2) {   // was 3
            lastHopTimeUs  = now;
            nextHopTimeUs  = now + (uint32_t)framePeriodUs;
            havePrediction = true;
        }

    } else {
        goodEstimates = 0;
    }

    lastTime = now;
    lastHop  = hop;
}


void updateFramePeriodEstimate() {
    // Phase 4: disabled old estimator
    return;
}


void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("Davis ISS Packet Sniffer (Phase 4)");
    Serial.println("----------------------------------");



    // Basic register checks
    Serial.print("Version: "); Serial.println(radio.readReg(0x10), HEX);
    Serial.print("OP Mode: "); Serial.println(radio.readReg(0x01), HEX);

    Serial.print("FRF: ");
    Serial.print(radio.readReg(0x07), HEX); Serial.print(" ");
    Serial.print(radio.readReg(0x08), HEX); Serial.print(" ");
    Serial.println(radio.readReg(0x09), HEX);

    Serial.print("IRQ1: "); Serial.println(radio.readReg(0x27), HEX);
    Serial.print("IRQ2: "); Serial.println(radio.readReg(0x28), HEX);

    Serial.print("DataModul (0x02): ");
    Serial.println(radio.readReg(0x02), HEX);

    // Quick RSSI samples
   // for (int i = 0; i < 5; i++) {
   //     Serial.print("RSSI: ");
   //     Serial.println(radio.rawRSSI());
  //      delay(50);
  //  }



        // Initialize radio
    radio.begin();

    // Park on hop 30 for initial testing
    radio.setHop(30);
    radio.enterRX();
    delay(5);    
    
    dumpRegisters();

}

void loop() {
    uint32_t now = micros();

    // Phase 4B: predictive hop tuning
    if (havePrediction && framePeriodUs > 0) {
        int32_t dt = (int32_t)(nextHopTimeUs - now);

        // If we're within 80 ms before the predicted time, pre‑tune
        if (dt < 80'000 && dt > -200'000) {
            uint8_t nextHop = (radio.currentHop() + 1) % config.hopCount;
            radio.setHop(nextHop);
            radio.enterRX();
        }
    } else {
        // Bootstrap behavior if we don't have a good estimate yet
        //radio.enterRX();
    }

    uint8_t buf[64];
    uint8_t len = sizeof(buf);
    int16_t pktRSSI = 0;

    // (Old Phase‑3 estimator now effectively disabled)
    // updateFramePeriodEstimate();

    // Try to receive a packet
    if (radio.receiveFrame(buf, len, pktRSSI)) {

        Serial.print("PACKET on channel ");
        Serial.print(radio.currentHop());
        Serial.print(" len=");
        Serial.print(len);
        Serial.print(" RSSI=");
        Serial.println(pktRSSI);

        // Record timing + update prediction
        recordHopEvent(radio.currentHop(), len, pktRSSI);

        // Bootstrap hop advance (still keep this as a fallback)
        uint8_t nextHop = (radio.currentHop() + 1) % config.hopCount;
        radio.setHop(nextHop);

        // Print payload
        for (uint8_t i = 0; i < len; i++) {
            if (buf[i] < 16) Serial.print('0');
            Serial.print(buf[i], HEX);
            Serial.print(' ');
        }
        Serial.println();
    }
}


