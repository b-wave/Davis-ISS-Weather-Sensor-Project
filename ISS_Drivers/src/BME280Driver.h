/**
 * BME280Driver.h — Bosch BME280 Temperature/Humidity/Pressure Driver
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Reads temperature (°F), humidity (%RH), and barometric pressure (hPa)
 * via I2C. Uses forced mode for low-power weather station operation.
 *
 * Davis packet encoding helpers included:
 *   Temperature → packet type 0x8: raw = (int16_t)(temp_F * 160.0)
 *   Humidity    → packet type 0xA: raw = (uint16_t)(humidity * 10.0)
 *
 * Pressure is used internally (storm detection for AS3935 power mgmt)
 * and is NOT transmitted in ISS packets — the VP2 console has its own
 * barometer.
 *
 * Requires: Adafruit_BME280 library (or compatible)
 */

#ifndef BME280_DRIVER_H
#define BME280_DRIVER_H

#include "SensorDriver.h"
#include <Wire.h>
#include <Adafruit_BME280.h>

class BME280Driver : public SensorDriver {
public:
    /**
     * @param addr     I2C address (0x76 default, 0x77 if SDO→VCC)
     * @param wire     TwoWire instance (Wire, Wire1, etc.)
     * @param tempOffsetF  Fixed temperature offset in °F for calibration
     */
    BME280Driver(uint8_t addr = 0x76, TwoWire* wire = &Wire,
                 float tempOffsetF = 0.0f);

    bool begin() override;
    bool read() override;
    bool selfTest() override;
    SensorStatus getStatus() override;
    const char* getName() override;

    // --- Weather data accessors ---

    /** Temperature in Fahrenheit */
    float getTemperatureF() const;

    /** Temperature in Celsius */
    float getTemperatureC() const;

    /** Relative humidity 0-100% */
    float getHumidityPct() const;

    /** Barometric pressure in hPa (millibars) */
    float getPressureHPa() const;

    // --- Davis packet encoding helpers ---

    /**
     * Get temperature as Davis raw int16 for packet bytes 3-4.
     * Formula: raw = (int16_t)(temp_F * 160.0)
     * Stored big-endian in packet: byte3 = raw >> 8, byte4 = raw & 0xFF
     */
    int16_t getDavisTemperatureRaw() const;

    /**
     * Get humidity as Davis raw uint16 for packet bytes 3-4.
     * Formula: raw = (uint16_t)(humidity * 10.0)
     * Encoding: byte3 = raw & 0xFF, byte4 upper nibble = (raw >> 8) << 4
     */
    uint16_t getDavisHumidityRaw() const;

    // --- Calibration ---

    /** Set temperature offset for field calibration */
    void setTempOffset(float offsetF);

    /** Get the current temperature offset */
    float getTempOffset() const;

private:
    Adafruit_BME280 _bme;
    TwoWire*  _wire;
    uint8_t   _addr;
    float     _tempOffsetF;
    bool      _initialized;
    SensorStatus _status;

    // Last readings
    float _tempC;
    float _tempF;
    float _humidity;
    float _pressure;
};

// =============================================================================
// Test stub — returns configurable values without real hardware
// =============================================================================
class BME280Stub : public SensorDriver {
public:
    BME280Stub()
        : _tempF(72.0f), _humidity(45.0f), _pressure(1013.25f) {}

    void setValues(float tempF, float humidity, float pressureHPa) {
        _tempF = tempF;
        _humidity = humidity;
        _pressure = pressureHPa;
    }

    bool begin() override { return true; }
    bool read() override { return true; }
    bool selfTest() override { return true; }
    SensorStatus getStatus() override { return SENSOR_OK; }
    const char* getName() override { return "BME280_STUB"; }

    float getTemperatureF() const { return _tempF; }
    float getTemperatureC() const { return (_tempF - 32.0f) * 5.0f / 9.0f; }
    float getHumidityPct()  const { return _humidity; }
    float getPressureHPa()  const { return _pressure; }

    int16_t  getDavisTemperatureRaw() const { return (int16_t)(_tempF * 160.0f); }
    uint16_t getDavisHumidityRaw()    const { return (uint16_t)(_humidity * 10.0f); }

    void setTempOffset(float) {}
    float getTempOffset() const { return 0.0f; }

private:
    float _tempF, _humidity, _pressure;
};

#endif // BME280_DRIVER_H
