
---
# Davis ISS Packet Sniffer  
A Teensy‑based packet sniffer for the Davis Instruments ISS (Integrated Sensor Suite).  

Purpose:
This tool receives, validates, and displays raw ISS packets using an RFM69 radio configured to match Davis VP/VP2 modulation and hop behavior.
This project is part of a larger effort to build a fully compatible Davis‑style transmitter, including future extensions such as lightning detection.

CHANGE LOG:

 --
**REV:   050326 VERS:  1.2.2**
 V1.2.2 (050326) 
 — Radio-lockup fix, noise guard, station filter
   - FIX: radio.setChannel() now ALWAYS called after receiveDone()
          to prevent radio staying in STANDBY mode (caused 15+ min lockups)
   - FIX: Noise guard — CRC FAIL packets ignored during park mode
          (hopCount==0). Only CRC OK can break out of park-and-wait.
          During tracking (hopCount>0), CRC FAIL still follows ISS.
   - FIX: Station ID filter — rejects packets from wrong station during
          park mode so multi-ISS environments don't cross-lock.
   - FIX: Resync (hopCount>12) now returns immediately after parking
          to prevent unnecessary channel advance.
   - ADD: Station change command (1-8) — switch stations without recompile.
   - FIX: 'i' command now shows actual PACKET_INTERVAL, not hardcoded value.
   - FIX: 'r' resync command properly resets to park-and-wait mode.
---
**REV: 042726 — VERS: 1.1**
Notes:
   - Fully funcional
   - Added non-blocking Status LEDs
      - Good Packet +CRC Check = Green LED
      - CRC Error = Yellow LED
      - Missed packet = Yellow LED
      - Re-Sync - Red LED
   - Working serial command processor
     -   type "?" to see commands
   - TODO: add packet stats 
   - TODO: Channel Filter
   - TODO: Raw Packet output for logging

---
**REV: 042526 — VERS: 1.0**
- First Working version!
- Detects and displays packets
- Uses full hop table / logic
- Adds Channel (hop), Rssi and other info

---
## Features

### Accurate Davis ISS Packet Reception
- Correct sync word and modulation
- Correct hop table and hop timing
- Correct CRC16‑CCITT validation (6‑byte payload CRC)

### Clean receiver architecture
- Mashup of of the following repos. 
  - b-wave: (VP2_TFT) https://github.com/b-wave/VP2_TFT-Weather-Receiver
  - dekay: (DavisRFM69) https://github.com/dekay/DavisRFM69  (not maintained?) 
  - kobuki:(VPTools) https://github.com/kobuki/VPTools 
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
- RFM69W or RFM69HW module (For US use 915MHz)
  - RFM69HCW Tranciever Radio (915/868) http://adafru.it/3070
  - 915 Spring Antenna  http://adafru.it/4269
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



