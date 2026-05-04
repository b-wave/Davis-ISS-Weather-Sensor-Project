/**
 * WindSpeedDriver.h — Hall-Effect Wind Speed Sensor Driver
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Reads wind speed from a hall-effect reed switch in the cup assembly.
 * The switch closes twice per revolution. Speed is computed from pulse
 * period using Davis's published RPS-to-MPH constant (2.25).
 *
 * Includes Davis OEM error correction via bilinear interpolation across
 * the calibration table in DavisCalibration.h. Wind affects both sides
 * of the cups — head-on reads high, tailwind reads low. The correction
 * is angle-dependent and becomes significant above 20 mph.
 *
 * Reference: VPTools by kobuki, Davis OEM calibration data
 */

#ifndef WIND_SPEED_DRIVER_H
#define WIND_SPEED_DRIVER_H

#include "SensorDriver.h"
#include "DavisCalibration.h"

class WindSpeedDriver : public SensorDriver {
public:
    /**
     * @param interruptPin  Digital pin connected to hall-effect switch
     * @param enableEC      Enable Davis error correction (default: true)
     */
    WindSpeedDriver(uint8_t interruptPin, bool enableEC = true);

    bool begin() override;
    bool read() override;
    bool selfTest() override;
    SensorStatus getStatus() override;
    const char* getName() override;

    // --- Wind speed accessors ---

    /** Average wind speed over last sample period (mph) */
    uint8_t getSpeedMPH() const;

    /** Raw uncorrected speed (mph, float) */
    float getSpeedRawMPH() const;

    /** Peak gust since last resetGust() call (mph) */
    uint8_t getGustMPH() const;

    /** Reset the gust tracker (call after reading gust for packet type 0x9) */
    void resetGust();

    /** Get pulse count since last read() — useful for diagnostics */
    uint16_t getPulseCount() const;

    /** Get the last measured rotation period in microseconds */
    uint32_t getLastPeriodUs() const;

    /**
     * Set the wind direction for error correction.
     * Must be called before read() for EC to work.
     * @param degrees  Wind vane angle 0-360
     */
    void setWindDirection(int degrees);

    // --- Static ISR entry point ---
    static void isrHandler();

private:
    uint8_t  _pin;
    bool     _enableEC;
    bool     _initialized;
    int      _windDirDeg;

    // Computed values
    float    _speedRawMPH;
    uint8_t  _speedMPH;
    uint8_t  _gustMPH;

    // ISR-shared state (volatile)
    static volatile uint32_t _isrLastSwitchTime;
    static volatile uint32_t _isrLastPulseStart;
    static volatile uint32_t _isrRotationPeriod;
    static volatile uint32_t _isrSampleStart;
    static volatile int      _isrPulseCount;

    // Snapshot from ISR (copied in read())
    uint32_t _snapPeriod;
    uint32_t _snapSampleStart;
    uint32_t _snapLastPulse;
    int      _snapPulseCount;

    // Error correction
    float applyErrorCorrection(float rawMPH, int angleDeg);
    float interpolateEC(float rawMPH, int angle,
                        int colBase);
};

// =============================================================================
// Test stub — returns configurable values without real hardware
// =============================================================================
class WindSpeedStub : public SensorDriver {
public:
    WindSpeedStub() : _speed(0), _gust(0), _pulses(0) {}

    void setValues(uint8_t speedMPH, uint8_t gustMPH, uint16_t pulses = 10) {
        _speed = speedMPH;
        _gust = gustMPH;
        _pulses = pulses;
    }

    bool begin() override { return true; }
    bool read() override { return true; }
    bool selfTest() override { return true; }
    SensorStatus getStatus() override { return SENSOR_OK; }
    const char* getName() override { return "WindSpeed_STUB"; }

    uint8_t getSpeedMPH() const { return _speed; }
    uint8_t getGustMPH() const { return _gust; }
    void resetGust() { _gust = 0; }
    uint16_t getPulseCount() const { return _pulses; }
    void setWindDirection(int) {} 
    
private:
    uint8_t  _speed;
    uint8_t  _gust;
    uint16_t _pulses;
};

#endif // WIND_SPEED_DRIVER_H

