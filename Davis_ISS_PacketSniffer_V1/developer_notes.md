# Developer Notes  
**Davis ISS Packet Sniffer — Internal Engineering Notes**  
**REV: 042526 — VERS: 1.0**

These notes capture subtle engineering traps encountered during development.  
They exist to save future developers (and Future‑Steve) from rediscovering them.

---

## 1. Davis ISS CRC Gotcha (Critical)

The ISS packet is 8 bytes total, but CRC is computed over **only the first 6 bytes**.

Correct:

```cpp
uint16_t calc = radio.crc16_ccitt(packet, 6);
uint16_t recv = (packet[6] << 8) | packet[7];
bool davisCrcOk = (calc == recv) && (calc != 0)
