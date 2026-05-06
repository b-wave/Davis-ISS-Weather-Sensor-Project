/**
 * ISS_packetEngine.ino — Complete ISS Packet Engine with Sensor Drivers
 *
 * Generates Davis-format 8-byte weather packets from real sensor readings,
 * following the VP2/Vue/Custom transmit sequence rotation.
 *
 * Packet types supported:
 *   0x20  Supercap / Battery Voltage   (Vue standard — battery V from divider)
 *   0x30  Lightning / Reserved         (future AS3935 — or fallback to 0xC0)
 *   0x40  UV Index                     (future — sensor TBD)
 *   0x50  Rain Rate                    (seconds since last bucket tip)
 *   0x60  Solar Radiation              (future — sensor TBD)
 *   0x70  PV (Solar Cell) Voltage      (Vue standard — solar panel V)
 *   0x80  Temperature                  (BME280)
 *   0x90  Wind Gust                    (hall-effect peak tracker)
 *   0xA0  Humidity                     (BME280)
 *   0xE0  Rain                         (bucket tip counter — future)
 *
 * Wind speed (byte 1) and direction (byte 2) are in EVERY packet.
 *
 * Hardware:
 *   Teensy 4.x with RFM69 radio (SPI), BME280 (I2C), AS5048B (I2C),
 *   hall-effect wind switch (interrupt), analog inputs, TX ID switch+LED.
 *
 * Build options:
 *   USE_TEST_STUBS=1    — run without any hardware (configurable fake data)
 *   TX_SEQ_MODE         — 0=VP2, 1=Vue, 2=Custom hybrid
 *   SERIAL_DEBUG        — verbose debug output on USB serial
 *
 * Reference: VPTools/AnemometerTX by kobuki, DeKay's protocol.txt
 */

#include  <SensorDriver.h>
#include  <DavisCalibration.h>
#include  <BME280Driver.h>
#include  <WindSpeedDriver.h>
#include  <AS5048BDriver.h>
#include  <AnalogDriver.h>
#include  <TxIdManager.h>

// ============================================================================
// BUILD CONFIGURATION
// ============================================================================

#define USE_TEST_STUBS    1       // 1 = no hardware needed (stub sensors)
#define TX_SEQ_MODE       2       // 0=VP2, 1=Vue, 2=Custom hybrid
#define SERIAL_DEBUG      1       // 1 = verbose USB serial debug output
#define SERIAL1_OUTPUT    1       // 1 = raw packet output on Serial1 (bridge)

// Pin assignments (must match Sensor Driver Architecture doc)
#define PIN_WIND_SPEED    2       // Interrupt — hall-effect switch
#define PIN_AS3935_IRQ    3       // Future — lightning detector interrupt
#define PIN_TX_SWITCH     4       // Momentary pushbutton (INPUT_PULLUP)
#define PIN_TX_LED        5       // Status LED
#define PIN_AS3935_CS     6       // Future — SPI CS for lightning detector
#define PIN_RFM69_CS      10      // SPI CS for radio
#define PIN_SOLAR_V       A0      // Analog — solar cell voltage
#define PIN_BATT_V        A1      // Analog — battery voltage
#define PIN_BATT_THERM    A2      // Analog — battery thermistor
#define PIN_LDR           A3      // Analog — LDR luminosity

// Timing
#define PACKET_INTERVAL_MS  2500  // 2.5 seconds — Davis standard for TX ID 0

// ============================================================================
// CRC-CCITT (0x1021) — Davis standard, init 0x0000
// ============================================================================
// Standard CRC-CCITT (poly=0x1021, init=0x0000) lookup table
// Mathematically generated — matches DavisRFM69.cpp bit-by-bit computation
// and real Davis ISS hardware. Replaces the corrupted table that had 33 errors.
static const uint16_t PROGMEM CRC_TABLE[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/*
static const uint16_t CRC_TABLE[256] PROGMEM = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x4864, 0x5845, 0x6826, 0x7807, 0x08E0, 0x18C1, 0x28A2, 0x38A3,
    0xC94C, 0xD96D, 0xE90E, 0xF92F, 0x89C8, 0x99E9, 0xA98A, 0xB9AB,
    0x5A75, 0x4A54, 0x7A37, 0x6A16, 0x1AF1, 0x0AD0, 0x3AB3, 0x2A92,
    0xDB7D, 0xCB5C, 0xFB3F, 0xEB1E, 0x9BF9, 0x8BD8, 0xBBBB, 0xAB9A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x85A9, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
*/
uint16_t crc_ccitt(const uint8_t *buf, uint8_t len) {
    uint16_t crc = 0x0000;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t idx = ((crc >> 8) ^ buf[i]) & 0xFF;
        uint16_t tval;
        memcpy_P(&tval, &CRC_TABLE[idx], sizeof(uint16_t));
        crc = (crc << 8) ^ tval;
    }
    return crc;
}

