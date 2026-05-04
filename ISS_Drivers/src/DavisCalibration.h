/**
 * DavisCalibration.h — Davis OEM Wind Calibration Table & Transmit Sequences
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Wind calibration data: Davis publishes OEM error-correction values for
 * anemometer installations because wind affects both sides of the cup
 * assembly. The correction varies by wind speed AND direction relative
 * to the sensor body. This is especially important at higher velocities
 * common in desert locations.
 *
 * Table structure: 27 rows (20-150 mph in 5 mph steps) x 3 columns
 * for perpendicular reference angles (0°, 90°/270°, 180°).
 * Bilinear interpolation is used for intermediate speeds and angles.
 *
 * Transmit sequences: Observed packet type rotation for VP2 and Vue ISS
 * units (20 slots per cycle). Wind speed and direction are in EVERY packet.
 *
 * Reference: VPTools by kobuki (github.com/kobuki/VPTools), Davis OEM docs
 */

#ifndef DAVIS_CALIBRATION_H
#define DAVIS_CALIBRATION_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Packet type nibbles (upper nibble of byte 0)
// ---------------------------------------------------------------------------
#define PKT_TYPE_SUPERCAP     0x2   // 0x20 — Supercap / Battery voltage (Vue)
#define PKT_TYPE_LIGHTNING    0x3   // 0x30 — Unknown/reserved → Lightning (future)
#define PKT_TYPE_UV           0x4   // 0x40 — UV Index
#define PKT_TYPE_RAIN_RATE    0x5   // 0x50 — Rain Rate (seconds since last tip)
#define PKT_TYPE_SOLAR        0x6   // 0x60 — Solar Radiation (W/m²)
#define PKT_TYPE_PV_VOLTAGE   0x7   // 0x70 — PV (Solar Cell) Voltage (Vue)
#define PKT_TYPE_TEMPERATURE  0x8   // 0x80 — Temperature
#define PKT_TYPE_WIND_GUST    0x9   // 0x90 — Wind Gust
#define PKT_TYPE_HUMIDITY     0xA   // 0xA0 — Humidity
#define PKT_TYPE_RAIN         0xE   // 0xE0 — Rain bucket tips
#define PKT_TYPE_UNKNOWN_ATK  0xC   // 0xC0 — Unknown (ATK/T+H stations)

// ---------------------------------------------------------------------------
// Davis wind speed constants
// ---------------------------------------------------------------------------
#define WIND_RPS_MPH          2.25f    // RPS-to-MPH constant published by Davis
#define WIND_DEBOUNCE_US      5000UL   // 5 ms debounce threshold (micros)
#define WIND_TIMEOUT_US       2250000UL // 2.25 s — no pulse = 0 mph
#define WIND_MIN_PULSE_COUNT  10       // below this, use single-period algorithm

// ---------------------------------------------------------------------------
// Davis OEM wind error-correction table
// Published by Davis for OEM anemometer installations.
// 27 rows: actual wind 20-150 mph (5 mph increments)
// 3 columns: raw reading at 0° (head-on), 90°/270° (crosswind), 180° (tail)
//
// The raw reading is what the anemometer reports; the row index gives
// the "true" wind speed. Wind blowing from behind the cups reads low,
// wind head-on reads high.
// ---------------------------------------------------------------------------
#define WIND_CAL_ROWS    27
#define WIND_CAL_COLS    3
#define WIND_CAL_MIN_MPH 20
#define WIND_CAL_STEP    5

static const float WIND_CAL_TABLE[WIND_CAL_ROWS][WIND_CAL_COLS] PROGMEM = {
    //   0°      90/270°    180°
    {  23.3f,   17.8f,   16.4f },  //  20 mph actual
    {  28.5f,   22.3f,   20.4f },  //  25
    {  33.8f,   27.1f,   25.3f },  //  30
    {  39.2f,   31.6f,   29.8f },  //  35
    {  44.5f,   35.9f,   34.3f },  //  40
    {  49.7f,   41.2f,   40.5f },  //  45
    {  55.0f,   45.5f,   45.1f },  //  50
    {  60.3f,   50.2f,   49.8f },  //  55
    {  65.7f,   54.7f,   54.1f },  //  60
    {  70.8f,   59.0f,   59.0f },  //  65
    {  76.2f,   64.4f,   63.9f },  //  70
    {  81.4f,   69.0f,   68.2f },  //  75
    {  86.8f,   73.6f,   73.1f },  //  80
    {  92.1f,   77.6f,   78.2f },  //  85
    {  97.4f,   82.0f,   83.2f },  //  90
    { 102.5f,   86.9f,   87.5f },  //  95
    { 107.7f,   92.1f,   92.8f },  // 100
    { 113.2f,   96.9f,   97.3f },  // 105
    { 118.5f,  101.5f,  102.3f },  // 110
    { 123.9f,  106.2f,  106.5f },  // 115
    { 129.5f,  110.6f,  111.0f },  // 120
    { 135.0f,  115.4f,  115.3f },  // 125
    { 139.8f,  120.3f,  119.7f },  // 130
    { 144.8f,  125.0f,  124.0f },  // 135
    { 149.3f,  129.8f,  128.7f },  // 140
    { 154.5f,  134.1f,  134.5f },  // 145
    { 159.8f,  137.9f,  138.0f }   // 150
};

