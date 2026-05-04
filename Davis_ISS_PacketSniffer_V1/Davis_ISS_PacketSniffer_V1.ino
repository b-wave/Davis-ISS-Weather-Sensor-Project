// =============================================================================
// Davis ISS Packet Sniffer — V1.2.2
// =============================================================================
//
// REV:   050326
// VERS:  1.2.2
//
// CHANGELOG:
// ─────────────────────────────────────────────────────────────────────────────
// V1.2.2 (050326) — Radio-lockup fix, noise guard, station filter
//   • FIX: radio.setChannel() now ALWAYS called after receiveDone()
//          to prevent radio staying in STANDBY mode (caused 15+ min lockups)
//   • FIX: Noise guard — CRC FAIL packets ignored during park mode
//          (hopCount==0). Only CRC OK can break out of park-and-wait.
//          During tracking (hopCount>0), CRC FAIL still follows ISS.
//   • FIX: Station ID filter — rejects packets from wrong station during
//          park mode so multi-ISS environments don't cross-lock.
//   • FIX: Resync (hopCount>12) now returns immediately after parking
//          to prevent unnecessary channel advance.
//   • ADD: Station change command (1-8) — switch stations without recompile.
//   • FIX: 'i' command now shows actual PACKET_INTERVAL, not hardcoded value.
//   • FIX: 'r' resync command properly resets to park-and-wait mode.
//
// V1.2.1 (050226) — CRC FAIL hop fix
//   • FIX: CRC FAIL during tracking keeps hopping (ISS moved regardless)
//          OLD (broken): hopCount = davisCrcOk ? 1 : 0;
//          NEW: hopCount = 1 for any received frame WHILE TRACKING
//
// V1.2   (050126) — RAW CSV mode, stats, output toggle
//   • ADD: RAW CSV output mode (R) for TeraTerm logging & analysis
//   • ADD: VERBOSE mode (V) for human-readable console output
//   • ADD: Packet statistics (pass/fail/missed) with periodic reporting
//   • ADD: printPacketVerbose() and printPacketRaw() formatters
//
// V1.1   (042726) — Command processor, LEDs
//   • ADD: Serial command processor (? r h s d i t)
//   • ADD: Non-blocking LED subsystem (Green=OK, Yellow=Miss, Red=Resync)
//
// V1.0   (042526) — First working version
//   • Correct Davis 6-byte CRC16-CCITT implementation
//   • Frequency-hopping receiver with predictive hop timing
//
// ─────────────────────────────────────────────────────────────────────────────
// CRC Gotcha Summary:
//   Davis ISS packet = 8 bytes: [0-5]=payload, [6-7]=CRC16 (big-endian)
//   CRC is computed ONLY over the first 6 bytes. CRC must NOT be zero.
//   Computing over 8 bytes will ALWAYS fail — classic mistake.
//
// Station ID Numbering:
//   Console/DIP switches: 1-8  |  Packet byte0 & 0x07: 0-7
//   Code uses CONSOLE number.  Packet TX ID = console - 1.
//
// Packet Interval Formula:
//   PACKET_INTERVAL = (40 + console_station) / 16 * 1000 ms
//   Station 1: 2562.5ms  Station 3: 2687.5ms  Station 5: 2812.5ms
//   Station 8: 3000.0ms
// =============================================================================

#include <Arduino.h>
#include <SPI.h>
#include "DavisRFM69.h"
#include "Davisdef.h"
#include "RFM69registers.h"

// =============================================================================
// HARDWARE CONFIGURATION — Teensy + RFM69
// =============================================================================
#define RFM69_RST     3
#define RFM69_CS     10
#define RF69_IRQ_PIN  2
#define RF69_IRQ_NUM  2

// LED pins
#define LED_GOOD     14   // Green  = CRC OK
#define LED_MISSED   15   // Yellow = Missed packet
#define LED_RESYNC   16   // Red    = Resync event
#define LED_PULSE_MS 60

// Uncomment only for RFM69HW (high-power version)
// #define IS_RFM69HW

