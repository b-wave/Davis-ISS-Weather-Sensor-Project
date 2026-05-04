/**
 * TxIdManager.cpp — Transmitter ID Configuration Implementation
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Non-blocking state machine for button debounce, press detection,
 * and LED blink feedback. Never calls delay().
 */

#include "TxIdManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
TxIdManager::TxIdManager(uint8_t switchPin, uint8_t ledPin, uint8_t defaultId)
    : _switchPin(switchPin)
    , _ledPin(ledPin)
    , _txId(defaultId & 0x07)
    , _defaultId(defaultId & 0x07)
    , _state(TXID_IDLE)
    , _ledOn(false)
    , _buttonEvent(false)
    , _pressStartMs(0)
    , _lastDebounceMs(0)
    , _blinkStartMs(0)
    , _blinksRemaining(0)
    , _blinkPhase(false)
    , _lastButtonState(HIGH)
    , _stableButtonState(HIGH)
{}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
void TxIdManager::begin() {
    pinMode(_switchPin, INPUT_PULLUP);
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_ledPin, LOW);

    loadFromEEPROM();

    // Quick blink on startup to show current ID
    startBlinkSequence(_txId + 1);
}

// ---------------------------------------------------------------------------
// Main update — call from loop(), non-blocking
// ---------------------------------------------------------------------------
void TxIdManager::update() {
    uint32_t now = millis();

    // --- Button debounce ---
    bool rawButton = digitalRead(_switchPin);

    if (rawButton != _lastButtonState) {
        _lastDebounceMs = now;
    }
    _lastButtonState = rawButton;

    bool buttonPressed = false;
    bool buttonReleased = false;

    if ((now - _lastDebounceMs) > DEBOUNCE_MS) {
        if (rawButton != _stableButtonState) {
            bool oldState = _stableButtonState;
            _stableButtonState = rawButton;

            if (oldState == HIGH && _stableButtonState == LOW) {
                // Button just pressed (active low)
                buttonPressed = true;
                _pressStartMs = now;
            } else if (oldState == LOW && _stableButtonState == HIGH) {
                // Button just released
                buttonReleased = true;
            }
        }
    }

    // --- State machine ---
    switch (_state) {
        case TXID_IDLE:
            if (buttonPressed) {
                _state = TXID_PRESSED;
                digitalWrite(_ledPin, HIGH);
                _ledOn = true;
            }
            break;

        case TXID_PRESSED:
            if (buttonReleased) {
                uint32_t duration = now - _pressStartMs;
                digitalWrite(_ledPin, LOW);
                _ledOn = false;

                if (duration < SHORT_PRESS_MS) {
                    // Short press: increment TX ID
                    _txId = (_txId + 1) & 0x07;
                    saveToEEPROM();
                    _buttonEvent = true;

                    // Blink new ID
                    startBlinkSequence(_txId + 1);
                    _state = TXID_BLINKING;
                } else {
                    // Long press: enter display mode
                    startBlinkSequence(_txId + 1);
                    _state = TXID_DISPLAY_MODE;
                }
            } else if ((now - _pressStartMs) > LONG_PRESS_MS) {
                // Visual feedback: LED stays on during long press
                // (already on from PRESSED state)
            }
            break;

        case TXID_BLINKING:
            updateBlink();
            if (_blinksRemaining == 0 && !_blinkPhase) {
                _state = TXID_IDLE;
            }
            break;

        case TXID_DISPLAY_MODE:
            updateBlink();
            if (_blinksRemaining == 0 && !_blinkPhase) {
                // Repeat the blink sequence in display mode
                if (_stableButtonState == HIGH) {
                    // Button released, show once more then exit
                    _state = TXID_IDLE;
                } else {
                    startBlinkSequence(_txId + 1);
                }
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// Blink management
// ---------------------------------------------------------------------------
void TxIdManager::startBlinkSequence(uint8_t count) {
    _blinksRemaining = count;
    _blinkPhase = true;  // Start with LED on
    _blinkStartMs = millis();
    digitalWrite(_ledPin, HIGH);
    _ledOn = true;
}

void TxIdManager::updateBlink() {
    if (_blinksRemaining == 0) return;

    uint32_t now = millis();
    uint32_t elapsed = now - _blinkStartMs;

    if (_blinkPhase) {
        // LED is on — wait for on duration
        if (elapsed >= BLINK_ON_MS) {
            digitalWrite(_ledPin, LOW);
            _ledOn = false;
            _blinkPhase = false;
            _blinkStartMs = now;
            _blinksRemaining--;
        }
    } else {
        // LED is off — wait for off duration (or pause if done)
        uint32_t offTime = (_blinksRemaining > 0) ? BLINK_OFF_MS : BLINK_PAUSE_MS;
        if (elapsed >= offTime) {
            if (_blinksRemaining > 0) {
                digitalWrite(_ledPin, HIGH);
                _ledOn = true;
                _blinkPhase = true;
                _blinkStartMs = now;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// EEPROM persistence
// ---------------------------------------------------------------------------
void TxIdManager::saveToEEPROM() {
    EEPROM.write(TX_ID_EEPROM_ADDR, TX_ID_EEPROM_MAGIC);
    EEPROM.write(TX_ID_EEPROM_ADDR + 1, _txId);
}

void TxIdManager::loadFromEEPROM() {
    uint8_t magic = EEPROM.read(TX_ID_EEPROM_ADDR);
    if (magic == TX_ID_EEPROM_MAGIC) {
        _txId = EEPROM.read(TX_ID_EEPROM_ADDR + 1) & 0x07;
    } else {
        _txId = _defaultId;
        saveToEEPROM();
    }
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
uint8_t TxIdManager::getCurrentId() const { return _txId; }

void TxIdManager::setId(uint8_t id) {
    _txId = id & 0x07;
    saveToEEPROM();
}

bool TxIdManager::getLedState() const { return _ledOn; }

bool TxIdManager::wasButtonPressed() {
    bool p = _buttonEvent;
    _buttonEvent = false;
    return p;
}

