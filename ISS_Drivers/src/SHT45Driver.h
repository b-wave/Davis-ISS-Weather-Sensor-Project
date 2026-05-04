// ============================================================================
// SHT45Driver.h — Sensirion SHT45 Temperature/Humidity Driver with Heater
// ============================================================================
// REV: 050226
// VERS: 1.0
//
// Drop-in upgrade for BME280Driver in the ISS Packet Engine.
// Provides temperature, humidity, and managed heater control for
// condensation protection during field deployment.
//
// Features:
//   - SHT45-AD1F via I2C (address 0x44)
//   - 3-level heater (low/med/high) with power-aware scheduling
//   - Configurable RH thresholds for automatic heater escalation
//   - Supercap voltage gating — heater disabled when power is marginal
//   - Software-configurable heater pulse interval
//
// I2C Bus Coexistence:
//   AS5048B  @ 0x40   (wind direction)
//   SHT45    @ 0x44   (this driver)
//   BME280   @ 0x76   (dev phase, removed for field)
//
// Davis Packet Integration:
//   - Temp  → packet type 0x8:  int16 = temp_F * 160
//   - Humid → packet type 0xA:  encoded per Davis spec
//
// ============================================================================

#ifndef SHT45_DRIVER_H
#define SHT45_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// SHT45 I2C Commands (from Sensirion datasheet)
// ============================================================================

#define SHT45_I2C_ADDR        0x44

// Measurement commands (with clock stretching disabled)
#define SHT45_CMD_MEAS_HIGH   0xFD    // High repeatability,   8.2ms max
#define SHT45_CMD_MEAS_MED    0xF6    // Medium repeatability, 4.5ms max
#define SHT45_CMD_MEAS_LOW    0xE0    // Low repeatability,    1.3ms max

// Heater commands: {power}_{duration}
// Each command triggers a measurement AFTER the heater pulse completes,
// so you get fresh temp/RH data that reflects post-heater conditions.
#define SHT45_CMD_HEAT_H_1S   0x39    // High power  (~200mW), 1.0s pulse
#define SHT45_CMD_HEAT_H_01S  0x32    // High power  (~200mW), 0.1s pulse
#define SHT45_CMD_HEAT_M_1S   0x2F    // Medium power (~20mW), 1.0s pulse
#define SHT45_CMD_HEAT_M_01S  0x24    // Medium power (~20mW), 0.1s pulse
#define SHT45_CMD_HEAT_L_1S   0x1E    // Low power    (~2mW),  1.0s pulse
#define SHT45_CMD_HEAT_L_01S  0x15    // Low power    (~2mW),  0.1s pulse

// Other commands
#define SHT45_CMD_SERIAL      0x89    // Read 32-bit serial number
#define SHT45_CMD_RESET       0x94    // Soft reset (1ms recovery)

// ============================================================================
// Heater power levels
// ============================================================================

enum SHT45HeaterLevel : uint8_t {
    HEATER_OFF    = 0,
    HEATER_LOW    = 1,    // ~2mW   — routine condensation prevention
    HEATER_MEDIUM = 2,    // ~20mW  — morning dew burn-off
    HEATER_HIGH   = 3     // ~200mW — heavy condensation recovery
};

// ============================================================================
// SHT45Driver — Temperature/Humidity with managed heater
// ============================================================================

class SHT45Driver {
public:
    // ---- Construction & Init ----
    SHT45Driver();
    bool begin(TwoWire &wire = Wire);
    void reset();
    uint32_t getSerialNumber();

    // ---- Measurement ----
    bool     measure();                   // Triggers measurement, blocks ~10ms
    float    getTemperatureC();           // Last reading in °C
    float    getTemperatureF();           // Last reading in °F
    float    getHumidity();              // Last reading in %RH (clamped 0-100)
    int16_t  getDavisTempRaw();          // temp_F * 160, ready for packet byte 3-4
    uint16_t getDavisHumidRaw();         // Encoded per Davis spec for packet

    // ---- Heater Control ----
    bool     runHeaterPulse(SHT45HeaterLevel level, bool longPulse = false);
    // Triggers heater + measurement in one I2C command.
    // longPulse=false → 0.1s,  longPulse=true → 1.0s
    // Returns true if heater ran and measurement succeeded.
    // NOTE: blocks for the heater duration (100ms or 1100ms).

    SHT45HeaterLevel getLastHeaterLevel();
    uint32_t         getLastHeaterTime();     // millis() of last heater pulse

    // ---- Status ----
    bool     isConnected();
    bool     isValid();                  // True if last measurement succeeded
    uint32_t getLastMeasureTime();       // millis() of last successful read

private:
    TwoWire* _wire;
    float    _tempC;
    float    _humidity;
    bool     _valid;
    uint32_t _lastMeasureMs;
    uint32_t _lastHeaterMs;
    SHT45HeaterLevel _lastHeaterLevel;

    bool     sendCommand(uint8_t cmd);
    bool     readResponse(uint8_t* buf, uint8_t len);
    uint8_t  crc8(const uint8_t* data, uint8_t len);
};


// ============================================================================
// HeaterManager — Power-aware automatic heater scheduling
// ============================================================================
//
// Usage:
//   Call evaluate() once per TX cycle (~2.6s). It decides whether to fire
//   the heater based on current RH, supercap voltage, and elapsed time
//   since last pulse. If a heater pulse is needed, it calls
//   SHT45Driver::runHeaterPulse() internally.
//
// Power strategy:
//   - Low pulse every heaterIntervalMs (~26s, ~10 TX cycles)
//   - Escalate to medium when RH > rhThresholdMed (default 95%)
//   - Escalate to high when RH > rhThresholdHigh (default 99%)
//   - Disable heater entirely when supercap < voltageMinimum (default 3.0V)
//
// ============================================================================

class HeaterManager {
public:
    HeaterManager();

    void begin(SHT45Driver* sensor);

    // Call once per TX cycle with current readings
    // Returns true if a heater pulse was fired this cycle
    bool evaluate(float currentRH, float supercapVoltage);

    // ---- Configuration ----
    void     setHeaterInterval(uint32_t ms);       // Default: 26000ms (~10 TX cycles)
    void     setRHThresholdMed(float rh);          // Default: 95.0%
    void     setRHThresholdHigh(float rh);         // Default: 99.0%
    void     setVoltageMinimum(float volts);       // Default: 3.0V
    void     setLongPulse(bool useLong);           // Default: false (0.1s pulses)

    // ---- Status ----
    bool              isHeaterEnabled();            // False when voltage too low
    SHT45HeaterLevel  getHeaterLevel();
    uint32_t          getLastHeaterTime();
    uint32_t          getPulseCount();              // Total heater activations
    float             getMinVoltage();              // Lowest voltage seen

private:
    SHT45Driver*     _sensor;
    SHT45HeaterLevel _level;
    bool             _enabled;
    bool             _longPulse;
    uint32_t         _lastHeaterMs;
    uint32_t         _heaterIntervalMs;
    uint32_t         _pulseCount;
    float            _rhThresholdMed;
    float            _rhThresholdHigh;
    float            _voltageMinimum;
    float            _minVoltage;
};

#endif // SHT45_DRIVER_H