// =============================================================================
// ISS CONFIGURATION
// =============================================================================
// Set this to your ISS console station number (1-8).
// Can also be changed at runtime by pressing 1-8.
byte TRANSMITTER_STATION_ID = 3;

// Computed from station ID — do NOT hardcode
float PACKET_INTERVAL = (40.0 + TRANSMITTER_STATION_ID) / 16.0 * 1000.0;

// Resync threshold: after this many consecutive misses, park and wait
#define RESYNC_THRESHOLD 12

// =============================================================================
// OUTPUT MODES
// =============================================================================
#define MODE_VERBOSE 0
#define MODE_RAW     1
uint8_t outputMode = MODE_VERBOSE;  // Default: human-readable

// =============================================================================
// STATISTICS
// =============================================================================
uint32_t statTotalPackets = 0;
uint32_t statCrcPass      = 0;
uint32_t statCrcFail      = 0;
uint32_t statMissedHops   = 0;
uint32_t lastStatsTime    = 0;
#define STATS_INTERVAL_MS 300000  // Print stats every 5 minutes

// =============================================================================
// RADIO OBJECT
// =============================================================================
DavisRFM69 radio(RFM69_CS, RF69_IRQ_PIN, false, RF69_IRQ_NUM);

// =============================================================================
// LED STATE MACHINE (non-blocking)
// =============================================================================
uint32_t ledGoodUntil   = 0;
uint32_t ledMissedUntil = 0;
uint32_t ledResyncUntil = 0;

void ledGood()   { ledGoodUntil   = millis() + LED_PULSE_MS; }
void ledMissed() { ledMissedUntil = millis() + LED_PULSE_MS; }
void ledResync() { ledResyncUntil = millis() + LED_PULSE_MS; }

void updateLEDs() {
    uint32_t now = millis();
    digitalWrite(LED_GOOD,   now < ledGoodUntil);
    digitalWrite(LED_MISSED, now < ledMissedUntil);
    digitalWrite(LED_RESYNC, now < ledResyncUntil);
}

// =============================================================================
// PACKET OUTPUT FORMATTERS
// =============================================================================
void printPacketVerbose(volatile uint8_t* buf, uint8_t len,
                        uint8_t hop, int16_t rssi, bool crcOk) {
    Serial.print(F("Channel="));
    Serial.print(hop);
    Serial.print(F(" RSSI="));
    Serial.print(rssi);
    Serial.print(crcOk ? F(" CRC OK  ") : F(" CRC FAIL "));
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 16) Serial.print('0');
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

