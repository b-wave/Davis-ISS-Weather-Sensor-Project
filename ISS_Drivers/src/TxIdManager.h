/**
 * TxIdManager.h — Transmitter ID Configuration (Switch + LED)
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Manages the Davis transmitter ID (0-7) via a momentary pushbutton
 * and status LED for field configuration.
 *
 * Behavior:
 *   Short press (< 1s): Increment TX ID (wraps 7 → 0)
 *   LED blinks N+1 times to confirm new ID
 *   Long press (> 3s): Enter display mode (LED shows current ID)
 *   ID persisted in EEPROM across power cycles
 *
 * NOTE: Davis TX ID in the packet (byte 0, bits 0-2) is ONE LESS than
 * the DIP switch setting on a real ISS. ID=0 here = station 1 on console.
 * Reference: VPTools AnemometerTX by kobuki
 */

#ifndef TX_ID_MANAGER_H
#define TX_ID_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

#define TX_ID_MIN          0
#define TX_ID_MAX          7
#define TX_ID_EEPROM_ADDR  0        // EEPROM address for persistent storage
#define TX_ID_EEPROM_MAGIC 0xDA     // Magic byte to verify EEPROM is initialized

#define DEBOUNCE_MS        50       // Button debounce time
#define SHORT_PRESS_MS     1000     // Max duration for short press
#define LONG_PRESS_MS      3000     // Min duration for long press
#define BLINK_ON_MS        200      // LED on time per blink
#define BLINK_OFF_MS       300      // LED off time per blink
#define BLINK_PAUSE_MS     1000     // Pause after blink sequence

enum TxIdState {
    TXID_IDLE,
    TXID_PRESSED,
    TXID_BLINKING,
    TXID_DISPLAY_MODE
};

class TxIdManager {
public:
    /**
     * @param switchPin  Digital pin for momentary pushbutton (INPUT_PULLUP)
     * @param ledPin     Digital pin for status LED (OUTPUT)
     * @param defaultId  Default TX ID if EEPROM is uninitialized
     */
    TxIdManager(uint8_t switchPin, uint8_t ledPin, uint8_t defaultId = 0);

    /** Initialize pins and load ID from EEPROM */
    void begin();

    /**
     * Call from loop() — handles debounce, press detection, LED blinking.
     * Non-blocking: uses millis(), never calls delay().
     */
    void update();

    /** Get current transmitter ID (0-7) */
    uint8_t getCurrentId() const;

    /** Set TX ID programmatically (also saves to EEPROM) */
    void setId(uint8_t id);

    /** Get current LED state (for external LED management if needed) */
    bool getLedState() const;

    /** Check if a button event just occurred (cleared after reading) */
    bool wasButtonPressed();

private:
    uint8_t  _switchPin;
    uint8_t  _ledPin;
    uint8_t  _txId;
    uint8_t  _defaultId;

    // State machine
    TxIdState _state;
    bool      _ledOn;
    bool      _buttonEvent;

    // Timing
    uint32_t _pressStartMs;
    uint32_t _lastDebounceMs;
    uint32_t _blinkStartMs;
    uint8_t  _blinksRemaining;
    bool     _blinkPhase;  // true = on, false = off

    // Button state
    bool     _lastButtonState;
    bool     _stableButtonState;

    // Internal
    void saveToEEPROM();
    void loadFromEEPROM();
    void startBlinkSequence(uint8_t count);
    void updateBlink();
};

// =============================================================================
// Test stub
// =============================================================================
class TxIdManagerStub {
public:
    TxIdManagerStub(uint8_t defaultId = 0) : _id(defaultId), _pressed(false) {}
    void begin() {}
    void update() {}
    uint8_t getCurrentId() const { return _id; }
    void setId(uint8_t id) { _id = id & 0x07; }
    bool getLedState() const { return false; }
    bool wasButtonPressed() { bool p = _pressed; _pressed = false; return p; }
    void simulatePress() { _id = (_id + 1) & 0x07; _pressed = true; }

private:
    uint8_t _id;
    bool    _pressed;
};

#endif // TX_ID_MANAGER_H

