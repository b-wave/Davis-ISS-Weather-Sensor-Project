// ============================================================================
// SHT45Driver.cpp — Sensirion SHT45 Temperature/Humidity Driver with Heater
// ============================================================================
// REV: 050226
// VERS: 1.0
//
// See SHT45Driver.h for API documentation and usage notes.
// ============================================================================

#include "SHT45Driver.h"

// ============================================================================
// SHT45Driver Implementation
// ============================================================================

SHT45Driver::SHT45Driver()
    : _wire(nullptr)
    , _tempC(0.0f)
    , _humidity(0.0f)
    , _valid(false)
    , _lastMeasureMs(0)
    , _lastHeaterMs(0)
    , _lastHeaterLevel(HEATER_OFF)
{
}

bool SHT45Driver::begin(TwoWire &wire) {
    _wire = &wire;

    // Soft reset first
    reset();
    delay(2);   // Datasheet: 1ms recovery after reset

    // Verify the sensor is on the bus
    return isConnected();
}

void SHT45Driver::reset() {
    if (_wire) {
        sendCommand(SHT45_CMD_RESET);
    }
    _valid = false;
}

uint32_t SHT45Driver::getSerialNumber() {
    if (!_wire) return 0;

    sendCommand(SHT45_CMD_SERIAL);
    delay(1);

    uint8_t buf[6];
    if (!readResponse(buf, 6)) return 0;

    // Serial number is in two 16-bit words (bytes 0-1 and 3-4)
    // Bytes 2 and 5 are CRC
    uint32_t serial = ((uint32_t)buf[0] << 24) |
                      ((uint32_t)buf[1] << 16) |
                      ((uint32_t)buf[3] << 8)  |
                      (uint32_t)buf[4];
    return serial;
}

// ---- Measurement ----

bool SHT45Driver::measure() {
    if (!_wire) return false;

    // High repeatability measurement (8.2ms max)
    if (!sendCommand(SHT45_CMD_MEAS_HIGH)) return false;
    delay(10);   // 8.2ms + margin

    uint8_t buf[6];
    if (!readResponse(buf, 6)) {
        _valid = false;
        return false;
    }

    // Verify CRC for both words
    if (crc8(buf, 2) != buf[2] || crc8(buf + 3, 2) != buf[5]) {
        _valid = false;
        return false;
    }

    // Convert raw values (Sensirion datasheet formulas)
    uint16_t rawTemp = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t rawHum  = ((uint16_t)buf[3] << 8) | buf[4];

    _tempC    = -45.0f + 175.0f * ((float)rawTemp / 65535.0f);
    _humidity = -6.0f + 125.0f * ((float)rawHum / 65535.0f);

    // Clamp humidity to physical range
    if (_humidity < 0.0f)   _humidity = 0.0f;
    if (_humidity > 100.0f) _humidity = 100.0f;

    _valid = true;
    _lastMeasureMs = millis();
    return true;
}

float SHT45Driver::getTemperatureC() {
    return _tempC;
}

float SHT45Driver::getTemperatureF() {
    return _tempC * 9.0f / 5.0f + 32.0f;
}

float SHT45Driver::getHumidity() {
    return _humidity;
}

int16_t SHT45Driver::getDavisTempRaw() {
    // Davis packet type 0x8: int16 = temp_F * 160
    // Stored big-endian in bytes 3-4 of the packet
    float tempF = getTemperatureF();
    return (int16_t)(tempF * 160.0f);
}

uint16_t SHT45Driver::getDavisHumidRaw() {
    // Davis packet type 0xA encoding:
    //   byte 3 = lower 8 bits of (RH% * 10)
    //   byte 4 bits 5:4 = upper 2 bits of (RH% * 10)
    // So the full value is: ((b4>>4 & 0x03) << 8) | b3 = RH% * 10
    uint16_t rh10 = (uint16_t)(_humidity * 10.0f + 0.5f);
    return rh10;
}

// ---- Heater Control ----

bool SHT45Driver::runHeaterPulse(SHT45HeaterLevel level, bool longPulse) {
    if (!_wire || level == HEATER_OFF) return false;

    // Select the appropriate I2C command
    uint8_t cmd;
    uint16_t waitMs;

    switch (level) {
        case HEATER_LOW:
            cmd = longPulse ? SHT45_CMD_HEAT_L_1S : SHT45_CMD_HEAT_L_01S;
            break;
        case HEATER_MEDIUM:
            cmd = longPulse ? SHT45_CMD_HEAT_M_1S : SHT45_CMD_HEAT_M_01S;
            break;
        case HEATER_HIGH:
            cmd = longPulse ? SHT45_CMD_HEAT_H_1S : SHT45_CMD_HEAT_H_01S;
            break;
        default:
            return false;
    }

    // Heater commands include a measurement after the pulse
    // Wait time = heater duration + measurement time
    waitMs = longPulse ? 1100 : 120;   // 1.0s or 0.1s + ~10ms measurement

    if (!sendCommand(cmd)) return false;
    delay(waitMs);

    // Read the post-heater measurement
    uint8_t buf[6];
    if (!readResponse(buf, 6)) {
        _valid = false;
        return false;
    }

    // Verify CRC
    if (crc8(buf, 2) != buf[2] || crc8(buf + 3, 2) != buf[5]) {
        _valid = false;
        return false;
    }

    // Convert (same as measure())
    uint16_t rawTemp = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t rawHum  = ((uint16_t)buf[3] << 8) | buf[4];

    _tempC    = -45.0f + 175.0f * ((float)rawTemp / 65535.0f);
    _humidity = -6.0f + 125.0f * ((float)rawHum / 65535.0f);

    if (_humidity < 0.0f)   _humidity = 0.0f;
    if (_humidity > 100.0f) _humidity = 100.0f;

    _valid = true;
    _lastMeasureMs  = millis();
    _lastHeaterMs   = millis();
    _lastHeaterLevel = level;

    return true;
}

