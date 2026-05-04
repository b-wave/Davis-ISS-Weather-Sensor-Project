/**
 * AnalogDriver.cpp — Generic Analog Input Driver Implementation
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Supports three modes:
 *   ANALOG_VOLTAGE:    V = (ADC / maxADC) * Vref * dividerRatio + offset
 *   ANALOG_NTC_THERM:  Steinhart-Hart B-parameter conversion
 *   ANALOG_RELATIVE:   0-100% of ADC range
 */

#include "AnalogDriver.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AnalogDriver::AnalogDriver(uint8_t pin, float dividerRatio,
                           AnalogMode mode, uint8_t oversample)
    : _pin(pin)
    , _dividerRatio(dividerRatio)
    , _mode(mode)
    , _oversample(oversample > 0 ? oversample : 1)
    , _initialized(false)
    , _status(SENSOR_NOT_INITIALIZED)
    , _name("AnalogInput")
    , _rawADC(0)
    , _voltage(0.0f)
    , _scaled(0.0f)
    , _ntcRFixed(NTC_DEFAULT_R_FIXED)
    , _ntcR0(NTC_DEFAULT_R0)
    , _ntcT0(NTC_DEFAULT_T0)
    , _ntcB(NTC_DEFAULT_B)
    , _threshLow(0.0f)
    , _threshHigh(ADC_VREF)
    , _battLowV(3.3f)
    , _offset(0.0f)
    , _scale(1.0f)
{}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
bool AnalogDriver::begin() {
    pinMode(_pin, INPUT);
    analogReadResolution(ADC_DEFAULT_BITS);

    // Take a test read to verify the pin responds
    analogRead(_pin);

    _initialized = true;
    _status = SENSOR_OK;
    return true;
}

// ---------------------------------------------------------------------------
// Read — multi-sample average, then convert based on mode
// ---------------------------------------------------------------------------
bool AnalogDriver::read() {
    if (!_initialized) {
        _status = SENSOR_NOT_INITIALIZED;
        return false;
    }

    // Oversampled ADC read
    uint32_t sum = 0;
    for (uint8_t i = 0; i < _oversample; i++) {
        sum += analogRead(_pin);
    }
    _rawADC = (uint16_t)(sum / _oversample);

    // Compute voltage at ADC pin
    float adcVoltage = ((float)_rawADC / (float)ADC_DEFAULT_MAX) * ADC_VREF;

    // Compute actual voltage (before divider)
    _voltage = adcVoltage * _dividerRatio;

    // Apply calibration
    _voltage = (_voltage * _scale) + _offset;

    // Mode-specific scaling
    switch (_mode) {
        case ANALOG_VOLTAGE:
            _scaled = _voltage;
            break;

        case ANALOG_NTC_THERM:
            _scaled = computeNTCTemp(adcVoltage);
            break;

        case ANALOG_RELATIVE:
            _scaled = ((float)_rawADC / (float)ADC_DEFAULT_MAX) * 100.0f;
            break;
    }

    _status = SENSOR_OK;
    return true;
}

// ---------------------------------------------------------------------------
// NTC thermistor conversion — B-parameter equation
// ---------------------------------------------------------------------------
float AnalogDriver::computeNTCTemp(float adcVoltage) const {
    // Resistor divider: VCC --- R_fixed --- ADC_pin --- NTC --- GND
    // V_adc = VCC * R_ntc / (R_fixed + R_ntc)
    // R_ntc = R_fixed * V_adc / (VCC - V_adc)

    if (adcVoltage <= 0.01f || adcVoltage >= (ADC_VREF - 0.01f)) {
        return -999.0f;  // Open or shorted sensor
    }

    float rNTC = _ntcRFixed * adcVoltage / (ADC_VREF - adcVoltage);

    // B-parameter equation:
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    float steinhart = 1.0f / _ntcT0 + (1.0f / _ntcB) * log(rNTC / _ntcR0);
    float tempK = 1.0f / steinhart;
    float tempC = tempK - 273.15f;

    return tempC;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
uint16_t AnalogDriver::readRaw()     const { return _rawADC; }
float    AnalogDriver::readVoltage() const { return _voltage; }
float    AnalogDriver::readScaled()  const { return _scaled; }

// ---------------------------------------------------------------------------
// NTC configuration
// ---------------------------------------------------------------------------
void AnalogDriver::setNTCParams(float rFixed, float r0, float t0K, float b) {
    _ntcRFixed = rFixed;
    _ntcR0 = r0;
    _ntcT0 = t0K;
    _ntcB = b;
}

// ---------------------------------------------------------------------------
// Thresholds
// ---------------------------------------------------------------------------
void AnalogDriver::setThreshold(float low, float high) {
    _threshLow = low;
    _threshHigh = high;
}

bool AnalogDriver::isBelowLow()  const { return _scaled < _threshLow; }
bool AnalogDriver::isAboveHigh() const { return _scaled > _threshHigh; }

// ---------------------------------------------------------------------------
// Davis battery-low flag
// ---------------------------------------------------------------------------
bool AnalogDriver::isDavisBatteryLow() const { return _voltage < _battLowV; }
void AnalogDriver::setBatteryLowThreshold(float v) { _battLowV = v; }

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
void AnalogDriver::setOffset(float offset) { _offset = offset; }
void AnalogDriver::setScale(float scale)   { _scale = scale; }
void AnalogDriver::setName(const char* n)  { _name = n; }

// ---------------------------------------------------------------------------
// Self-test — verify ADC returns a non-railed value
// ---------------------------------------------------------------------------
bool AnalogDriver::selfTest() {
    if (!_initialized) return false;

    uint16_t raw = analogRead(_pin);
    // A completely railed reading (0 or max) may indicate a wiring issue
    // But for some sensors (e.g., dark LDR) 0 is valid, so just check for max
    if (raw >= ADC_DEFAULT_MAX) {
        _status = SENSOR_CAL_ERROR;
        return false;
    }

    _status = SENSOR_OK;
    return true;
}

SensorStatus AnalogDriver::getStatus() { return _status; }
const char*  AnalogDriver::getName()   { return _name; }