void appendCRC(uint8_t *pkt) {
    uint16_t crc = crc_ccitt(pkt, 6);
    pkt[6] = (crc >> 8) & 0xFF;
    pkt[7] = crc & 0xFF;
}

// ============================================================================
// SENSOR INSTANCES
// ============================================================================

#if USE_TEST_STUBS
    // --- Hardware-free test mode ---
    BME280Stub      bme;
    WindSpeedStub   windSpeed;
    AS5048BStub     windDir;
    AnalogStub      solarV(PIN_SOLAR_V);
    AnalogStub      battV(PIN_BATT_V);
    AnalogStub      battTherm(PIN_BATT_THERM);
    AnalogStub      ldrLight(PIN_LDR);
    TxIdManagerStub txId;
#else
    // --- Real hardware ---
    BME280Driver    bme(0x76, &Wire, 0.0f);
    WindSpeedDriver windSpeed(PIN_WIND_SPEED, true);  // EC enabled
    AS5048BDriver   windDir(0x40, &Wire);
    AnalogDriver    solarV(PIN_SOLAR_V, 2.0f, ANALOG_VOLTAGE, 8);
    AnalogDriver    battV(PIN_BATT_V, 1.5f, ANALOG_VOLTAGE, 8);
    AnalogDriver    battTherm(PIN_BATT_THERM, 1.0f, ANALOG_NTC_THERM, 16);
    AnalogDriver    ldrLight(PIN_LDR, 1.0f, ANALOG_RELATIVE, 8);
    TxIdManager     txId(PIN_TX_SWITCH, PIN_TX_LED, 0);
#endif

// ============================================================================
// PACKET ENGINE STATE
// ============================================================================

uint8_t  packet[8];               // Current packet buffer
uint8_t  txSeqIndex = 0;          // Position in transmit sequence (0-19)
uint32_t lastPacketTime = 0;      // millis() of last packet
uint32_t packetCount = 0;         // Total packets generated

// Rain tracking state
uint8_t  rainBucketCount = 0;     // Running bucket tip counter (wraps at 255)
uint32_t lastRainTipTime = 0;     // millis() of last rain bucket tip
uint16_t rainRateSeconds = 0xFFFF; // Seconds since last tip (0xFFFF = no rain)

// Lightning state (future AS3935)
uint8_t  lightningDistance = 0;   // km (0 = no event)
uint8_t  lightningEnergy = 0;    // Relative energy
bool     lightningEvent = false;  // Event pending flag

// ============================================================================
// TRANSMIT SEQUENCE SELECTION
// ============================================================================

const uint8_t* getActiveSequence() {
    #if TX_SEQ_MODE == 0
        return TX_SEQ_VP2;
    #elif TX_SEQ_MODE == 1
        return TX_SEQ_VUE;
    #else
        return TX_SEQ_CUSTOM;
    #endif
}

const char* getSequenceName() {
    #if TX_SEQ_MODE == 0
        return "VP2";
    #elif TX_SEQ_MODE == 1
        return "Vue";
    #else
        return "Custom";
    #endif
}

uint8_t getSequenceType(uint8_t index) {
    uint8_t typeRaw;
    memcpy_P(&typeRaw, &(getActiveSequence()[index]), 1);
    return typeRaw;
}

// ============================================================================
// PACKET BUILDERS — one function per packet type
// Each populates bytes 3, 4, 5 of the packet buffer.
// Bytes 0 (header), 1 (wind speed), 2 (wind dir) are filled in buildPacket().
// Bytes 6-7 (CRC) are appended after all data bytes are set.
// ============================================================================

void buildTemperature(uint8_t *pkt) {
    // Type 0x80 — Temperature
    // Davis formula: temp_F = raw / 160.0
    // Encoding: raw = (int16_t)(temp_F * 160.0), big-endian in bytes 3-4
    int16_t raw = bme.getDavisTemperatureRaw();
    pkt[3] = (raw >> 8) & 0xFF;
    pkt[4] = raw & 0xFF;
    pkt[5] = 0x00;
}