SHT45HeaterLevel SHT45Driver::getLastHeaterLevel() {
    return _lastHeaterLevel;
}

uint32_t SHT45Driver::getLastHeaterTime() {
    return _lastHeaterMs;
}

// ---- Status ----

bool SHT45Driver::isConnected() {
    if (!_wire) return false;
    _wire->beginTransmission(SHT45_I2C_ADDR);
    return (_wire->endTransmission() == 0);
}

bool SHT45Driver::isValid() {
    return _valid;
}

uint32_t SHT45Driver::getLastMeasureTime() {
    return _lastMeasureMs;
}

// ---- Private Helpers ----

bool SHT45Driver::sendCommand(uint8_t cmd) {
    _wire->beginTransmission(SHT45_I2C_ADDR);
    _wire->write(cmd);
    return (_wire->endTransmission() == 0);
}

bool SHT45Driver::readResponse(uint8_t* buf, uint8_t len) {
    uint8_t received = _wire->requestFrom((uint8_t)SHT45_I2C_ADDR, len);
    if (received != len) return false;

    for (uint8_t i = 0; i < len; i++) {
        buf[i] = _wire->read();
    }
    return true;
}

uint8_t SHT45Driver::crc8(const uint8_t* data, uint8_t len) {
    // Sensirion CRC-8: polynomial 0x31, init 0xFF
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = crc << 1;
        }
    }
    return crc;
}


// ============================================================================
// HeaterManager Implementation
// ============================================================================

HeaterManager::HeaterManager()
    : _sensor(nullptr)
    , _level(HEATER_OFF)
    , _enabled(true)
    , _longPulse(false)
    , _lastHeaterMs(0)
    , _heaterIntervalMs(26000)     // ~10 TX cycles at 2.6s each
    , _pulseCount(0)
    , _rhThresholdMed(95.0f)
    , _rhThresholdHigh(99.0f)
    , _voltageMinimum(3.0f)
    , _minVoltage(5.0f)
{
}

void HeaterManager::begin(SHT45Driver* sensor) {
    _sensor = sensor;
    _lastHeaterMs = millis();
}

bool HeaterManager::evaluate(float currentRH, float supercapVoltage) {
    // Track minimum voltage seen
    if (supercapVoltage < _minVoltage) {
        _minVoltage = supercapVoltage;
    }

    // ---- Power gate: disable heater when voltage is marginal ----
    if (supercapVoltage < _voltageMinimum) {
        _enabled = false;
        _level   = HEATER_OFF;
        return false;
    }
    _enabled = true;

    // ---- Time gate: not yet time for a pulse ----
    uint32_t now = millis();
    if ((now - _lastHeaterMs) < _heaterIntervalMs) {
        return false;
    }

    // ---- Determine heater level based on humidity ----
    if (currentRH >= _rhThresholdHigh) {
        _level = HEATER_HIGH;
    } else if (currentRH >= _rhThresholdMed) {
        _level = HEATER_MEDIUM;
    } else {
        _level = HEATER_LOW;       // Routine maintenance pulse
    }

    // ---- Fire the heater ----
    if (_sensor && _sensor->runHeaterPulse(_level, _longPulse)) {
        _lastHeaterMs = millis();
        _pulseCount++;
        return true;
    }

    return false;
}

// ---- Configuration ----

void HeaterManager::setHeaterInterval(uint32_t ms) {
    _heaterIntervalMs = ms;
}

void HeaterManager::setRHThresholdMed(float rh) {
    _rhThresholdMed = rh;
}

void HeaterManager::setRHThresholdHigh(float rh) {
    _rhThresholdHigh = rh;
}

void HeaterManager::setVoltageMinimum(float volts) {
    _voltageMinimum = volts;
}

void HeaterManager::setLongPulse(bool useLong) {
    _longPulse = useLong;
}

// ---- Status ----

bool HeaterManager::isHeaterEnabled() {
    return _enabled;
}

SHT45HeaterLevel HeaterManager::getHeaterLevel() {
    return _level;
}

uint32_t HeaterManager::getLastHeaterTime() {
    return _lastHeaterMs;
}

uint32_t HeaterManager::getPulseCount() {
    return _pulseCount;
}

float HeaterManager::getMinVoltage() {
    return _minVoltage;
}
