/**
 * WindSpeedDriver.cpp — Hall-Effect Wind Speed Sensor Implementation
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Implements two speed-calculation algorithms (matching VPTools):
 *   Mode 1 (< MIN_PULSE_COUNT pulses): Use last single rotation period
 *   Mode 2 (>= MIN_PULSE_COUNT pulses): Average period across all pulses
 *
 * Davis OEM error correction uses bilinear interpolation across the
 * calibration table for wind speeds 20-150 mph, factoring in wind
 * direction relative to the cup assembly. Below 20 mph the correction
 * is negligible; above 150 mph the raw value is returned as-is.
 *
 * Reference: VPTools/AnemometerTX by kobuki, Davis OEM calibration data
 */

#include "WindSpeedDriver.h"

// ---------------------------------------------------------------------------
// Static volatile ISR state
// ---------------------------------------------------------------------------
volatile uint32_t WindSpeedDriver::_isrLastSwitchTime = 0;
volatile uint32_t WindSpeedDriver::_isrLastPulseStart = 0;
volatile uint32_t WindSpeedDriver::_isrRotationPeriod = 0;
volatile uint32_t WindSpeedDriver::_isrSampleStart    = 0;
volatile int      WindSpeedDriver::_isrPulseCount     = 0;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
WindSpeedDriver::WindSpeedDriver(uint8_t interruptPin, bool enableEC)
    : _pin(interruptPin)
    , _enableEC(enableEC)
    , _initialized(false)
    , _windDirDeg(0)
    , _speedRawMPH(0.0f)
    , _speedMPH(0)
    , _gustMPH(0)
    , _snapPeriod(0)
    , _snapSampleStart(0)
    , _snapLastPulse(0)
    , _snapPulseCount(0)
{}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
bool WindSpeedDriver::begin() {
    pinMode(_pin, INPUT_PULLUP);

    // Reset ISR state
    noInterrupts();
    _isrLastSwitchTime = 0;
    _isrLastPulseStart = 0;
    _isrRotationPeriod = WIND_TIMEOUT_US;
    _isrSampleStart    = 0;
    _isrPulseCount     = 0;
    interrupts();

    // Attach interrupt — FALLING edge = switch closure
    attachInterrupt(digitalPinToInterrupt(_pin), isrHandler, FALLING);

    _initialized = true;
    return true;
}

// ---------------------------------------------------------------------------
// ISR — minimal work: count pulses, track timing
// ---------------------------------------------------------------------------
void WindSpeedDriver::isrHandler() {
    uint32_t t = micros();

    // Debounce: ignore pulses < 5 ms apart
    if (_isrLastPulseStart > 0 &&
        (t - _isrLastSwitchTime) > WIND_DEBOUNCE_US) {
        _isrRotationPeriod = t - _isrLastPulseStart;
        _isrLastPulseStart = t;
        _isrPulseCount++;
    }

    _isrLastSwitchTime = t;

    if (_isrLastPulseStart == 0) _isrLastPulseStart = t;
    if (_isrSampleStart == 0)    _isrSampleStart = t;
}

