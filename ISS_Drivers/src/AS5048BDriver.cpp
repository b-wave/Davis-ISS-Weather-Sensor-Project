/**
 * AS5048BDriver.cpp — AS5048B Magnetic Angle Sensor Implementation
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * I2C register reads for 14-bit absolute angle. The AS5048B provides
 * contactless angular measurement using a Hall sensor array beneath
 * a diametrically magnetized disc magnet on the wind vane shaft.
 *
 * Key advantage over potentiometer vanes: no dead zone, no wear,
 * no contact resistance variation, works through PCB/enclosure.
 */
#include "AS5048BDriver.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AS5048BDriver::AS5048BDriver(uint8_t addr, TwoWire* wire)
    : _wire(wire)
    , _addr(addr)
    , _initialized(false)
    , _status(SENSOR_NOT_INITIALIZED)
    , _angleDeg(0.0f)
    , _angleRaw(0)
    , _agc(0)
    , _zeroOffset(0.0f)
    , _magnetOK(false)
{}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
bool AS5048BDriver::begin() {
    _wire->begin();

    // Check device presence
    _wire->beginTransmission(_addr);
    uint8_t err = _wire->endTransmission();
    if (err != 0) {
        _status = SENSOR_NOT_FOUND;
        return false;
    }

    // Read AGC to confirm communication
    _agc = readRegister8(AS5048B_REG_AGC);

    _initialized = true;
    _status = SENSOR_OK;
    return true;
}

// ---------------------------------------------------------------------------
// Read angle + AGC
// ---------------------------------------------------------------------------
bool AS5048BDriver::read() {
    if (!_initialized) {
        _status = SENSOR_NOT_INITIALIZED;
        return false;
    }

    // Read 14-bit angle
    _angleRaw = readRegister14(AS5048B_REG_ANGLE_HI, AS5048B_REG_ANGLE_LO);

    // Read AGC (magnet strength)
    _agc = readRegister8(AS5048B_REG_AGC);

    // Magnet OK range
    _magnetOK = (_agc >= 20 && _agc <= 240);

    // Convert to degrees
    _angleDeg = (float)_angleRaw * 360.0f / (float)AS5048B_RESOLUTION;

    // Apply software zero offset
    _angleDeg -= _zeroOffset;
    if (_angleDeg < 0.0f) _angleDeg += 360.0f;
    if (_angleDeg >= 360.0f) _angleDeg -= 360.0f;

    _status = SENSOR_OK;
    return true;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
float AS5048BDriver::getAngleDeg() const { return _angleDeg; }

uint16_t AS5048BDriver::getAngleRaw14() const { return _angleRaw; }

uint8_t AS5048BDriver::getDavisDirectionByte() const {
    if (!_magnetOK) return 0;  // Davis "no reading"

    uint8_t d = (uint8_t)(_angleDeg * 255.0f / 360.0f);

    if (d == 0) d = 1;  // avoid Davis "no reading" collision

    return d;
}

uint8_t AS5048BDriver::getAGC() const { return _agc; }
bool AS5048BDriver::isMagnetOK() const { return _magnetOK; }

// ---------------------------------------------------------------------------
// Zero offset
// ---------------------------------------------------------------------------
void AS5048BDriver::setZeroOffset(float offsetDeg) {
    _zeroOffset = offsetDeg;
}

// ---------------------------------------------------------------------------
// I2C register access
// ---------------------------------------------------------------------------
uint8_t AS5048BDriver::readRegister8(uint8_t reg) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->endTransmission(false);   // repeated start
    _wire->requestFrom(_addr, (uint8_t)1);

    if (_wire->available()) {
        return _wire->read();
    }
    return 0;
}

uint16_t AS5048BDriver::readRegister14(uint8_t regHi, uint8_t regLo) {
    uint8_t hi = readRegister8(regHi);
    uint8_t lo = readRegister8(regLo);
    return ((uint16_t)hi << 6) | (lo & 0x3F);
}

// ---------------------------------------------------------------------------
// Self-test
// ---------------------------------------------------------------------------
bool AS5048BDriver::selfTest() {
    if (!_initialized) return false;

    _agc = readRegister8(AS5048B_REG_AGC);

    if (_agc == 0 || _agc == 0xFF) {
        _status = SENSOR_NOT_FOUND;
        return false;
    }

    if (!_magnetOK) {
        _status = SENSOR_CAL_ERROR;
        return false;
    }

    _status = SENSOR_OK;
    return true;
}

SensorStatus AS5048BDriver::getStatus() { return _status; }
const char*  AS5048BDriver::getName()   { return "AS5048B_WindDir"; }
