#include "TxIdManager.h"
#include <Arduino.h>
#include <EEPROM.h>

TxIdManager::TxIdManager(uint8_t pinButton, uint8_t pinLED, uint8_t defaultId)
    : _pinButton(pinButton),
      _pinLED(pinLED),
      _currentId(defaultId),
      _mode(NORMAL),
      _pressStart(0),
      _tapCount(0),
      _lastTapTime(0),
      _flashTimer(0),
      _flashState(false),
      _lastButtonState(false)
{}

void TxIdManager::begin() {
    pinMode(_pinButton, INPUT_PULLUP);
    pinMode(_pinLED, OUTPUT);
    digitalWrite(_pinLED, LOW);
    loadFromEEPROM();
}

void TxIdManager::update() {
    bool pressed = (digitalRead(_pinButton) == LOW);
    uint32_t now = millis();

    switch (_mode) {

    case NORMAL:
        if (pressed) {
            _pressStart = now;
            _mode = PRESSING;
        }
        break;

    case PRESSING:
        if (!pressed) {
            _mode = NORMAL;   // short press ignored
        } else if (now - _pressStart >= LONG_PRESS_MS) {
            _mode = PROGRAM_FLASH;
            _flashTimer = now;
            _flashState = false;
        }
        break;

    case PROGRAM_FLASH:
        if (!pressed) {
            _mode = TAP_COUNT;
            _tapCount = 0;
            _lastTapTime = now;
            digitalWrite(_pinLED, LOW);
        } else {
            if (now - _flashTimer >= 50) {
                _flashTimer = now;
                _flashState = !_flashState;
                digitalWrite(_pinLED, _flashState);
            }
        }
        break;

    case TAP_COUNT:
        if (pressed && !_lastButtonState) {
            _tapCount++;
            blinkOnce();
            _lastTapTime = now;
        }
        if (now - _lastTapTime >= 2000) {
            finalizeId();
        }
        break;

    case CONFIRM:
        if (now - _flashTimer >= BLINK_INTERVAL_MS) {
            _flashTimer = now;
            if (_confirmCount > 0) {
                digitalWrite(_pinLED, !_flashState);
                _flashState = !_flashState;
                _confirmCount--;
            } else {
                digitalWrite(_pinLED, LOW);
                _mode = NORMAL;
            }
        }
        break;
    }

    _lastButtonState = pressed;
}

void TxIdManager::flashTx() {
    digitalWrite(_pinLED, HIGH);
    delayMicroseconds(500);
    digitalWrite(_pinLED, LOW);
}

void TxIdManager::blinkOnce() {
    digitalWrite(_pinLED, HIGH);
    delay(80);
    digitalWrite(_pinLED, LOW);
    delay(80);
}

void TxIdManager::finalizeId() {
    if (_tapCount >= 1 && _tapCount <= 8) {
        _currentId = _tapCount - 1;   // Davis uses 1–8 → TX ID 0–7
        saveToEEPROM();
    }
    _confirmCount = _tapCount;
    _flashTimer = millis();
    _flashState = false;
    _mode = CONFIRM;
}

void TxIdManager::loadFromEEPROM() {
    _currentId = EEPROM.read(TXID_EEPROM_ADDR) & 0x07;
}

void TxIdManager::saveToEEPROM() {
    EEPROM.write(TXID_EEPROM_ADDR, _currentId);
}

bool TxIdManager::wasButtonPressed() {
    // Compatibility stub: return true on any short press
    // (Pressed then released without long-press)
    // This mimics your old behavior.
    return false;  // For now, no short-press behavior needed
}