void buildHumidity(uint8_t *pkt) {
    // Type 0xA0 — Humidity
    // Davis formula: humidity = (((byte4>>4) << 8) + byte3) / 10.0
    // Encoding: raw = humidity * 10, byte3 = raw & 0xFF,
    //           byte4 = (raw >> 8) << 4 | (byte4 & 0x0F)
    uint16_t raw = bme.getDavisHumidityRaw();
    pkt[3] = raw & 0xFF;
    pkt[4] = ((raw >> 8) & 0x03) << 4;
    pkt[5] = 0x00;
}

void buildWindGust(uint8_t *pkt) {
    // Type 0x90 — Wind Gust
    // Byte 3 = gust speed in mph
    // Byte 5 = gust direction (same encoding as byte 2)
    pkt[3] = windSpeed.getGustMPH();
    pkt[4] = 0x00;
    pkt[5] = windDir.getDavisDirectionByte();  // Direction of gust
    windSpeed.resetGust();                      // Reset after reading
}

void buildRain(uint8_t *pkt) {
    // Type 0xE0 — Rain
    // Byte 3 = running bucket tip counter (wraps 0-255)
    // Console tracks delta between readings
    // Each tip = 0.01 inches
    pkt[3] = rainBucketCount;
    pkt[4] = 0x01;   // Flags — per VPTools no-sensor pattern
    pkt[5] = 0x00;
}

void buildRainRate(uint8_t *pkt) {
    // Type 0x50 — Rain Rate (seconds since last bucket tip)
    // If no rain: 0xFF, 0x71 per VPTools no-sensor pattern
    if (rainRateSeconds >= 0xFFF) {
        // No rain or very long ago
        pkt[3] = 0xFF;
        pkt[4] = 0x71;
    } else {
        pkt[3] = (rainRateSeconds >> 4) & 0xFF;
        pkt[4] = ((rainRateSeconds & 0x0F) << 4) | 0x01;
    }
    pkt[5] = 0x00;
}

void buildUVIndex(uint8_t *pkt) {
    // Type 0x40 — UV Index
    // No UV sensor installed yet: send "no sensor" pattern
    // Future: UV Index = ((byte3<<8 | byte4) >> 6) / 50.0
    pkt[3] = 0xFF;
    pkt[4] = 0xC5;
    pkt[5] = 0x00;
}

void buildSolarRadiation(uint8_t *pkt) {
    // Type 0x60 — Solar Radiation (W/m^2)
    // No solar radiation sensor yet: send "no sensor" pattern
    // Future: Solar = ((byte3<<8 | byte4) >> 6) * 1.757936
    pkt[3] = 0xFF;
    pkt[4] = 0xC5;
    pkt[5] = 0x00;
}

void buildSupercapBattV(uint8_t *pkt) {
    // Type 0x20 — Supercap / Battery Voltage (Vue standard)
    //
    // Repurposed for battery monitoring. The Vue sends a raw ADC value
    // representing the supercap voltage. We send battery voltage instead.
    //
    // Encoding (matching Vue convention):
    //   raw = (uint16_t)(voltage * 100)   → e.g., 3.85V = 385
    //   byte3 = raw >> 2 (upper 8 bits of 10-bit value)
    //   byte4 = (raw & 0x03) << 6 | flags
    //
    // The VP2/Vue console may not display this value directly, but
    // it's captured in LOOP data and packet sniffers.
    battV.read();
    float voltage = battV.readVoltage();
    uint16_t raw = (uint16_t)(voltage * 100.0f);
    if (raw > 1023) raw = 1023;   // 10-bit clamp

    pkt[3] = (raw >> 2) & 0xFF;
    pkt[4] = ((raw & 0x03) << 6) | 0x05;
    pkt[5] = 0x00;

    #if SERIAL_DEBUG
        Serial.print("    [BattV=");
        Serial.print(voltage, 2);
        Serial.print("V raw=");
        Serial.print(raw);
        Serial.println("]");
    #endif
}

void buildPVVoltage(uint8_t *pkt) {
    // Type 0x70 — PV (Solar Cell) Voltage (Vue standard)
    //
    // Reports solar panel voltage for power management visibility.
    // Same encoding approach as supercap (0x20).
    //
    // Encoding:
    //   raw = (uint16_t)(voltage * 100)   → e.g., 5.24V = 524
    //   byte3 = raw >> 2
    //   byte4 = (raw & 0x03) << 6 | flags
    solarV.read();
    float voltage = solarV.readVoltage();
    uint16_t raw = (uint16_t)(voltage * 100.0f);
    if (raw > 1023) raw = 1023;

    pkt[3] = (raw >> 2) & 0xFF;
    pkt[4] = ((raw & 0x03) << 6) | 0x05;
    pkt[5] = 0x00;

    #if SERIAL_DEBUG
        Serial.print("    [SolarPV=");
        Serial.print(voltage, 2);
        Serial.print("V raw=");
        Serial.print(raw);
        Serial.println("]");
    #endif
}