// ---------------------------------------------------------------------------
// Helper: read a float from the PROGMEM calibration table
// ---------------------------------------------------------------------------
static inline float readCalTable(uint8_t row, uint8_t col) {
    float val;
    memcpy_P(&val, &WIND_CAL_TABLE[row][col], sizeof(float));
    return val;
}

// ---------------------------------------------------------------------------
// VP2 transmit sequence (20 slots, observed from real hardware)
// Upper nibble = packet type, lower nibble = TX ID | battery flag
//
// Type mapping:
//   0x80 = Temperature         0xE0 = Rain
//   0x50 = Rain Rate (seconds) 0x40 = UV Index
//   0x90 = Wind Gust           0xA0 = Humidity
//   0x60 = Solar Radiation     0xC0 = Unknown (ATK/T+H stations)
//
// Wind speed (byte 1) and direction (byte 2) are in EVERY packet.
// ---------------------------------------------------------------------------
#define TX_SEQ_LENGTH 20

static const uint8_t TX_SEQ_VP2[TX_SEQ_LENGTH] PROGMEM = {
    0x80, 0xE0, 0x50, 0x40,
    0x80, 0xE0, 0x50, 0x90,
    0x80, 0xE0, 0x50, 0xA0,
    0x80, 0xE0, 0x50, 0xA0,
    0x80, 0xE0, 0x50, 0x60
};

// ---------------------------------------------------------------------------
// Vue transmit sequence (20 slots)
//
// Type mapping:
//   0x80 = Temperature    0xE0 = Rain
//   0x20 = Supercap       0xA0 = Humidity  (battery voltage on our ISS)
//   0x70 = PV Voltage     0x50 = Rain Rate (seconds)
//   0x90 = Wind Gust      0x30 = Unknown   (lightning sensor, future)
// ---------------------------------------------------------------------------
static const uint8_t TX_SEQ_VUE[TX_SEQ_LENGTH] PROGMEM = {
    0x80, 0x50, 0xE0, 0x20,
    0x80, 0x50, 0xE0, 0x70,
    0x80, 0x50, 0xE0, 0x30,
    0x80, 0x50, 0xE0, 0x90,
    0x80, 0x50, 0xE0, 0xA0
};

// ---------------------------------------------------------------------------
// Custom ISS transmit sequence (20 slots)
//
// Hybrid VP2 + Vue: includes battery (0x20), solar PV (0x70), and
// a 0x30 slot reserved for lightning (AS3935).
// If the console chokes on 0x30, swap it for 0xC0 (ATK/unknown)
// and test adding a custom nibble for lightning separately.
// ---------------------------------------------------------------------------
static const uint8_t TX_SEQ_CUSTOM[TX_SEQ_LENGTH] PROGMEM = {
    0x80, 0xE0, 0x50, 0x20,      // temp, rain, rainRate, battV
    0x80, 0xE0, 0x50, 0x70,      // temp, rain, rainRate, solarPV
    0x80, 0xE0, 0x50, 0x30,      // temp, rain, rainRate, lightning*
    0x80, 0xE0, 0x50, 0x90,      // temp, rain, rainRate, gust
    0x80, 0xE0, 0x50, 0xA0       // temp, rain, rainRate, humidity
};

// ---------------------------------------------------------------------------
// "No sensor" packet byte patterns for VP2 (bytes 3, 4, 5)
// Used when a sensor is not present — prevents the console from
// displaying stale or garbage data.
// ---------------------------------------------------------------------------
struct NoSensorPattern {
    uint8_t typeNibble;     // upper nibble of byte 0
    uint8_t byte3;
    uint8_t byte4;
    uint8_t byte5;
    const char* name;
};

static const NoSensorPattern NO_SENSOR_VP2[] = {
    { 0x20, 0x00, 0x05, 0x00, "Supercap/BattV" },  // Vue battery slot
    { 0x30, 0x00, 0x05, 0x00, "Lightning/Rsv" },    // Reserved for AS3935
    { 0x40, 0xFF, 0xC5, 0x00, "UV Index" },
    { 0x50, 0xFF, 0x71, 0x00, "Rain Rate" },
    { 0x60, 0xFF, 0xC5, 0x00, "Solar Radiation" },
    { 0x70, 0x00, 0x05, 0x00, "PV Voltage" },       // Vue solar cell slot
    { 0x80, 0xFF, 0xC1, 0x00, "Temperature" },
    { 0x90, 0x00, 0x05, 0x00, "Wind Gust" },
    { 0xA0, 0x00, 0x01, 0x00, "Humidity" },
    { 0xC0, 0x00, 0x05, 0x00, "Unknown/ATK" },
    { 0xE0, 0x80, 0x01, 0x00, "Rain" },
};
#define NO_SENSOR_COUNT (sizeof(NO_SENSOR_VP2) / sizeof(NO_SENSOR_VP2[0]))

#endif // DAVIS_CALIBRATION_H
