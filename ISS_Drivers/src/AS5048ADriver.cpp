//File: AS5048ADriver.cpp
//-----------------------
#include "AS5048ADriver.h"

#define AS5048A_REG_ANGLE 0x3FFF

AS5048ADriver::AS5048ADriver(uint8_t csPin, uint8_t statusLedPin)
    : _csPin(csPin),
      _statusLedPin(statusLedPin),
      _lastRaw(0),
      _lastDeg(0.0f),
      _offsetDeg(0.0f),
      _calState(CAL_WAIT),
      _lastBlinkMs(0),
      _ledState(false)
{}

bool AS5048ADriver::begin() {
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    if (_statusLedPin != 0xFF) {
        pinMode(_statusLedPin, OUTPUT);
        digitalWrite(_statusLedPin, LOW);
    }

    SPI.begin();
   // SPI.usingInterrupt(digitalPinToInterrupt(_csPin));

    _calState    = CAL_WAIT;
    _offsetDeg   = 0.0f;
    _lastBlinkMs = millis();
    _ledState    = false;
    return true;
}
bool AS5048ADriver::read() {
    uint32_t now = millis();

    // Blink LED while waiting for first valid reading
    if (_calState == CAL_WAIT && _statusLedPin != 0xFF) {
        if (now - _lastBlinkMs >= 200) {
            _lastBlinkMs = now;
            _ledState = !_ledState;
            digitalWrite(_statusLedPin, _ledState ? HIGH : LOW);
        }
    }

    // --- Minimal AS5048A angle read (matches test sketch) ---
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));

    noInterrupts();
    digitalWrite(_csPin, LOW);
    uint16_t frame = SPI.transfer16(0xFFFF);   // <-- identical to test sketch
    digitalWrite(_csPin, HIGH);
    interrupts();

    SPI.endTransaction();

    // Skip parity for now (debugging)
     if (!checkParity(frame)) return false;

    uint16_t raw = frame & 0x3FFF;  // 14-bit angle
    float rawDeg = (raw * 360.0f) / 16384.0f;

    // One-time calibration: align first reading to 90°
    if (_calState == CAL_WAIT) {
        _offsetDeg = 90.0f - rawDeg;
        _calState  = CAL_DONE;
        if (_statusLedPin != 0xFF) {
            digitalWrite(_statusLedPin, LOW);
        }
    }

    float adj = rawDeg + _offsetDeg;
    while (adj < 0.0f)   adj += 360.0f;
    while (adj >= 360.0f) adj -= 360.0f;

    _lastRaw = raw;
    _lastDeg = adj;
    return true;
}

float AS5048ADriver::getAngleDeg() const {
    return _lastDeg;
}

uint8_t AS5048ADriver::getDavisDirectionByte() const {
    // Map 0–360° → 0–255
    return (uint8_t)((_lastDeg / 360.0f) * 255.0f + 0.5f);
}


bool AS5048ADriver::checkParity(uint16_t frame) {
    // Bits 0–14 are data + error bit
    uint16_t v = frame & 0x7FFF;

    bool p = 0;
    while (v) {
        p = !p;
        v &= (v - 1);
    }

    bool parityBit = (frame & 0x8000) != 0;

    // Even parity: parityBit must equal p
    return (p == parityBit);
}