void buildLightning(uint8_t *pkt) {
    // Type 0x30 — Lightning / Reserved
    //
    // STRATEGY: Use the 0x30 slot (listed as "unknown" in Vue sequence)
    // for AS3935 Franklin lightning sensor data.
    //
    // If the Davis console doesn't tolerate 0x30 data, two fallback plans:
    //   Plan B: Swap 0x30 for 0xC0 in the transmit sequence (ATK/unknown)
    //   Plan C: Add a custom hex nibble outside the standard set
    //   Testing will prove which approach works!
    //
    // Proposed encoding for lightning data:
    //   byte3 = lightning distance in km (0-63, clamped)
    //   byte4 = lightning energy (0-255, relative)
    //   byte5 = flags: bit 7 = event pending, bits 0-6 = reserved
    //
    // When no lightning sensor installed or no event pending:
    //   Send the "no sensor" / "no event" pattern
    if (lightningEvent) {
        pkt[3] = lightningDistance & 0x3F;  // 6-bit distance
        pkt[4] = lightningEnergy;
        pkt[5] = 0x80;  // Bit 7 = event flag
        lightningEvent = false;  // Clear after sending

        #if SERIAL_DEBUG
            Serial.print("    [LIGHTNING! dist=");
            Serial.print(lightningDistance);
            Serial.print("km energy=");
            Serial.print(lightningEnergy);
            Serial.println("]");
        #endif
    } else {
        // No event — send quiet pattern
        pkt[3] = 0x00;
        pkt[4] = 0x05;
        pkt[5] = 0x00;
    }
}

void buildUnknownATK(uint8_t *pkt) {
    // Type 0xC0 — Unknown / ATK / T+H stations
    // Placeholder — send no-sensor pattern
    pkt[3] = 0x00;
    pkt[4] = 0x05;
    pkt[5] = 0x00;
}

// ============================================================================
// PACKET ASSEMBLY — builds a complete 8-byte Davis packet
// ============================================================================

void buildPacket() {
    // --- Get current sequence slot ---
    uint8_t typeRaw = getSequenceType(txSeqIndex);
    uint8_t typeNibble = (typeRaw >> 4) & 0x0F;

    // --- Read common sensors (wind is in every packet) ---
    windDir.read();
    windSpeed.setWindDirection((int)windDir.getAngleDeg());
    windSpeed.read();

    // --- Byte 0: Header ---
    // Upper nibble = packet type
    // Bit 3 = battery low flag
    // Bits 0-2 = TX ID
    uint8_t id = txId.getCurrentId() & 0x07;
    uint8_t battLow = 0;
    #if !USE_TEST_STUBS
        battLow = battV.isDavisBatteryLow() ? 0x08 : 0x00;
    #endif
    packet[0] = (typeNibble << 4) | battLow | id;

    // --- Byte 1: Wind speed (always present) ---
    packet[1] = windSpeed.getSpeedMPH();

    // --- Byte 2: Wind direction (always present) ---
    packet[2] = windDir.getDavisDirectionByte();

    // --- Bytes 3-5: Sensor-specific data ---
    switch (typeNibble) {
        case PKT_TYPE_SUPERCAP:     buildSupercapBattV(packet);   break;
        case PKT_TYPE_LIGHTNING:    buildLightning(packet);       break;
        case PKT_TYPE_UV:           buildUVIndex(packet);         break;
        case PKT_TYPE_RAIN_RATE:    buildRainRate(packet);        break;
        case PKT_TYPE_SOLAR:        buildSolarRadiation(packet);  break;
        case PKT_TYPE_PV_VOLTAGE:   buildPVVoltage(packet);       break;
        case PKT_TYPE_TEMPERATURE:  buildTemperature(packet);     break;
        case PKT_TYPE_WIND_GUST:    buildWindGust(packet);        break;
        case PKT_TYPE_HUMIDITY:     buildHumidity(packet);        break;
        case PKT_TYPE_RAIN:         buildRain(packet);            break;
        case PKT_TYPE_UNKNOWN_ATK:  buildUnknownATK(packet);      break;
        default:
            // Unknown type — zero fill
            packet[3] = 0x00;
            packet[4] = 0x00;
            packet[5] = 0x00;
            break;
    }

    // --- Bytes 6-7: CRC ---
    appendCRC(packet);

    packetCount++;
}