void printPacketRaw(volatile uint8_t* buf, uint8_t len,
                    uint8_t hop, int16_t rssi, bool crcOk) {
    // Format: millis,hop,rssi,crc,b0,b1,...,b7,type
    Serial.print(millis());
    Serial.print(',');
    Serial.print(hop);
    Serial.print(',');
    Serial.print(rssi);
    Serial.print(',');
    Serial.print(crcOk ? 1 : 0);
    for (uint8_t i = 0; i < len; i++) {
        Serial.print(',');
        if (buf[i] < 16) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
    // Append message type nibble for easy filtering
    Serial.print(',');
    Serial.print((buf[0] >> 4) & 0x0F, HEX);
    Serial.println();
}

void printStats() {
    uint32_t total = statCrcPass + statCrcFail;
    float pct = (total > 0) ? (100.0 * statCrcPass / total) : 0.0;
    Serial.print(F("#STATS,uptime_s="));
    Serial.print(millis() / 1000);
    Serial.print(F(",pass="));
    Serial.print(statCrcPass);
    Serial.print(F(",fail="));
    Serial.print(statCrcFail);
    Serial.print(F(",missed="));
    Serial.print(statMissedHops);
    Serial.print(F(",pass_pct="));
    Serial.println(pct, 1);
}

// =============================================================================
// COMMAND PROCESSOR
// =============================================================================
// Commands:
//   ?     Help
//   V     Switch to VERBOSE output mode
//   R     Switch to RAW CSV mode (prints header)
//   r     Force resync (park and wait)
//   h     Print hop info
//   s     Radio status
//   d     Dump all registers
//   i     ISS info (station, timing, stats)
//   t     TX test placeholder
//   1-8   Switch to station 1-8 (console number)
// =============================================================================
void processCommand(char c) {
    // Forward-declare loop statics we need to access
    // (These are extern'd from the loop — see loop() for declarations)
    extern uint8_t  currentHop;
    extern uint8_t  hopCount;
    extern uint32_t lastRxTime;

    switch (c) {
        case '?':
            Serial.println(F("Commands:"));
            Serial.println(F("  ?     Help"));
            Serial.println(F("  V     Verbose output mode"));
            Serial.println(F("  R     RAW CSV mode (for logging)"));
            Serial.println(F("  r     Force resync"));
            Serial.println(F("  h     Hop info"));
            Serial.println(F("  s     Radio status"));
            Serial.println(F("  d     Dump registers"));
            Serial.println(F("  i     ISS info + stats"));
            Serial.println(F("  t     TX test (stub)"));
            Serial.println(F("  1-8   Switch to station 1-8"));
            break;

        case 'V': case 'v':
            outputMode = MODE_VERBOSE;
            Serial.println(F("[Mode: VERBOSE]"));
            break;

        case 'R':
            outputMode = MODE_RAW;
            Serial.println(F("[Mode: RAW CSV]"));
            Serial.println(F("millis,hop,rssi,crc,b0,b1,b2,b3,b4,b5,b6,b7,type"));
            break;

        case 'r':
            // --- Force resync: park on current channel and wait ---
            hopCount = 0;
            lastRxTime = micros() / 1000UL;
            ledResync();
            Serial.println(F("Resync forced- parked on ch "));
            Serial.print(currentHop);
            Serial.println(F(", waiting for ISS..."));
            break;

        case 'h':
            Serial.print(F("Current hop: "));
            Serial.println(currentHop);
            Serial.print(F("Next hop: "));
            Serial.println((currentHop + 1) % DAVIS_FREQ_TABLE_LENGTH);
            Serial.print(F("FRF MSB: 0x"));
            Serial.println(radio.readReg(REG_FRFMSB), HEX);
            break;

        case 's':
            Serial.println(F("Radio status:"));
            Serial.print(F("  RSSI: "));
            Serial.println(radio.readRSSI());
            Serial.print(F("  Mode: 0x"));
            Serial.println(radio.readReg(REG_OPMODE), HEX);
            Serial.print(F("  IRQ Flags 1: 0x"));
            Serial.println(radio.readReg(REG_IRQFLAGS1), HEX);
            Serial.print(F("  IRQ Flags 2: 0x"));
            Serial.println(radio.readReg(REG_IRQFLAGS2), HEX);
            break;

        case 'd':
            Serial.println(F("Register dump:"));
            radio.readAllRegs();
            break;

        case 'i':
            Serial.println(F("ISS Info:"));
            Serial.print(F("  Station ID:      "));
            Serial.print(TRANSMITTER_STATION_ID);
            Serial.print(F(" (console)  TX ID = "));
            Serial.print(TRANSMITTER_STATION_ID - 1);
            Serial.println(F(" (packet)"));
            Serial.print(F("  Packet interval: "));
            Serial.print((uint16_t)PACKET_INTERVAL);
            Serial.println(F(" ms"));
            Serial.print(F("  Output mode:     "));
            Serial.println(outputMode == MODE_RAW ? F("RAW CSV") : F("VERBOSE"));
            Serial.print(F("  Hop count:       "));
            Serial.println(hopCount);
            Serial.print(F("  Current hop:     "));
            Serial.println(currentHop);
            Serial.println(F("  --- Stats ---"));
            Serial.print(F("  Total frames:    "));
            Serial.println(statTotalPackets);
            Serial.print(F("  CRC OK:          "));
            Serial.println(statCrcPass);
            Serial.print(F("  CRC FAIL:        "));
            Serial.println(statCrcFail);
            Serial.print(F("  Missed hops:     "));
            Serial.println(statMissedHops);
            if (statCrcPass + statCrcFail > 0) {
                Serial.print(F("  Pass rate:       "));
                Serial.print(100.0 * statCrcPass / (statCrcPass + statCrcFail), 1);
                Serial.println(F("%"));
            }
            break;

        case 't':
            Serial.println(F("TX test mode not implemented yet."));
            break;

        // --- Station ID change (digits 1-8) ---
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
            TRANSMITTER_STATION_ID = c - '0';
            PACKET_INTERVAL = (40.0 + TRANSMITTER_STATION_ID) / 16.0 * 1000.0;
            // Force resync on new station
            hopCount = 0;
            lastRxTime = micros() / 1000UL;
            // Clear stats for clean slate
            statTotalPackets = 0;
            statCrcPass      = 0;
            statCrcFail      = 0;
            statMissedHops   = 0;
            ledResync();
            Serial.print(F("\n>>> Station ID -> "));
            Serial.print(TRANSMITTER_STATION_ID);
            Serial.print(F(" (console)  TX ID = "));
            Serial.print(TRANSMITTER_STATION_ID - 1);
            Serial.print(F(" (packet)  Interval = "));
            Serial.print((uint16_t)PACKET_INTERVAL);
            Serial.println(F("ms"));
            Serial.println(F(">>> Resyncing - parked & waiting for ISS..."));
            break;

        default:
            Serial.print(F("[Unknown: '"));
            Serial.print(c);
            Serial.println(F("' - type ? for help]"));
            break;
    }
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println(F("─────────────────────────────"));
    Serial.println(F("  Davis ISS Packet Sniffer"));
    Serial.println(F("  REV: 050326   VERS: 1.2.2"));
    Serial.println(F("─────────────────────────────"));

    // ----- Hardware bring-up -----
    pinMode(RFM69_CS,    OUTPUT);
    pinMode(RF69_IRQ_PIN, INPUT);
    pinMode(RFM69_RST,   OUTPUT);
    pinMode(LED_GOOD,    OUTPUT);
    pinMode(LED_MISSED,  OUTPUT);
    pinMode(LED_RESYNC,  OUTPUT);

    digitalWrite(LED_GOOD,   LOW);
    digitalWrite(LED_MISSED,  LOW);
    digitalWrite(LED_RESYNC,  LOW);
    digitalWrite(RFM69_CS,   HIGH);
    digitalWrite(RFM69_RST,  LOW);

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

    // Initialize radio
    radio.initialize();

    // Start on a known hop
    uint8_t startHop = 30;
    radio.setChannel(startHop);
    delay(5);

    // Show config
    Serial.print(F("Station: "));
    Serial.print(TRANSMITTER_STATION_ID);
    Serial.print(F("  Interval: "));
    Serial.print((uint16_t)PACKET_INTERVAL);
    Serial.println(F("ms"));
    Serial.println(F("LEDs: Green=OK  Yellow=Miss  Red=Resync"));
    Serial.println(F("Commands: ? for help  |  V=verbose  R=raw CSV  |  1-8=station"));
    Serial.println(F("Syncing... park-and-wait on ch 30 (up to ~2.5 min)"));

    lastStatsTime = millis();
}

// =============================================================================
// MAIN LOOP
// =============================================================================
//
// State machine:
//   hopCount == 0 : PARKED — sitting on one channel, waiting for ISS
//                   Only CRC OK from our station breaks out of park.
//                   CRC FAIL is ignored (noise/other station).
//   hopCount >= 1 : TRACKING — following ISS hop-by-hop
//                   Both CRC OK and CRC FAIL advance (ISS moved regardless).
//   hopCount > 12 : LOST — too many misses, reset to PARKED.
//
// =============================================================================

// These must be file-scope so processCommand() can access them via extern
uint8_t  currentHop  = 30;   // Start on hop 30 (matches setup)
uint8_t  hopCount    = 0;    // Start PARKED (wait for ISS)
uint32_t lastRxTime  = 0;    // ms timestamp of last event

void loop() {
    // ----- Serial command handling -----
    if (Serial.available()) {
        char cmd = Serial.read();
        processCommand(cmd);
    }

    // ----- Periodic stats output -----
    if (millis() - lastStatsTime >= STATS_INTERVAL_MS) {
        printStats();
        lastStatsTime = millis();
    }

    // =====================================================================
    // 1. RECEIVE — Check if radio has a frame
    // =====================================================================
    if (radio.receiveDone()) {
        int16_t           rssi = radio.RSSI;
        volatile uint8_t* buf  = radio.DATA;
        uint8_t           len  = DAVIS_PACKET_LEN;

        // --- CRC check (6 bytes only!) ---
        uint16_t calc = radio.crc16_ccitt(buf, 6);
        uint16_t recv = (buf[6] << 8) | buf[7];
        bool davisCrcOk = (calc == recv) && (calc != 0);

        // --- Extract station ID from packet ---
        uint8_t rxTxId     = buf[0] & 0x07;           // 0-7
        uint8_t rxStation  = rxTxId + 1;               // 1-8 console number
        bool    rightStation = (rxStation == TRANSMITTER_STATION_ID);

        // --- Update statistics ---
        statTotalPackets++;
        if (davisCrcOk) {
            statCrcPass++;
            ledGood();
        } else {
            statCrcFail++;
            ledMissed();
        }

        // --- Output packet in current mode ---
        if (outputMode == MODE_VERBOSE) {
            printPacketVerbose(buf, len, currentHop, rssi, davisCrcOk);
        } else {
            printPacketRaw(buf, len, currentHop, rssi, davisCrcOk);
        }

        // =================================================================
        // V1.2.2 — Context-aware hop decision
        // =================================================================
        //
        // TRACKING (hopCount > 0):
        //   Any frame (CRC OK or FAIL) → follow ISS to next channel.
        //   The ISS has moved regardless of whether we decoded the packet.
        //
        // PARKED (hopCount == 0):
        //   CRC OK + right station  → ISS found! Start tracking.
        //   CRC OK + wrong station  → ignore, keep waiting.
        //   CRC FAIL                → noise/other station, ignore.
        //
        // In ALL cases, radio.setChannel() is called to restart RX.
        // This prevents the radio from staying in STANDBY after
        // receiveDone() reads the FIFO.
        // =================================================================

        if (hopCount > 0) {
            // --- TRACKING: follow ISS regardless of CRC result ---
            hopCount = 1;
            lastRxTime = micros() / 1000UL;
            currentHop = (currentHop + 1) % DAVIS_FREQ_TABLE_LENGTH;
        }
        else if (davisCrcOk && rightStation) {
            // --- PARKED: real ISS found! Lock on and start tracking ---
            hopCount = 1;
            lastRxTime = micros() / 1000UL;
            currentHop = (currentHop + 1) % DAVIS_FREQ_TABLE_LENGTH;
        }
        // else: PARKED + noise or wrong station → stay on same channel

        // ALWAYS re-tune radio to restart RX (prevents STANDBY lockup)
        radio.setChannel(currentHop);

        return;
    }

    // =====================================================================
    // 2. PREDICTIVE HOP — Advance when a packet is missed
    // =====================================================================
    // Only runs while TRACKING (hopCount > 0).
    // While PARKED (hopCount == 0), we just sit and wait.
    // =====================================================================
    uint32_t now = micros() / 1000UL;

    if ((hopCount > 0) &&
        ((now - lastRxTime) > (hopCount * PACKET_INTERVAL + 200)))
    {
        hopCount++;
        statMissedHops++;

        if (hopCount > RESYNC_THRESHOLD) {
            // --- LOST: too many misses → park on current channel ---
            hopCount = 0;
            lastRxTime = micros() / 1000UL;
            ledResync();
            // Stay on current channel — do NOT advance.
            // ISS will visit this channel within one full hop cycle
            // (~137s for Stn 3, ~143s for Stn 5).
            radio.setChannel(currentHop);  // Restart RX on same channel
            return;  // Skip the channel advance below
        }

        // Still tracking — advance to next expected channel
        currentHop = (currentHop + 1) % DAVIS_FREQ_TABLE_LENGTH;
        radio.setChannel(currentHop);
    }

    // ----- Update LEDs -----
    updateLEDs();
}
