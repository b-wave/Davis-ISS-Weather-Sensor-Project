/**
 * SensorDriver.h — Base interface for all ISS sensor drivers
 * 
 * ISS Packet Engine — Sensor Driver Architecture
 * All sensor drivers inherit from this base class, providing a uniform
 * interface for initialization, reading, self-test, and status reporting.
 * 
 * Test stubs also inherit from this base, enabling hardware-free testing
 * of the full Packet Engine pipeline.
 */

#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Sensor status codes
// ---------------------------------------------------------------------------
enum SensorStatus {
    SENSOR_OK = 0,
    SENSOR_NOT_FOUND,
    SENSOR_READ_ERROR,
    SENSOR_CRC_ERROR,
    SENSOR_TIMEOUT,
    SENSOR_NOT_INITIALIZED,
    SENSOR_DISABLED,
    SENSOR_CAL_ERROR        // calibration / range error
};

// ---------------------------------------------------------------------------
// Base class — all sensor drivers implement this interface
// ---------------------------------------------------------------------------
class SensorDriver {
public:
    /**
     * Initialize sensor hardware (I2C, SPI, GPIO, etc.).
     * @return true if sensor responds and passes basic ID check.
     */
    virtual bool begin() = 0;

    /**
     * Take a reading from the sensor and store it internally.
     * Call getXxx() methods afterward to retrieve decoded values.
     * @return true if reading was successful.
     */
    virtual bool read() = 0;

    /**
     * Perform a self-test to verify the sensor is functioning.
     * May include chip-ID verification, range checks, etc.
     * @return true if self-test passes.
     */
    virtual bool selfTest() = 0;

    /**
     * Get the current sensor status.
     * @return SensorStatus code.
     */
    virtual SensorStatus getStatus() = 0;

    /**
     * Get a human-readable name for this sensor (for debug logging).
     * @return Null-terminated string.
     */
    virtual const char* getName() = 0;

    virtual ~SensorDriver() {}
};

#endif // SENSOR_DRIVER_H

