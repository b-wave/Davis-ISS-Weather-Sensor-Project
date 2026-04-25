# Davis ISS Packet Sniffer  
**REV: 042526 — VERS: 1.0**

A Teensy‑based packet sniffer for the Davis Instruments ISS (Integrated Sensor Suite).  
This tool receives, validates, and displays raw ISS packets using an RFM69 radio configured to match Davis VP/VP2 modulation and hop behavior.

This project is part of a larger effort to build a fully compatible Davis‑style transmitter, including future extensions such as lightning detection.

---

## Features

### Accurate Davis ISS Packet Reception
- Correct sync word and modulation
- Correct hop table and hop timing
- Correct CRC16‑CCITT validation (6‑byte payload CRC)

### Clean receiver architecture
- Mashup of DeKay,kaboui and b-wave (VP2_TFT) 
- No prediction
- No timing estimator
- No repeated RX resets
- Only real ISS packets (SYNC + CRC)

###  Non‑blocking LED status indicators
- **Green** — Valid packet (CRC OK)  
- **Yellow** — Missed packet  
- **Red** — Resync event  

###  Expandable UI command system (coming in Phase B)
- Channel stepping  
- Channel‑locked vs all‑channels mode  
- Register dump  
- Stats collection  
- Packet filtering  
- Timing diagnostics  

---

## Hardware Requirements

- Teensy (3.x or 4.x recommended)
- RFM69W or RFM69HW module  
- Standard SPI wiring  
- Optional: 3 LEDs for status indicators  

---

## Software Requirements

- Arduino IDE or PlatformIO  
- `DavisRFM69` library (included in this repo)  
- Teensyduino (if using Teensy boards)  

---

## Packet Format (Davis ISS)

| Byte | Meaning |
|------|---------|
| 0–5  | Payload (6 bytes) |
| 6–7  | CRC16‑CCITT (big‑endian) |

CRC is computed over **bytes 0–5 only**.

---

## CRC Validation

```cpp
uint16_t calc = radio.crc16_ccitt(packet, 6);
uint16_t recv = (packet[6] << 8) | packet[7];
bool davisCrcOk = (calc == recv) && (calc != 0);