// ---------------------------------------------------------------------------
// Read — snapshot ISR state and compute speed
// ---------------------------------------------------------------------------
bool WindSpeedDriver::read() {
    if (!_initialized) return false;

    uint32_t now = micros();

    // Atomic snapshot of ISR state
    noInterrupts();
    _snapPeriod      = _isrRotationPeriod;
    _snapSampleStart = _isrSampleStart;
    _snapLastPulse   = _isrLastPulseStart;
    _snapPulseCount  = _isrPulseCount;
    // Reset for next sample period
    _isrPulseCount   = 0;
    _isrSampleStart  = 0;
    interrupts();

    // Check for wind timeout (no pulse in WIND_TIMEOUT_US)
    if (_snapPeriod == 0 ||
        _snapPeriod >= WIND_TIMEOUT_US ||
        (now - _snapLastPulse) > WIND_TIMEOUT_US) {
        _speedRawMPH = 0.0f;
        _speedMPH = 0;
        return true;  // Not an error — just no wind
    }

    // Compute raw speed from pulse timing
    uint32_t period;

    if (_snapPulseCount < WIND_MIN_PULSE_COUNT) {
        // Mode 1: Use last single rotation period
        period = _snapPeriod;
    } else {
        // Mode 2: Average period across all pulses in sample window
        if (_snapPulseCount > 1) {
            period = (_snapLastPulse - _snapSampleStart) / (_snapPulseCount - 1);
        } else {
            period = _snapPeriod;
        }
    }

    // Davis formula: mph = (1,000,000 / period_us) * WIND_RPS_MPH
    _speedRawMPH = 1000000.0f / (float)period * WIND_RPS_MPH;

    // Apply error correction if enabled
    float corrected;
    if (_enableEC && _speedRawMPH >= 20.0f) {
        corrected = applyErrorCorrection(_speedRawMPH, _windDirDeg);
    } else {
        corrected = _speedRawMPH;
    }

    // Clamp and store as uint8_t (Davis byte 1 range: 0-255 mph)
    _speedMPH = (uint8_t)constrain((int)round(corrected), 0, 255);

    // Track peak gust
    if (_speedMPH > _gustMPH) {
        _gustMPH = _speedMPH;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Davis OEM error correction with bilinear interpolation
// ---------------------------------------------------------------------------
float WindSpeedDriver::applyErrorCorrection(float rawMPH, int angleDeg) {
    // EC is symmetric between E and W (90° and 270°)
    if (angleDeg > 180) angleDeg = 360 - angleDeg;

    // Determine which two table columns bracket this angle
    // Column 0 = 0° (head-on), Column 1 = 90°, Column 2 = 180° (tail)
    int colBase = (angleDeg - 1) / 90;
    if (colBase < 0) colBase = 0;
    if (colBase > 1) colBase = 1;  // cols 0+1 or cols 1+2

    // Normalize angle to 0-90 range for interpolation
    int normAngle = (angleDeg >= 90 && angleDeg % 90 == 0) ? 90 : angleDeg % 90;

    return interpolateEC(rawMPH, normAngle, colBase);
}

float WindSpeedDriver::interpolateEC(float rawMPH, int angle, int colBase) {
    // Structure matching VPTools: find the two table rows that bracket
    // the raw MPH value in each of the two angle columns, then bilinear
    // interpolate.

    struct ECPoint { float raw; float real; };
    ECPoint im[2][2];

    for (uint8_t icol = 0; icol < 2; icol++) {
        uint8_t col = colBase + icol;
        if (col >= WIND_CAL_COLS) col = WIND_CAL_COLS - 1;

        float firstVal = readCalTable(0, col);
        float lastVal  = readCalTable(WIND_CAL_ROWS - 1, col);

        if (rawMPH <= firstVal) {
            // Below table range — no correction
            im[0][icol] = { 1.0f, 1.0f };
            im[1][icol] = { firstVal, (float)WIND_CAL_MIN_MPH };
        } else if (rawMPH >= lastVal) {
            // Above table range — return raw
            return rawMPH;
        } else {
            // Find bracketing rows
            uint8_t i;
            for (i = 0; rawMPH > readCalTable(i, col); i++);
            im[0][icol] = { readCalTable(i - 1, col),
                            (float)(WIND_CAL_MIN_MPH - WIND_CAL_STEP + i * WIND_CAL_STEP) };
            im[1][icol] = { readCalTable(i, col),
                            (float)(WIND_CAL_MIN_MPH + i * WIND_CAL_STEP) };
        }
    }

    // Bilinear interpolation (matching VPTools)
    float mph1, mph2;

    if (im[0][0].raw == im[1][0].raw) {
        mph1 = im[0][0].real;
    } else {
        mph1 = im[0][0].real +
               (im[1][0].real - im[0][0].real) *
               (rawMPH - im[0][0].raw) /
               (im[1][0].raw - im[0][0].raw);
    }

    if (im[0][1].raw == im[1][1].raw) {
        mph2 = im[0][1].real;
    } else {
        mph2 = im[0][1].real +
               (im[1][1].real - im[0][1].real) *
               (rawMPH - im[0][1].raw) /
               (im[1][1].raw - im[0][1].raw);
    }

    return mph1 + (float)angle / 90.0f * (mph2 - mph1);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
uint8_t  WindSpeedDriver::getSpeedMPH()     const { return _speedMPH; }
float    WindSpeedDriver::getSpeedRawMPH()   const { return _speedRawMPH; }
uint8_t  WindSpeedDriver::getGustMPH()       const { return _gustMPH; }
uint16_t WindSpeedDriver::getPulseCount()    const { return (uint16_t)_snapPulseCount; }
uint32_t WindSpeedDriver::getLastPeriodUs()  const { return _snapPeriod; }
void     WindSpeedDriver::resetGust()              { _gustMPH = 0; }
void     WindSpeedDriver::setWindDirection(int d)  { _windDirDeg = d; }

// ---------------------------------------------------------------------------
// Self-test & status
// ---------------------------------------------------------------------------
bool WindSpeedDriver::selfTest() {
    if (!_initialized) return false;
    // Verify pin is configured — read should be HIGH (pullup, switch open)
    return (digitalRead(_pin) == HIGH);
}

SensorStatus WindSpeedDriver::getStatus() {
    if (!_initialized) return SENSOR_NOT_INITIALIZED;
    return SENSOR_OK;
}

const char* WindSpeedDriver::getName() { return "WindSpeed_HallEffect"; }

