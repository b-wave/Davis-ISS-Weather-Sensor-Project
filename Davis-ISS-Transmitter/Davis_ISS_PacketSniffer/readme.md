# Davis ISS Packet Sniffer
Modular RX diagnostic tool for Davis weather station telemetry.  
Focus: deterministic SPI timing, safe RF bring‑up, and teachable workflow.

**Not Functional 4/15/2026**

## Bring‑Up Phases

### Phase 1 — SPI Timing Fix (`v4.14.2026_SPI_FIX`)
**Goal:** Restore 8 MHz clock and MODE0 transactions.  
**Key changes:**
- Added `SPI.beginTransaction()` / `SPI.endTransaction()` in `readReg()` and `writeReg()`.
- Initialized SPI in RX constructor.
**Expected scope results:**
- Clean 8 MHz CLK square wave.
- CS low for ~16 µs per register access.
- LED dimmer (shorter CS low time).
**Bench notes:**  
*(Add scope screenshot here — `/docs/lab_notes/spi_timing.png`)*

---

### Phase 2 — Reset + OPMODE Cleanup (`v4.15.2026_RX_TIMING_FIX`)
**Goal:** Ensure stable RX entry and clean startup.  
**Key changes:**
- Added hardware reset pulse.
- Changed OPMODE mask from `0xE3` → `0xE0`.
- Configured IRQ pin and disabled ListenMode.
**Expected behavior:**
- Version register reads correctly (0x24 / 0x25 typical).
- RSSI responds to ISS transmissions.
**Bench notes:**  
*(Add screenshot — `/docs/lab_notes/reset_opmode.png`)*

---

### Phase 3 — RSSI/FIFO Timing Alignment (`v4.16.2026_CONFIG_PARITY`)
**Goal:** Match VPTools timing for RSSI and FIFO reads.  
**Key changes:**
- Added delay and timeout in `readRSSI()`.
- Set FIFO threshold and payload length.
**Expected behavior:**
- RSSI fluctuates between –70 dBm and –90 dBm.
- IRQ pulse ~1 ms after RSSI measurement.
**Bench notes:**  
*(Add screenshot — `/docs/lab_notes/rssi_fifo.png`)*

---

### Phase 4 — Configuration Parity (`v4.17.2026_VALIDATION`)
**Goal:** Lock modulation, sync word, and deviation to Davis ISS defaults.  
**Key changes:**
- Set FSK packet mode, bitrate = 0x1A0B, deviation = 0x0052.
- Added sync word (2D D4) and disabled ListenMode.
**Expected behavior:**
- Stable 0x40‑byte payloads.
- Consistent IRQ timing (~1.2 ms delay).
**Bench notes:**  
*(Add screenshot — `/docs/lab_notes/config_parity.png`)*

---

### Phase 5 — Validation & Documentation
**Goal:** Confirm reproducibility and finalize lab notes.  
**Actions:**
- Added `dumpRegisters()` diagnostic.
- Captured scope traces for SPI and IRQ.
- Updated README and `/docs/lab_notes/`.
**Expected behavior:**
- Deterministic RX operation identical to VPTools.
- Sniffer ready for TX validation.

