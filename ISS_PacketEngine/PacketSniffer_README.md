# Davis ISS Packet Sniffer V1.2.1

> Teensy 3.x + RFM69 receiver that captures, validates, and logs
> real Davis Vantage Pro2 / Vue ISS wireless packets.

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| **1.2.1** | 4/30/2026 | **Resync fix** — CRC FAIL no longer disables predictive hopping (was causing 7–30+ minute gaps); `hopCount` and `currentHop` moved to proper globals; resync-after-12-misses now keeps scanning instead of stopping |
| 1.2 | 4/30/2026 | Switchable output modes (Verbose / RAW CSV), packet statistics with CRC pass/fail tracking, multi-day logging with periodic checkpoints, optional timestamp on verbose output |
| 1.1 | 4/27/2026 | Non-blocking status LEDs, serial command processor |
| 1.0 | 4/25/2026 | First working version with correct Davis 6-byte CRC logic |

## Hardware Required

| Component | Part | Connection |
|-----------|------|------------|
| MCU | Teensy 3.2 (or 3.5/3.6) | USB to host PC |
| Radio | RFM69HCW 915 MHz | SPI: CS=10, MOSI=11, MISO=12, SCK=13 |
| Antenna | 915 MHz quarter-wave | RFM69 ANT pin |
| Green LED | Standard + 220Ω resistor | Pin 14 (CRC OK) |
| Yellow LED | Standard + 220Ω resistor | Pin 15 (Missed packet) |
| Red LED | Standard + 220Ω resistor | Pin 16 (Resync) |
| Reset | RFM69 RST | Pin 3 |
| Interrupt | RFM69 DIO0 | Pin 2 |

## Wiring

```
  Teensy 3.2          RFM69HCW
  ──────────          ────────
  Pin 10  ──────────  NSS (CS)
  Pin 11  ──────────  MOSI
  Pin 12  ──────────  MISO
  Pin 13  ──────────  SCK
  Pin 2   ──────────  DIO0 (IRQ)
  Pin 3   ──────────  RST
  3.3V    ──────────  VCC (3.3V ONLY!)
  GND     ──────────  GND

  Pin 14  ──[220Ω]──  Green LED (+) ── GND
  Pin 15  ──[220Ω]──  Yellow LED (+) ── GND
  Pin 16  ──[220Ω]──  Red LED (+) ── GND
```

> ⚠️ **The RFM69HCW is a 3.3V device.** Do NOT connect VCC to 5V.

## Usage

1. Flash `Davis_ISS_PacketSniffer_V1.ino` to Teensy
2. Open Arduino Serial Monitor at **115200 baud**
3. Wait for sync (may take up to a few minutes)
4. Packets will appear as they are received

## Serial Commands

Type a character in Serial Monitor to execute:

### Output & Statistics (uppercase)

| Cmd | Function | Description |
|-----|----------|-------------|
| `V` | Verbose mode | Human-readable output (default) |
| `R` | Raw CSV mode | Machine-parseable for Python decoder and multi-day logging |
| `P` | Statistics | Show uptime, CRC pass/fail rates, missed hops, pkts/min |
| `T` | Timestamp | Toggle elapsed time prefix on verbose output |
| `C` | Clear stats | Reset all counters to zero |

### Radio & Debug (lowercase)

| Cmd | Function | Description |
|-----|----------|-------------|
| `?` | Help | List all available commands |
| `r` | Resync | Force re-acquisition of ISS hop sequence |
| `h` | Hop info | Show current hop channel and frequency |
| `s` | Radio status | RSSI, mode register, IRQ flags |
| `d` | Register dump | Print all RFM69 register values |
| `i` | ISS info | Station ID, packet interval, current output mode |

## Output Formats

### Verbose Mode (default — type `V`)

Human-readable, one line per packet:

```
Channel=15 RSSI=-72 CRC OK  80 08 A1 2F C0 00 F6 6C FF FF
Channel=16 RSSI=-68 CRC OK  E0 08 A1 00 01 00 26 E6 FF FF
```

With elapsed timestamp enabled (type `T`):

