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
//new adds Davis style button behaviour: 
#ifndef TX_ID_MANAGER_H
#define TX_ID_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

#define TXID_EEPROM_ADDR   0
#define TXID_MIN           0
#define TXID_MAX           7

// Timing (ms)
#define DEBOUNCE_MS        50
#define LONG_PRESS_MS      2000
#define BLINK_INTERVAL_MS  300

class TxIdManager {
public:
    TxIdManager(uint8_t pinButton, uint8_t pinLED, uint8_t defaultId = 0);
    bool wasButtonPressed();    //Not used? added for compatibility
    void begin();
    void update();

    uint8_t getCurrentId() const { return _currentId; }
    void flashTx();

private:
    enum Mode { NORMAL, PRESSING, PROGRAM_FLASH, TAP_COUNT, CONFIRM };
    Mode _mode;

    uint8_t _pinButton, _pinLED;
    uint8_t _currentId;
    uint8_t _tapCount;
    uint8_t _confirmCount;

    uint32_t _pressStart;
    uint32_t _lastTapTime;
    uint32_t _flashTimer;

    bool _flashState;
    bool _lastButtonState;

    void blinkOnce();
    void finalizeId();
    void loadFromEEPROM();
    void saveToEEPROM();
};

#endif