// ============================================================================
// PERIODIC SENSOR READS
// BME280 doesn't need reading every 2.5s — read it when its packet type
// comes up in the sequence. But we read analog channels periodically for
// battery monitoring (every cycle).
// ============================================================================

bool bmeReadDue = true;

void readSensorsForType(uint8_t typeNibble) {
    switch (typeNibble) {
        case PKT_TYPE_TEMPERATURE:
        case PKT_TYPE_HUMIDITY:
            if (bmeReadDue) {
                bme.read();
                bmeReadDue = false;
            }
            break;
        case PKT_TYPE_SUPERCAP:
            battV.read();
            break;
        case PKT_TYPE_PV_VOLTAGE:
            solarV.read();
            break;
    }

    // Re-arm BME read after a full sequence cycle
    if (txSeqIndex == 0) bmeReadDue = true;
}

// ============================================================================
// DEBUG OUTPUT
// ============================================================================

#if SERIAL_DEBUG
void printPacketDebug() {
    uint8_t typeNibble = (packet[0] >> 4) & 0x0F;

    // Type name lookup
    const char* typeName = "???";
    switch (typeNibble) {
        case PKT_TYPE_SUPERCAP:    typeName = "BattV";     break;
        case PKT_TYPE_LIGHTNING:   typeName = "Lightning"; break;
        case PKT_TYPE_UV:          typeName = "UV";        break;
        case PKT_TYPE_RAIN_RATE:   typeName = "RainRate";  break;
        case PKT_TYPE_SOLAR:       typeName = "Solar";     break;
        case PKT_TYPE_PV_VOLTAGE:  typeName = "SolarPV";   break;
        case PKT_TYPE_TEMPERATURE: typeName = "Temp";      break;
        case PKT_TYPE_WIND_GUST:   typeName = "Gust";      break;
        case PKT_TYPE_HUMIDITY:    typeName = "Humid";      break;
        case PKT_TYPE_RAIN:        typeName = "Rain";      break;
        case PKT_TYPE_UNKNOWN_ATK: typeName = "ATK";       break;
    }

    Serial.print("#");
    Serial.print(packetCount);
    Serial.print(" [");
    Serial.print(txSeqIndex);
    Serial.print("/19] ");
    Serial.print(typeName);
    Serial.print("  PKT: ");

    for (int i = 0; i < 8; i++) {
        if (packet[i] < 0x10) Serial.print("0");
        Serial.print(packet[i], HEX);
        Serial.print(" ");
    }

    Serial.print(" ID=");
    Serial.print(packet[0] & 0x07);
    if (packet[0] & 0x08) Serial.print(" [BATT LOW]");
    Serial.print(" Wind=");
    Serial.print(packet[1]);
    Serial.print("mph@");
    Serial.print((int)(packet[2] * 360.0f / 255.0f));
    Serial.println("deg");
}
#endif

// ============================================================================
// SERIAL1 OUTPUT — raw 8-byte packet for iss_bridge.py
// ============================================================================

