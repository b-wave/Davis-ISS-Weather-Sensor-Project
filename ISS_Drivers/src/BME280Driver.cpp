/**
 * BME280Driver.cpp — Bosch BME280 Temperature/Humidity/Pressure Implementation
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Uses forced mode: the sensor takes one measurement and returns to sleep.
 * This minimizes self-heating and power consumption. The ISS only needs
 * readings every ~10 seconds (temperature packet interval).
 *
 * Requires: Adafruit_BME280 library
 */

#include "BME280Driver.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
BME280Driver::BME280Driver(uint8_t addr, TwoWire* wire, float tempOffsetF)
    : _wire(wire)
    , _addr(addr)
    , _tempOffsetF(tempOffsetF)
    , _initialized(false)
    , _status(SENSOR_NOT_INITIALIZED)
    , _tempC(0.0f)
    , _tempF(0.0f)
    , _humidity(0.0f)
    , _pressure(0.0f)
{}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
bool BME280Driver::begin() {
    if (!_bme.begin(_addr, _wire)) {
        _status = SENSOR_NOT_FOUND;
        return false;
    }

    // Configure for weather station: forced mode, minimal oversampling
    // This minimizes power and self-heating
    _bme.setSampling(
        Adafruit_BME280::MODE_FORCED,     // Take one reading, then sleep
        Adafruit_BME280::SAMPLING_X1,     // Temperature: 1x oversampling
        Adafruit_BME280::SAMPLING_X1,     // Pressure: 1x oversampling
        Adafruit_BME280::SAMPLING_X1,     // Humidity: 1x oversampling
        Adafruit_BME280::FILTER_OFF,      // No IIR filter (we sample infrequently)
        Adafruit_BME280::STANDBY_MS_1000  // Standby (not used in forced mode)
    );

    _initialized = true;
    _status = SENSOR_OK;
    return true;
}

// ---------------------------------------------------------------------------
// Read — trigger forced measurement and retrieve all three values
// ---------------------------------------------------------------------------
bool BME280Driver::read() {
    if (!_initialized) {
        _status = SENSOR_NOT_INITIALIZED;
        return false;
    }

    // In forced mode, must call takeForcedMeasurement() before reading
    if (!_bme.takeForcedMeasurement()) {
        _status = SENSOR_READ_ERROR;
        return false;
    }

    _tempC    = _bme.readTemperature();          // Celsius
    _humidity = _bme.readHumidity();             // %RH
    _pressure = _bme.readPressure() / 100.0f;   // Pa → hPa

    // Apply calibration offset (in Fahrenheit)
    _tempF = (_tempC * 9.0f / 5.0f + 32.0f) + _tempOffsetF;

    // Sanity check — BME280 returns NaN on bus error
    if (isnan(_tempC) || isnan(_humidity) || isnan(_pressure)) {
        _status = SENSOR_READ_ERROR;
        return false;
    }

    // Range check
    if (_tempC < -40.0f || _tempC > 85.0f ||
        _humidity < 0.0f || _humidity > 100.0f ||
        _pressure < 300.0f || _pressure > 1100.0f) {
        _status = SENSOR_CAL_ERROR;
        return false;
    }

    _status = SENSOR_OK;
    return true;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
float BME280Driver::getTemperatureF()  const { return _tempF; }
float BME280Driver::getTemperatureC()  const { return _tempC; }
float BME280Driver::getHumidityPct()   const { return _humidity; }
float BME280Driver::getPressureHPa()   const { return _pressure; }

// ---------------------------------------------------------------------------
// Davis packet encoding
// ---------------------------------------------------------------------------
int16_t BME280Driver::getDavisTemperatureRaw() const {
    // Davis ISS: temp_F = raw / 160.0
    // So: raw = temp_F * 160.0
    return (int16_t)(_tempF * 160.0f);
}

uint16_t BME280Driver::getDavisHumidityRaw() const {
    // Davis ISS: humidity = raw / 10.0
    // So: raw = humidity * 10.0
    return (uint16_t)(_humidity * 10.0f);
}

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
void  BME280Driver::setTempOffset(float offsetF) { _tempOffsetF = offsetF; }
float BME280Driver::getTempOffset() const        { return _tempOffsetF; }

// ---------------------------------------------------------------------------
// Self-test — verify chip ID
// ---------------------------------------------------------------------------
bool BME280Driver::selfTest() {
    if (!_initialized) return false;

    // The BME280 chip ID should be 0x60
    // Adafruit library checks this in begin(), but we can re-read
    uint8_t chipId = _bme.sensorID();
    if (chipId != 0x60) {
        _status = SENSOR_NOT_FOUND;
        return false;
    }

    // Take a test reading
    if (!read()) return false;

    _status = SENSOR_OK;
    return true;
}

SensorStatus BME280Driver::getStatus() { return _status; }
const char*  BME280Driver::getName()   { return "BME280"; }

