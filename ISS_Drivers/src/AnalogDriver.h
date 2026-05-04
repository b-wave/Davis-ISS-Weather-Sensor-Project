/**
 * AnalogDriver.h — Generic Analog Input Driver with Calibration
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Handles all analog-input sensors through a single configurable class:
 *   - Solar cell voltage (resistor divider)
 *   - Battery voltage (resistor divider)
 *   - Battery thermistor (NTC, Steinhart-Hart / B-parameter)
 *   - LDR luminosity (relative light level)
 *   - UV sensor (future, if analog photodiode chosen)
 *
 * Features:
 *   - Configurable resistor divider ratio
 *   - Multi-sample oversampling for noise reduction
 *   - Optional NTC thermistor mode with B-parameter conversion
 *   - Configurable ADC resolution (10-bit default, 12-bit on Teensy 4.x)
 */

#ifndef ANALOG_DRIVER_H
#define ANALOG_DRIVER_H

#include "SensorDriver.h"

// ADC configuration for Teensy 4.x
#define ADC_DEFAULT_BITS       10
#define ADC_DEFAULT_MAX        1023
#define ADC_VREF               3.3f

// NTC thermistor defaults (10K NTC, B=3950)
#define NTC_DEFAULT_R_FIXED    10000.0f   // Fixed resistor in divider (ohms)
#define NTC_DEFAULT_R0         10000.0f   // NTC resistance at T0 (ohms)
#define NTC_DEFAULT_T0         298.15f    // T0 in Kelvin (25°C)
#define NTC_DEFAULT_B          3950.0f    // B-parameter

enum AnalogMode {
    ANALOG_VOLTAGE,      // Simple voltage via resistor divider
    ANALOG_NTC_THERM,    // NTC thermistor with B-parameter conversion
    ANALOG_RELATIVE      // Relative value 0-100% (for LDR, etc.)
};

class AnalogDriver : public SensorDriver {
public:
    /**
     * @param pin            Analog input pin
     * @param dividerRatio   Voltage divider ratio (e.g., 2.0 for 2:1)
     * @param mode           Interpretation mode
     * @param oversample     Number of ADC reads to average (default 8)
     */
    AnalogDriver(uint8_t pin, float dividerRatio = 1.0f,
                 AnalogMode mode = ANALOG_VOLTAGE, uint8_t oversample = 8);

    bool begin() override;
    bool read() override;
    bool selfTest() override;
    SensorStatus getStatus() override;
    const char* getName() override;

    // --- Value accessors ---

    /** Raw averaged ADC value (0-1023 at 10-bit) */
    uint16_t readRaw() const;

    /** Actual voltage after divider compensation */
    float readVoltage() const;

    /**
     * Application-specific scaled value:
     *   ANALOG_VOLTAGE:   Voltage in volts
     *   ANALOG_NTC_THERM: Temperature in °C
     *   ANALOG_RELATIVE:  Percentage 0-100%
     */
    float readScaled() const;

    // --- NTC thermistor configuration ---
    void setNTCParams(float rFixed, float r0, float t0Kelvin, float bParam);

    // --- Threshold support (for battery-low flag, day/night, etc.) ---
    void setThreshold(float lowThreshold, float highThreshold);
    bool isBelowLow() const;
    bool isAboveHigh() const;

    // --- Calibration ---
    void setOffset(float offset);
    void setScale(float scale);

    // --- Davis battery-low flag helper ---
    /** Returns true if battery voltage is below threshold */
    bool isDavisBatteryLow() const;

    /** Set the battery-low voltage threshold */
    void setBatteryLowThreshold(float volts);

    void setName(const char* name);

private:
    uint8_t    _pin;
    float      _dividerRatio;
    AnalogMode _mode;
    uint8_t    _oversample;
    bool       _initialized;
    SensorStatus _status;
    const char* _name;

    // Last readings
    uint16_t _rawADC;
    float    _voltage;
    float    _scaled;

    // NTC parameters
    float _ntcRFixed;
    float _ntcR0;
    float _ntcT0;
    float _ntcB;

    // Thresholds
    float _threshLow;
    float _threshHigh;
    float _battLowV;

    // Calibration
    float _offset;
    float _scale;

    // Internal
    float computeNTCTemp(float voltage) const;
};

// =============================================================================
// Test stub
// =============================================================================
class AnalogStub : public SensorDriver {
public:
    AnalogStub(uint8_t pin) : _pin(pin), _voltage(3.3f), _raw(512), _scaled(50.0f) {}

    void setValues(uint16_t raw, float voltage, float scaled) {
        _raw = raw; _voltage = voltage; _scaled = scaled;
    }
    void setVoltage(float v) { _voltage = v; _raw = (uint16_t)(v / 3.3f * 1023); _scaled = v; }

    bool begin() override { return true; }
    bool read() override { return true; }
    bool selfTest() override { return true; }
    SensorStatus getStatus() override { return SENSOR_OK; }
    const char* getName() override { return "Analog_STUB"; }

    uint16_t readRaw()    const { return _raw; }
    float    readVoltage() const { return _voltage; }
    float    readScaled()  const { return _scaled; }
    bool     isDavisBatteryLow() const { return _voltage < 3.3f; }

private:
    uint8_t  _pin;
    float    _voltage;
    uint16_t _raw;
    float    _scaled;
};

#endif // ANALOG_DRIVER_H