#if SERIAL1_OUTPUT
void outputPacketSerial1() {
    Serial1.write(packet, 8);
}
#endif

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("  ISS Packet Engine v2.0");
    Serial.print("  TX Sequence: ");
    Serial.println(getSequenceName());
    Serial.print("  Test Stubs:  ");
    Serial.println(USE_TEST_STUBS ? "ENABLED (no hardware)" : "DISABLED (real sensors)");
    Serial.println("==============================================");
    Serial.println();

    // --- Initialize Serial1 for bridge output ---
    #if SERIAL1_OUTPUT
        Serial1.begin(19200);
    #endif

    // --- Initialize I2C ---
    Wire.begin();

    // --- Initialize all sensors ---
    struct SensorInit {
        const char* name;
        bool (*initFunc)();
    };

    Serial.println("Initializing sensors:");

    Serial.print("  BME280............. ");
    Serial.println(bme.begin() ? "OK" : "FAIL");

    Serial.print("  Wind Speed......... ");
    Serial.println(windSpeed.begin() ? "OK" : "FAIL");

    Serial.print("  Wind Direction..... ");
    Serial.println(windDir.begin() ? "OK" : "FAIL");

    #if !USE_TEST_STUBS
        solarV.setName("Solar_V");
        battV.setName("Battery_V");
        battTherm.setName("BattTherm");
        ldrLight.setName("LDR");

        battV.setBatteryLowThreshold(3.3f);
        battTherm.setNTCParams(10000.0f, 10000.0f, 298.15f, 3950.0f);

        Serial.print("  Solar Voltage...... ");
        Serial.println(solarV.begin() ? "OK" : "FAIL");

        Serial.print("  Battery Voltage.... ");
        Serial.println(battV.begin() ? "OK" : "FAIL");

        Serial.print("  Battery Therm...... ");
        Serial.println(battTherm.begin() ? "OK" : "FAIL");

        Serial.print("  LDR Light.......... ");
        Serial.println(ldrLight.begin() ? "OK" : "FAIL");
    #endif

    txId.begin();
    Serial.print("  TX ID Manager...... OK  (ID=");
    Serial.print(txId.getCurrentId());
    Serial.println(")");

    #if USE_TEST_STUBS
        // Load some realistic test values
        bme.setValues(76.4f, 45.0f, 1013.25f);
        windSpeed.setValues(8, 12, 20);
        windDir.setAngle(228.0f);
        solarV.setVoltage(5.1f);
        battV.setVoltage(3.85f);
        Serial.println("\n  Stub values loaded:");
        Serial.println("    Temp=76.4F  RH=45%  Baro=1013.25hPa");
        Serial.println("    Wind=8mph@228deg  Gust=12mph");
        Serial.println("    BattV=3.85V  SolarV=5.10V");
    #endif

    // --- Read battery voltage for initial battery-low flag ---
    battV.read();

    Serial.println();
    Serial.println("Packet engine running. Interval: 2.5s");
    Serial.print("Transmit sequence (");
    Serial.print(getSequenceName());
    Serial.println("):");

    // Print the active sequence
    for (int i = 0; i < TX_SEQ_LENGTH; i++) {
        uint8_t t = getSequenceType(i);
        uint8_t tn = (t >> 4) & 0x0F;
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] 0x");
        Serial.print(t, HEX);
        Serial.print(" = ");
        switch (tn) {
            case PKT_TYPE_SUPERCAP:    Serial.print("BattV");     break;
            case PKT_TYPE_LIGHTNING:   Serial.print("Lightning*"); break;
            case PKT_TYPE_UV:          Serial.print("UV");        break;
            case PKT_TYPE_RAIN_RATE:   Serial.print("RainRate");  break;
            case PKT_TYPE_SOLAR:       Serial.print("Solar");     break;
            case PKT_TYPE_PV_VOLTAGE:  Serial.print("SolarPV");   break;
            case PKT_TYPE_TEMPERATURE: Serial.print("Temp");      break;
            case PKT_TYPE_WIND_GUST:   Serial.print("Gust");      break;
            case PKT_TYPE_HUMIDITY:    Serial.print("Humid");      break;
            case PKT_TYPE_RAIN:        Serial.print("Rain");      break;
            default:                   Serial.print("?");          break;
        }
        if ((i + 1) % 4 == 0) Serial.println();
        else Serial.print("  ");
    }

    Serial.println();
    Serial.println("----------------------------------------------");
    Serial.println();

    lastPacketTime = millis();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    uint32_t now = millis();

    // --- TX ID button management (non-blocking) ---
    txId.update();
    #if !USE_TEST_STUBS
        if (txId.wasButtonPressed()) {
            Serial.print("  [TX ID changed to ");
            Serial.print(txId.getCurrentId());
            Serial.println("]");
        }
    #endif

    // --- Rain rate timer update ---
    if (lastRainTipTime > 0) {
        rainRateSeconds = (uint16_t)((now - lastRainTipTime) / 1000UL);
    }

    // --- Packet generation on timer ---
    if ((now - lastPacketTime) >= PACKET_INTERVAL_MS) {
        lastPacketTime = now;

        // Pre-read sensors needed for this slot
        uint8_t typeRaw = getSequenceType(txSeqIndex);
        uint8_t typeNibble = (typeRaw >> 4) & 0x0F;
        readSensorsForType(typeNibble);

        // Build and output the packet
        buildPacket();

        #if SERIAL_DEBUG
            printPacketDebug();
        #endif

        #if SERIAL1_OUTPUT
            outputPacketSerial1();
        #endif

        // Advance sequence index (wraps 0-19)
        txSeqIndex = (txSeqIndex + 1) % TX_SEQ_LENGTH;
    }
}