```
[00:15:30] Channel=15 RSSI=-72 CRC OK  80 08 A1 2F C0 00 F6 6C FF FF
```

### Raw CSV Mode (type `R`)

Machine-parseable output designed for the Python decoder and long-term logging:

```
millis,hop,rssi,crc,b0,b1,b2,b3,b4,b5,b6,b7,type
1234567,15,-72,1,80,08,A1,2F,C0,00,F6,6C,8
1237089,16,-68,1,E0,08,A1,00,01,00,26,E6,E
```

Periodic checkpoint lines (every 5 minutes):

```
#STATS,uptime_s=7200,pass=3198,fail=43,missed=127,pass_pct=98.7
```

## Multi-Day Monitoring

The RAW CSV mode is designed for unattended, multi-day monitoring:

1. Connect sniffer via USB
2. Open a terminal with file logging (PuTTY, `screen`, or Arduino Serial Monitor)
3. Type `R` to switch to RAW CSV mode
4. Let it run — the checkpoint lines provide health-at-a-glance without stopping capture
5. Feed the captured CSV into the Python decoder:

```bash
python iss_packet_decoder.py --file capture.csv --csv decoded.csv --verbose
```

## CRC Validation

The Davis ISS packet is 8 bytes:

```
Bytes 0-5 : Payload (6 bytes)
Bytes 6-7 : CRC-CCITT (big-endian)
```

The CRC is computed over bytes 0–5 ONLY. A CRC of exactly 0x0000 is invalid.

```c
uint16_t calc = radio.crc16_ccitt(packet, 6);
uint16_t recv = (packet[6] << 8) | packet[7];
bool davisCrcOk = (calc == recv) && (calc != 0);
```

> **Classic gotcha:** Computing CRC over all 8 bytes instead of just the first 6
> will produce `CRC FAIL` on every packet. This is the #1 mistake in Davis
> packet implementations.

The DavisRFM69 library computes CRC bit-by-bit (no lookup table) — this was
verified correct against real Davis ISS packets and served as the ground truth
reference during the CRC table audit (see main README for details).

## Bug Fix Notes

### V1.2.1 — Resync Logic Fix

**Symptom:** After any single CRC failure, the sniffer stopped following the
ISS hop sequence and sat on one channel for 7–30+ minutes until the ISS
randomly landed on it.

**Root cause:** In the `receiveDone()` block, `hopCount` was set to
`davisCrcOk ? 1 : 0`. On CRC failure, `hopCount = 0` disabled all predictive
hopping (the hop timer requires `hopCount > 0`).

**Evidence from bench testing:**

```
18:26:47 Ch 36 CRC OK   ← In sync, hopCount=1
18:26:50 Ch 37 CRC FAIL ← hopCount set to 0 — stuck!
18:33:44 Ch 38 CRC FAIL ← 7 min gap — noise, not ISS
18:43:32 Ch 39 CRC FAIL ← 10 min gap — noise
19:12:39 Ch 40 CRC OK   ← 29 min later — ISS finally landed here
```

**Fix (two changes):**

1. After any `receiveDone()`: always set `hopCount = 1` (the ISS has
   already hopped to the next channel regardless of CRC outcome)
2. In the resync block (after 12 consecutive misses): set `hopCount = 1`
   instead of `hopCount = 0` (keep scanning, don't stop)

**Additional:** `hopCount` and `currentHop` were local statics inside
`loop()` — moved to proper globals so the command processor (`h` command)
can access them.

## Dependencies

- Arduino IDE 2.x with Teensyduino
- DavisRFM69 library (included in this directory — frozen, do not modify)
- Teensy 3.x board support

## Python Companion Tool

The packet decoder in `tools/iss_packet_decoder.py` can:

- Decode individual packets from the command line
- Parse verbose and RAW CSV output files
- Run 33-vector self-test including real ISS packet validation
- Cross-validate Packet Engine output against sniffer captures
- Export decoded results to CSV

See `tools/` directory and the main README for usage.

## License

MIT — see [LICENSE](../LICENSE)
