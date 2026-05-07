# ISS Packet Engine — Test Program
drafted by copilot 

> **Dual-Teensy bench harness for validating ISS packet generation,
> VP2 console decoding, and end-to-end CRC integrity.**
> It is basically a Davis ISS transmitter without the RF!
---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Hardware Requirements](#hardware-requirements)
4. [Firmware Build Configuration](#firmware-build-configuration)
5. [Testing Setup](#testing-setup)
   - [Wiring the Dual-Teensy Harness](#wiring-the-dual-teensy-harness)
   - [USB Serial / COM Port Configuration](#usb-serial--com-port-configuration)
   - [Python Bridge Script](#python-bridge-script)
6. [Packet Format Reference](#packet-format-reference)
7. [Verifying Packet Flow](#verifying-packet-flow)
8. [CRC Validation](#crc-validation)
9. [Packet Decoder & Ground-Truth Validation](#packet-decoder--ground-truth-validation)
10. [Troubleshooting](#troubleshooting)
11. [Appendix A — CRC-CCITT Reference Implementation](#appendix-a--crc-ccitt-reference-implementation)
12. [Appendix B — Quick-Start Checklist](#appendix-b--quick-start-checklist)
13. [Appendix C — Transmit Sequence Reference](#appendix-c--transmit-sequence-reference)

---

## Overview

This test program validates the **ISS Packet Engine** (`ISS_packetEngine.ino`)
in isolation from RF hardware by wiring two Teensy boards back-to-back through
their hardware serial ports and bridging their USB CDC channels to a host PC
running Python monitor scripts.

| Role | Firmware | Serial Config | Purpose |
|------|----------|---------------|---------|
| **Teensy A** (Transmit side) | `ISS_packetEngine.ino` | USB @ 115200, Serial1 TX @ 19200 | Generates Davis-format ISS packets on a 2.5 s schedule |
| **Teensy B** (Receive side) | `VP2_TFTd.ino` | USB @ 115200, Serial1 RX @ 19200 | Decodes incoming packets, validates CRC, renders weather fields |

Two Python tools run on the host PC:

| Script | Purpose |
|--------|---------|
| `iss_bridge.py` | Monitors both USB serial streams, performs independent CRC-CCITT checks, logs pass/fail to CSV |
| `iss_packet_decoder.py` | Decodes individual packets, runs self-tests against DeKay reference vectors, cross-validates Packet Engine output against real Davis ISS sniffer captures |

---

## Architecture

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                              HOST  PC                                    │
│                                                                          │
│  ┌────────────────┐     iss_bridge.py      ┌────────────────┐            │
│  │  COM port A    │◄── USB CDC 115200  ───►│  COM port B    │            │
│  │  (e.g. COM5)   │                        │  (e.g. COM6)   │            │
│  └───────┬────────┘                        └───────┬────────┘            │
│          │  debug + raw hex log                    │  decoded output     │
│          │                                         │                     │
│  ┌───────┴─────────────────────────────────────────┴────────┐            │
│  │            iss_packet_decoder.py (offline)               │            │
│  │    --selftest  |  --file  |  --compare  |  --packet      │            │
│  └──────────────────────────────────────────────────────────┘            │
└──────────┬──────────────────────────────────────────┬────────────────────┘
           │ USB cable                                │ USB cable
┌──────────┴────────────────┐         ┌───────────────┴───────────────────┐
│       TEENSY  A           │         │          TEENSY  B                │
│   ISS_packetEngine.ino    │         │        VP2_TFTd.ino               │
│                           │         │                                   │
│  USB  ← debug @ 115200    │         │  USB  ← decoded output @ 115200   │
│                           │         │                                   │
│  Serial1 TX (pin 1)  ─────┼── wire ─┼─► Serial1 RX (pin 0)              │
│  Serial1 RX (pin 0)  ◄────┼── wire ─┼── Serial1 TX (pin 1)              │
│                           │         │                                   │
│          GND ─────────────┼── wire ─┼── GND                             │
└───────────────────────────┘         └───────────────────────────────────┘
```

**Data flow:**

1. **Teensy A** builds an 8-byte Davis packet every 2.5 seconds and writes
   the raw bytes on `Serial1 TX` (19200 baud, 8N1).
2. The packet travels over a jumper wire to **Teensy B** `Serial1 RX`.
3. Teensy B decodes the packet, validates CRC, and prints decoded weather
   values on its USB CDC serial port.
4. Both boards also echo diagnostic / hex-dump output on their respective
   USB CDC ports back to the host PC.
5. `iss_bridge.py` reads both USB streams, performs its own CRC check on
   every packet, timestamps everything, and writes a unified CSV log.
6. `iss_packet_decoder.py` can later decode captured packets offline,
   run self-tests, or cross-validate against real Davis ISS sniffer data.

---

## Hardware Requirements

| Qty | Item | Notes |
|-----|------|-------|
| 2 | Teensy 4.0 or 4.1 | Any Teensy with hardware Serial1 works (3.2, 3.5, 3.6, 4.0, 4.1) |
| 2 | USB Micro-B (or USB-C) cables | One per Teensy, both connected to the host PC |
| 3 | Jumper wires (F-F or M-M depending on headers) | TX→RX, RX→TX, GND→GND |
| 1 | Breadboard (optional) | Convenient for stable connections |
| 1 | Host PC | Windows, macOS, or Linux with Python 3.8+ and `pyserial` installed |

> **Note:** When building `ISS_packetEngine.ino` with `USE_TEST_STUBS=1`,
> no sensor hardware (BME280, AS5048B, hall-effect switch, analog channels)
> is needed — the firmware uses configurable stub values for all sensors.

---

## Firmware Build Configuration

`ISS_packetEngine.ino` has four compile-time flags that control its
behaviour. Set these at the top of the file before uploading:

| Flag | Default | Description |
|------|---------|-------------|
| `USE_TEST_STUBS` | `0` | Set to `1` for hardware-free bench testing. Stub drivers return realistic fake sensor data. |
| `TX_SEQ_MODE` | `2` | `0` = VP2 sequence, `1` = Vue sequence, `2` = Custom hybrid sequence |
| `SERIAL_DEBUG` | `1` | `1` = verbose hex-dump + decoded output on USB serial (115200) |
| `SERIAL1_OUTPUT` | `1` | `1` = raw 8-byte binary packets on Serial1 TX (19200) for bridge |

### Test Stub Mode

When `USE_TEST_STUBS=1`, the firmware loads default sensor values at startup:

```
Temp=76.4F  RH=45%  Baro=1013.25hPa
Wind=8mph@228deg  Gust=12mph
BattV=3.85V  SolarV=5.10V
```

These values produce known-good packets that are easy to verify against
expected CRC values and decoded fields.

### Serial Port Baud Rates

| Port | Baud | Purpose | Data Format |
|------|------|---------|-------------|
| USB CDC (`Serial`) | **115200** | Debug output: hex dumps, sensor readings, status | Human-readable text lines |
| Hardware Serial1 | **19200** | Bridge link to Teensy B | Raw 8-byte binary packets |

> **Important distinction:** The ISS Packet Engine uses 115200 for USB
> debug output, but 19200 on the Serial1 inter-Teensy link (matching
> the Davis standard serial speed). The bridge script connects to the
> USB CDC ports, which carry the debug text stream — not the raw
> Serial1 binary stream.

---

## Testing Setup

### Wiring the Dual-Teensy Harness

> **Critical: Cross-connect TX↔RX. Never connect TX→TX or RX→RX.**

#### Pin Map (Teensy 4.0 / 4.1 — Serial1)

| Signal | Teensy A Pin | Wire Color (suggested) | Teensy B Pin | Signal |
|--------|-------------|------------------------|-------------|--------|
| Serial1 TX | **Pin 1** | Yellow | **Pin 0** | Serial1 RX |
| Serial1 RX | **Pin 0** | Green | **Pin 1** | Serial1 TX |
| GND | **GND** | Black | **GND** | GND |

#### ASCII Wiring Diagram — Inter-Teensy Serial Link

```text
        TEENSY A                              TEENSY B
   (ISS_packetEngine)                       (VP2_TFTd)
  ┌──────────────────┐                 ┌──────────────────┐
  │             Pin 1├──── yellow ─────┤Pin 0             │
  │  Serial1 TX      │    (TX → RX)    │      Serial1 RX  │
  │                  │                 │                  │
  │              in 0├──── green ──────┤Pin 1             │
  │  Serial1 RX      │    (RX ← TX)    │      Serial1 TX  │
  │                  │                 │                  │
  │              GND ├──── black ──────┤GND               │
  └──────────────────┘                 └──────────────────┘
        │ USB                                  │ USB
        ▼                                      ▼
     [COM5]                                 [COM6]
  115200 baud                            115200 baud
     Host PC ◄──────── iss_bridge.py ────────► Host PC
```

#### Full Test Bench Layout

```text
  ┌─────────────────────────────────────────────────────────────────────┐
  │                         BREADBOARD                                  │
  │                                                                     │
  │    ┌─────────────┐                        ┌─────────────┐           │
  │    │  TEENSY A   │                        │  TEENSY B   │           │
  │    │             │                        │             │           │
  │    │  Pin 1 (TX) ├────────────────────────┤ Pin 0 (RX)  │           │
  │    │             │      yellow wire       │             │           │
  │    │  Pin 0 (RX) ├────────────────────────┤ Pin 1 (TX)  │           │
  │    │             │      green wire        │             │           │
  │    │  GND        ├────────────────────────┤ GND         │           │
  │    │             │      black wire        │             │           │
  │    │  USB ●      │                        │  USB ●      │           │
  │    └──────┼──────┘                        └──────┼──────┘           │
  │           │                                      │                  │
  └───────────┼──────────────────────────────────────┼──────────────────┘
              │ USB cable                            │ USB cable
              ▼                                      ▼
  ┌───────────────────────────────────────────────────────────────┐
  │                        HOST PC                                │
  │                                                               │
  │   COM5 (Teensy A)             COM6 (Teensy B)                 │
  │       │                           │                           │
  │       └──────── iss_bridge.py ────┘                           │
  │                      │                                        │
  │              iss_test_log.csv                                 │
  │                                                               │
  │    (offline analysis: iss_packet_decoder.py --file log.txt)   │
  └───────────────────────────────────────────────────────────────┘
```

#### Wiring Notes

- **Keep wires short** (< 15 cm / 6 in) to minimize noise at 19200 baud.
- **GND is mandatory** — both boards share USB ground through the PC, but an
  explicit GND jumper eliminates ground-loop uncertainty.
- If using a **breadboard**, seat both Teensys and run the three jumper wires
  on adjacent rows for clean routing.
- **Do NOT connect 3.3 V or VIN between boards** — each Teensy is powered
  independently via its own USB cable.

#### Alternate Boards (Teensy 3.x Serial1 Pins)

| Board | Serial1 TX | Serial1 RX |
|-------|-----------|-----------:|
| Teensy 3.2 | Pin 1 | Pin 0 |
| Teensy 3.5 | Pin 1 | Pin 0 |
| Teensy 3.6 | Pin 1 | Pin 0 |
| Teensy 4.0 | Pin 1 | Pin 0 |
| Teensy 4.1 | Pin 1 | Pin 0 |

> Serial1 TX/RX mapping is consistent across the Teensy family (pins 1 and 0).

---

### USB Serial / COM Port Configuration

Both Teensys enumerate as USB CDC serial devices when plugged in. You need to
identify which COM port (Windows) or `/dev/tty*` (macOS/Linux) belongs to
which board.

#### Windows — Identifying COM Ports

1. Open **Device Manager** → **Ports (COM & LPT)**.
2. Plug in **Teensy A only** → note the new COM port (e.g., `COM5`).
3. Plug in **Teensy B** → note the second new COM port (e.g., `COM6`).
4. Label them:
   - `COM5` → Teensy A (`ISS_packetEngine`)
   - `COM6` → Teensy B (`VP2_TFTd`)

> **Tip:** Right-click each port → Properties → Port Settings → Advanced →
> set a fixed COM port number and rename the friendly name to
> `Teensy A - ISS PacketEngine` and `Teensy B - VP2 TFTd` for easy
> identification across reboots.

#### macOS — Identifying Ports

```bash
# List all Teensy USB serial ports
ls /dev/tty.usbmodem*

# Plug in one at a time to identify which is which
# Example result:
#   /dev/tty.usbmodem14201  →  Teensy A
#   /dev/tty.usbmodem14301  →  Teensy B
```

#### Linux — Identifying Ports

```bash
# List all Teensy USB serial ports
ls /dev/ttyACM*

# Use udevadm for detailed identification
udevadm info -a /dev/ttyACM0 | grep serial

# Create persistent symlinks (optional — add to /etc/udev/rules.d/)
# SUBSYSTEM=="tty", ATTRS{serial}=="12345678", SYMLINK+="teensy_a"
```

#### Serial Port Settings — USB CDC (Host ↔ Teensy)

The USB CDC ports carry **debug text output** from the firmware.
Configure your terminal / bridge script as follows:

| Parameter | Value |
|-----------|-------|
| Baud rate | **115200** (matches `Serial.begin(115200)` in firmware) |
| Data bits | **8** |
| Parity | **None** |
| Stop bits | **1** |
| Flow control | **None** |

> **Note:** Teensy USB CDC ignores the baud rate setting (USB runs at
> native speed), but `pyserial` requires you to specify one. Use
> **115200** for consistency with the firmware's `Serial.begin()` call.
> The bridge script defaults to 19200 in its config — **update this to
> 115200** if you want to match the firmware's USB debug baud rate, or
> leave it since Teensy CDC ignores it anyway.

#### Serial Port Settings — Hardware Serial1 (Teensy A ↔ Teensy B)

The inter-Teensy serial link carries **raw 8-byte binary packets**:

| Parameter | Value |
|-----------|-------|
| Baud rate | **19200** (Davis standard) |
| Data bits | **8** |
| Parity | **None** |
| Stop bits | **1** |
| Flow control | **None** |

This link is firmware-configured (`Serial1.begin(19200)`) and requires
no host-side setup — the jumper wires carry the signal directly between
boards.

---

### Python Bridge Script

The bridge script (`iss_bridge.py`) monitors both USB CDC ports
simultaneously, performs independent CRC validation on every packet,
and produces a unified log for analysis.

#### Prerequisites

```bash
pip install pyserial
```

> **Caution:** Install `pyserial`, not the similarly-named `serial`
> package. If you have both installed, remove `serial` first:
> `pip uninstall serial && pip install pyserial`

#### Configuration — `bridge_config.json`

Create a configuration file in the same directory as the bridge script:

```json
{
    "teensy_a": {
        "port": "COM5",
        "label": "ISS_packetEngine",
        "role": "transmitter"
    },
    "teensy_b": {
        "port": "COM6",
        "label": "VP2_TFTd",
        "role": "receiver"
    },
    "serial": {
        "baudrate": 115200,
        "bytesize": 8,
        "parity": "N",
        "stopbits": 1,
        "timeout": 2.0
    },
    "logging": {
        "log_file": "iss_test_log.csv",
        "console_verbosity": "verbose",
        "log_raw_hex": true
    }
}
```

> **Platform-specific port names:**
>
> | OS | Teensy A (example) | Teensy B (example) |
> |----|--------------------|--------------------|
> | Windows | `COM5` | `COM6` |
> | macOS | `/dev/tty.usbmodem14201` | `/dev/tty.usbmodem14301` |
> | Linux | `/dev/ttyACM0` | `/dev/ttyACM1` |

#### Running the Bridge

```bash
# Default configuration (bridge_config.json in current directory)
python iss_bridge.py

# Custom config file
python iss_bridge.py my_test_config.json
```

#### Expected Console Output

```text
[ISS_packetEngine] Opened COM5 @ 115200
[VP2_TFTd] Opened COM6 @ 115200

============================================================
  ISS Packet Engine — Test Bridge Running
============================================================
  Teensy A: COM5  (ISS_packetEngine)
  Teensy B: COM6  (VP2_TFTd)
  Baud:     115200
  Log:      iss_test_log.csv
============================================================
  Press Ctrl+C to stop.

  [ISS_packetEngine] #1 [0/19] Temp  PKT: 80 08 E4 30 40 00 A2 1F  ID=0 Wind=8mph@228deg
  [VP2_TFTd]         PKT: 80 08 E4 30 40 00 A2 1F  CRC OK
  [VP2_TFTd]         Temp: 76.4 F  Wind: 228 deg  Speed: 8 mph
  [STATS] 2 packets | 2 pass | 0 fail | 100.0% CRC pass rate
```

#### Log File Format

The bridge writes a CSV log (`iss_test_log.csv`) with the following columns:

| Column | Description |
|--------|-------------|
| `timestamp` | ISO 8601 with milliseconds |
| `source` | `ISS_packetEngine` or `VP2_TFTd` |
| `type` | `debug` (text line) or `packet` (8-byte packet) |
| `raw_hex` | Space-separated hex bytes |
| `crc_valid` | `True` or `False` |
| `crc_expected` | CRC computed by the bridge |
| `crc_received` | CRC bytes extracted from the packet |
| `decoded` | Decoded field values (if available) |

#### How the Bridge Works Internally

1. **Two `TeensyReader` threads** — one per COM port — read serial data
   continuously using `pyserial`.
2. Each thread buffers incoming bytes and processes them two ways:
   - **Newline-delimited text** (debug output) → logged as `debug` type
   - **Raw 8-byte blocks** (binary packets) → logged as `packet` type
     with CRC validation
3. The main thread prints a **stats summary** every 5 seconds showing
   total packets, CRC pass/fail counts, and pass rate percentage.
4. All data is flushed to the CSV after every packet for crash resilience.

---

## Packet Format Reference

The ISS Packet Engine generates standard Davis 8-byte packets:

```text
Byte:   [0]      [1]      [2]      [3]      [4]      [5]      [6]      [7]
        +--------+--------+--------+--------+--------+--------+--------+--------+
        | Header | Wind   | Wind   | Data   | Data   | Data   | CRC    | CRC    |
        | / Type | Speed  | Dir    | High   | Low    | Flags  | High   | Low    |
        +--------+--------+--------+--------+--------+--------+--------+--------+
```

### Byte 0 — Header Breakdown

```text
  Bit:  7  6  5  4   3     2  1  0
       ├──────────┤  ├──┤  ├────────┤
       Packet Type  Batt   TX ID
       (4 bits)     Low    (3 bits)
                    Flag
```

| Field | Bits | Description |
|-------|------|-------------|
| Packet type | `[7:4]` | Upper nibble — identifies the sensor data category |
| Battery low | `[3]` | `1` = transmitter battery is low |
| TX ID | `[2:0]` | Transmitter ID (0–7) |

### Bytes 1–2 — Wind (Always Present)

| Byte | Field | Formula |
|------|-------|---------|
| 1 | Wind Speed | Raw value in mph |
| 2 | Wind Direction | `degrees = byte × 360 / 255`; 0x00 = no reading |

### Bytes 3–5 — Sensor-Specific Data

### Packet Types (All 10 Types from ISS Packet Engine)

| Nibble | Type Name | Data Encoding (Bytes 3–5) |
|--------|-----------|---------------------------|
| `0x2` | Supercap / Battery Voltage | `raw = (byte3 << 2) \| (byte4 >> 6)`; voltage = raw / 100.0 V |
| `0x3` | Lightning (reserved) | byte3 = distance km (6-bit), byte4 = energy, byte5 bit 7 = event flag |
| `0x4` | UV Index | `UV = ((byte3 << 8 \| byte4) >> 6) / 50.0`; byte3=0xFF → no sensor |
| `0x5` | Rain Rate | Seconds since last bucket tip (encoded in bytes 3–4); 0xFF,0x71 = no rain |
| `0x6` | Solar Radiation | `W/m² = ((byte3 << 8 \| byte4) >> 6) × 1.757936`; byte3=0xFF → no sensor |
| `0x7` | PV (Solar Cell) Voltage | Same encoding as 0x2 — solar panel voltage |
| `0x8` | Temperature | `temp_F = ((int16_t)(byte3 << 8 \| byte4)) / 160.0` (signed) |
| `0x9` | Wind Gust | byte3 = gust speed mph, byte5 = gust direction (same as byte 2) |
| `0xA` | Humidity | `RH% = (((byte4 >> 4) & 0x03) << 8 \| byte3) / 10.0` |
| `0xE` | Rain | byte3 = running bucket tip counter (wraps 0–255, each tip = 0.01 in) |

> Packet types rotate through a 20-slot transmit sequence. Each slot
> fires every 2.5 seconds, so the full sequence takes 50 seconds.
> Temperature and humidity appear multiple times per cycle; UV and
> solar appear once.

### Bytes 6–7 — CRC

CRC-CCITT (polynomial 0x1021, initial value 0x0000) computed over bytes
0–5, stored big-endian.

---

## Verifying Packet Flow

### Step-by-Step Verification

1. **Configure firmware for bench testing**

   In `ISS_packetEngine.ino`, set:
   ```cpp
   #define USE_TEST_STUBS    1    // No hardware needed
   #define TX_SEQ_MODE       2    // Custom hybrid sequence
   #define SERIAL_DEBUG      1    // Verbose USB output
   #define SERIAL1_OUTPUT    1    // Raw packets on Serial1
   ```

2. **Flash firmware**
   - Upload `ISS_packetEngine.ino` to Teensy A
   - Upload `VP2_TFTd.ino` to Teensy B

3. **Wire the harness** (see [Wiring](#wiring-the-dual-teensy-harness) above)

4. **Verify Teensy A output** (quick manual check before running the bridge)

   ```bash
   # Open a serial monitor on Teensy A
   python -m serial.tools.miniterm COM5 115200
   ```

   You should see initialization output followed by packets every 2.5 seconds:

   ```text
   ==============================================
     ISS Packet Engine v2.0
     TX Sequence: Custom
     Test Stubs:  ENABLED (no hardware)
   ==============================================

   Initializing sensors:
     BME280............. OK
     Wind Speed......... OK
     Wind Direction..... OK
     TX ID Manager...... OK  (ID=0)

     Stub values loaded:
       Temp=76.4F  RH=45%  Baro=1013.25hPa
       Wind=8mph@228deg  Gust=12mph
       BattV=3.85V  SolarV=5.10V

   Packet engine running. Interval: 2.5s
   #1 [0/19] Temp   PKT: 80 08 E4 30 40 00 A2 1F  ID=0 Wind=8mph@228deg
   #2 [1/19] Humid  PKT: A0 08 E4 C2 10 00 B3 2C  ID=0 Wind=8mph@228deg
   ```

5. **Verify Teensy B output** (close Teensy A miniterm first)

   ```bash
   python -m serial.tools.miniterm COM6 115200
   ```

   Teensy B should print decoded weather values for each received packet.

6. **Run the bridge** for automated logging and CRC validation:

   ```bash
   python iss_bridge.py
   ```

7. **Verify end-to-end**
   - Confirm the same packet hex appears in both Teensy A (sent) and
     Teensy B (received) logs.
   - Confirm **100% CRC pass rate** under normal conditions.
   - Confirm decoded values (temperature, wind, etc.) match the test
     stub values programmed in the firmware.

### What to Look For

| Indicator | Healthy | Problem |
|-----------|---------|---------|
| Packets from Teensy A | Regular interval (~2.5 s) | Missing or irregular → check `SERIAL1_OUTPUT` is `1` |
| Packets from Teensy B | Matches Teensy A hex exactly | Missing → check wiring (TX/RX crossed?) |
| CRC pass rate | 100% | < 100% → noise on wires, baud mismatch, or framing error |
| Decoded values | Match test stub values | Wrong values → packet type parsing bug |
| Sequence index | Cycles 0–19 then wraps | Stuck at 0 → `TX_SEQ_LENGTH` not defined or wrong |
| Battery low flag | `0` (unless testing) | Unexpected `1` → check battery voltage threshold |

---

## CRC Validation

### How Davis CRC-CCITT Works

Davis weather stations use **CRC-CCITT (polynomial 0x1021)** with an initial
value of **0x0000**. The CRC is computed over bytes 0–5 of each packet and
appended as bytes 6–7 (big-endian).

**Validation method:** Compute CRC-CCITT over all 8 bytes (including the
appended CRC). A valid packet always yields a remainder of **0x0000**.

**Additional Davis convention:** A CRC value of exactly 0x0000 is treated as
invalid (per the packet decoder's `validate_crc` function: `calc == recv &&
calc != 0`).

### Quick CRC Test (Python)

```python
from iss_bridge import crc_ccitt, validate_packet_crc

# Known good packet (Temperature, stub values)
pkt = bytes([0x80, 0x08, 0xE4, 0x30, 0x40, 0x00, 0xA2, 0x1F])

# Method 1: Validate entire packet (should be True)
assert validate_packet_crc(pkt), "CRC validation failed!"

# Method 2: Compute CRC of payload and compare
payload_crc = crc_ccitt(pkt[:6])
received_crc = int.from_bytes(pkt[6:8], "big")
assert payload_crc == received_crc, f"Mismatch: {payload_crc:#06x} != {received_crc:#06x}"

print("CRC validation passed ✓")
```

### Firmware-Side CRC (Arduino / Teensy)

The firmware uses a PROGMEM lookup table for fast CRC computation:

```cpp
uint16_t crc_ccitt(const uint8_t *buf, uint8_t len) {
    uint16_t crc = 0x0000;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t idx = ((crc >> 8) ^ buf[i]) & 0xFF;
        uint16_t tval;
        memcpy_P(&tval, &CRC_TABLE[idx], sizeof(uint16_t));
        crc = (crc << 8) ^ tval;
    }
    return crc;
}

// Append CRC to a 6-byte payload → 8-byte packet
void appendCRC(uint8_t *pkt) {
    uint16_t crc = crc_ccitt(pkt, 6);
    pkt[6] = (crc >> 8) & 0xFF;   // CRC high byte
    pkt[7] = crc & 0xFF;          // CRC low byte
}
```

---

## Packet Decoder & Ground-Truth Validation

The `iss_packet_decoder.py` tool provides five operating modes for
validating Packet Engine output:

### Mode 1 — Self-Test (DeKay Reference Vectors)

Run the built-in test suite to verify decode formulas against
published DeKay reference packets from real Davis ISS hardware:

```bash
python iss_packet_decoder.py --selftest
```

Expected output:

```text
  SELF-TEST: Validating decoder against DeKay reference vectors

  Test 1: DeKay STRMON example (Solar Radiation, no sensor)
    PASS: CRC valid = True
    PASS: msg_type = 6
    PASS: wind_speed_mph = 6
    PASS: wind_dir_deg = 297.6
    PASS: solar_sensor = NOT PRESENT

  Test 2: DeKay temperature example (25.0 F)
    PASS: temp_f = 25.0

  Test 3: DeKay humidity example (89.9%)
    PASS: humidity_pct = 89.9

  SELF-TEST RESULTS: 20/20 passed -- ALL PASS
```

### Mode 2 — Decode a Single Packet

```bash
python iss_packet_decoder.py --packet "80 08 E4 30 40 00 A2 1F"
```

### Mode 3 — Decode from Capture File

```bash
# Save bridge output to a text file, then decode
python iss_packet_decoder.py --file iss_test_log.txt --verbose

# Export decoded results to CSV
python iss_packet_decoder.py --file iss_test_log.txt --csv decoded.csv
```

### Mode 4 — Cross-Validate Against Sniffer Ground Truth

Compare Packet Engine output against real packets captured from a
Davis ISS using a PacketSniffer receiver:

```bash
python iss_packet_decoder.py --compare engine_output.txt --truth sniffer_capture.txt
```

This mode checks:
- CRC pass rates for both sources
- Packet type coverage (does the engine produce all types the real ISS does?)
- Field encoding match (bit positions, formulas)
- Value range plausibility

### Mode 5 — Live Serial from PacketSniffer

```bash
python iss_packet_decoder.py --serial COM7 --baud 115200
```

### Supported Input Formats

The decoder's flexible parser handles multiple sniffer output formats:

```text
50 00 D3 FF C0 00 78 75
PKT: 50 00 D3 FF C0 00 78 75 CRC OK
Ch:25 50 00 D3 FF C0 00 78 75 RSSI:-65
0=60 1=06 2=D3 3=FF 4=C0 5=00 6=78 7=75
```

---

## Troubleshooting

### Serial Communication Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No COM port appears | Teensy not recognized by OS | Install Teensy USB drivers; try a different USB cable (data cable, not charge-only) |
| COM port appears but no data | Serial not initialized in firmware | Verify `Serial.begin(115200)` and `Serial1.begin(19200)` in both sketches |
| Garbage characters on USB monitor | Baud rate mismatch | Use **115200** for USB CDC monitor (not 19200) |
| Teensy A sends but Teensy B is silent | TX/RX wiring reversed or missing GND | Verify Pin 1 (TX) → Pin 0 (RX) cross-connection; check GND jumper |
| Intermittent CRC failures | Noise or loose jumper wires | Shorten wires; reseat connections; add 0.1 µF cap across RX line if persistent |
| Python script can't open port | Port in use by another application | Close Arduino Serial Monitor, PuTTY, or any other serial terminal before running bridge |
| `PermissionError` on Linux/macOS | User not in `dialout` group | `sudo usermod -aG dialout $USER` then log out/in |

### COM Port Assignment Changes

Windows may reassign COM port numbers after:
- Using a different USB port
- Reinstalling drivers
- Windows Update

**Fix:** Update `bridge_config.json` with the new port numbers. Use
Device Manager to verify current assignments.

**Persistent port assignment (Windows):**
1. Device Manager → Ports → right-click the Teensy port
2. Properties → Port Settings → Advanced
3. Set a fixed COM port number (e.g., COM5, COM6)

### Python Bridge Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `ModuleNotFoundError: serial` | `pyserial` not installed (or `serial` conflict) | `pip uninstall serial && pip install pyserial` |
| Bridge runs but shows 0 packets | Config points to wrong COM port | Verify ports in `bridge_config.json` |
| Bridge connects but only shows debug lines, no packets | Firmware `SERIAL1_OUTPUT` is `0` | Set `SERIAL1_OUTPUT` to `1` and re-flash Teensy A |
| CRC failures on USB stream but not Serial1 | Bridge parsing debug text as binary | Ensure the bridge is extracting hex tokens from debug lines, not treating text as raw bytes |
| Timestamps drift between Teensy A and B logs | Normal — USB CDC latency varies | Not a bug; the bridge timestamps host-side arrival, not transmission |
| CSV log file is empty | Script crashed before flushing | Check console for error messages |

### Firmware Debugging Tips

- **Loopback test** — temporarily connect Teensy A's TX back to its own RX
  to verify it can send and receive its own packets:
  ```text
  Teensy A Pin 1 (TX) ──── wire ──── Teensy A Pin 0 (RX)
  ```

- **Verify test stub values** — with `USE_TEST_STUBS=1`, the firmware prints
  loaded stub values at startup. Cross-reference these against the packets:

  | Stub Value | Expected Packet Type | Expected Encoding |
  |------------|---------------------|-------------------|
  | Temp=76.4 F | 0x80 | raw = 76.4 × 160 = 12224 = 0x2FC0 |
  | RH=45.0% | 0xA0 | raw = 450 = 0x01C2; byte3=0xC2, byte4=0x10 |
  | Wind=8 mph | All packets, byte 1 | 0x08 |
  | Wind dir=228° | All packets, byte 2 | 228 × 255 / 360 ≈ 161 = 0xA1 |

- **Logic analyzer** — if issues persist, use a logic analyzer on the
  Serial1 TX/RX lines to verify signal integrity, baud rate accuracy,
  and frame timing at 19200 baud.

- **Enable/disable debug output** — toggle `SERIAL_DEBUG` to reduce USB
  output if the debug text is overwhelming the bridge parser.

### Packet Decoder Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Self-test reports failures | Decoder formula mismatch | Check `_decode_sensor_data()` against DeKay's protocol.txt |
| `--compare` shows "MISSING" types | Packet Engine not generating that type | Check `TX_SEQ_MODE` — try mode 2 (Custom) for full coverage |
| Parser skips valid packet lines | Line format not recognized | Check the regex parser; add format support if using a custom sniffer output |

---

## Appendix A — CRC-CCITT Reference Implementation

Full 256-entry lookup table generator for CRC-CCITT (0x1021), initial
value 0x0000:

```python
def build_crc_table():
    """Generate the full CRC-CCITT lookup table."""
    table = []
    for i in range(256):
        crc = i << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1
            crc &= 0xFFFF
        table.append(crc)
    return table

# Print table for embedding in firmware (C/C++ format)
table = build_crc_table()
print("static const uint16_t CRC_TABLE[256] PROGMEM = {")
for i in range(0, 256, 8):
    row = ", ".join(f"0x{table[j]:04X}" for j in range(i, i + 8))
    comma = "," if i + 8 < 256 else ""
    print(f"    {row}{comma}")
print("};")
```

---

## Appendix B — Quick-Start Checklist

```text
Prerequisites:
  [ ] Install Python 3.8+ and pyserial (pip install pyserial)
  [ ] Install Arduino IDE / PlatformIO with Teensyduino support

Firmware:
  [ ] Set USE_TEST_STUBS=1 in ISS_packetEngine.ino (for bench testing)
  [ ] Set SERIAL_DEBUG=1 and SERIAL1_OUTPUT=1
  [ ] Flash ISS_packetEngine.ino to Teensy A
  [ ] Flash VP2_TFTd.ino to Teensy B

Wiring:
  [ ] Wire: Teensy A Pin 1  →  Teensy B Pin 0  (TX to RX)
  [ ] Wire: Teensy A Pin 0  ←  Teensy B Pin 1  (RX from TX)
  [ ] Wire: Teensy A GND    →  Teensy B GND
  [ ] Do NOT connect 3.3V or VIN between boards

Host PC:
  [ ] Connect both Teensys via USB to host PC
  [ ] Identify COM ports in Device Manager (or /dev/tty*)
  [ ] Create bridge_config.json with correct port assignments
  [ ] Set baudrate to 115200 in bridge_config.json

Verification:
  [ ] Run: python iss_bridge.py
  [ ] Verify packets appear from both Teensy A and Teensy B
  [ ] Verify 100% CRC pass rate
  [ ] Verify decoded weather values match test stub values
  [ ] Run: python iss_packet_decoder.py --selftest
  [ ] Save iss_test_log.csv for regression analysis
```

---

## Appendix C — Transmit Sequence Reference

The ISS Packet Engine rotates through a 20-slot transmit sequence.
Each slot fires every 2.5 seconds (full cycle = 50 seconds).

### Custom Hybrid Sequence (`TX_SEQ_MODE=2`)

| Slot | Type | Hex | Sensor |
|------|------|-----|--------|
| 0 | Temperature | 0x80 | BME280 |
| 1 | Humidity | 0xA0 | BME280 |
| 2 | Rain | 0xE0 | Bucket counter |
| 3 | Rain Rate | 0x50 | Timer |
| 4 | Temperature | 0x80 | BME280 |
| 5 | Wind Gust | 0x90 | Hall-effect peak |
| 6 | Supercap / BattV | 0x20 | Analog (A1) |
| 7 | PV Voltage | 0x70 | Analog (A0) |
| 8 | Temperature | 0x80 | BME280 |
| 9 | Humidity | 0xA0 | BME280 |
| 10 | Rain | 0xE0 | Bucket counter |
| 11 | Rain Rate | 0x50 | Timer |
| 12 | Temperature | 0x80 | BME280 |
| 13 | Wind Gust | 0x90 | Hall-effect peak |
| 14 | UV Index | 0x40 | Future sensor |
| 15 | Solar Radiation | 0x60 | Future sensor |
| 16 | Temperature | 0x80 | BME280 |
| 17 | Humidity | 0xA0 | BME280 |
| 18 | Lightning | 0x30 | Future AS3935 |
| 19 | Rain | 0xE0 | Bucket counter |

> Wind speed and direction are transmitted in **every** packet
> (bytes 1 and 2), regardless of the slot type.

---

*Document version: 2.0 — April 2026*
*ISS Packet Engine Test Program — Dual-Teensy Bench Harness*
*Reference: DeKay @ madscientistlabs.blogspot.com, VPTools/AnemometerTX by kobuki*
