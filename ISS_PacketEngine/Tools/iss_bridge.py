#!/usr/bin/env python3
"""
iss_bridge.py — Dual-Teensy ISS Packet Engine test bridge.

Monitors USB serial output from Teensy A (ISS_packetEngine) and
Teensy B (VP2_TFTd), performs CRC-CCITT validation, and logs results.
"""

import json
import serial
import threading
import time
import csv
import sys
from datetime import datetime

# ---------------------------------------------------------------------------
# CRC-CCITT (0x1021) — Davis standard
# ---------------------------------------------------------------------------
CRC_TABLE = []

def _build_crc_table():
    """Pre-compute CRC-CCITT lookup table."""
    for i in range(256):
        crc = i << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1
            crc &= 0xFFFF
        CRC_TABLE.append(crc)

_build_crc_table()


def crc_ccitt(data: bytes) -> int:
    """Calculate CRC-CCITT over a byte sequence."""
    crc = 0x0000
    for byte in data:
        crc = ((crc << 8) & 0xFFFF) ^ CRC_TABLE[((crc >> 8) ^ byte) & 0xFF]
    return crc


def validate_packet_crc(packet: bytes) -> bool:
    """
    Validate an 8-byte Davis ISS packet.
    Bytes 0-5 are payload, bytes 6-7 are CRC (big-endian).
    A valid packet yields crc_ccitt(all 8 bytes) == 0.
    """
    if len(packet) < 8:
        return False
    return crc_ccitt(packet[:8]) == 0


# ---------------------------------------------------------------------------
# Serial reader thread
# ---------------------------------------------------------------------------
class TeensyReader(threading.Thread):
    """Read serial data from one Teensy and buffer complete packets."""

    def __init__(self, port, baudrate, label, packet_callback):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.label = label
        self.callback = packet_callback
        self.running = True

    def run(self):
        try:
            ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1.0
            )
            print(f"[{self.label}] Opened {self.port} @ {self.baudrate}")
        except serial.SerialException as e:
            print(f"[{self.label}] FAILED to open {self.port}: {e}")
            return

        buf = b""
        while self.running:
            try:
                chunk = ser.read(ser.in_waiting or 1)
                if chunk:
                    buf += chunk
                    # Process complete lines (debug output)
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        self.callback(self.label, "line", line.strip())
                    # Process raw 8-byte packets if accumulated
                    while len(buf) >= 8:
                        packet = buf[:8]
                        buf = buf[8:]
                        self.callback(self.label, "packet", packet)
            except serial.SerialException:
                print(f"[{self.label}] Serial error — reconnecting...")
                time.sleep(2)

    def stop(self):
        self.running = False


# ---------------------------------------------------------------------------
# Main bridge
# ---------------------------------------------------------------------------
def main():
    # Load configuration
    config_path = "bridge_config.json"
    if len(sys.argv) > 1:
        config_path = sys.argv[1]

    try:
        with open(config_path) as f:
            cfg = json.load(f)
    except FileNotFoundError:
        print(f"Config file '{config_path}' not found.")
        print("Using defaults: COM5 (Teensy A), COM6 (Teensy B), 19200 baud")
        cfg = {
            "teensy_a": {"port": "COM5", "label": "ISS_packetEngine"},
            "teensy_b": {"port": "COM6", "label": "VP2_TFTd"},
            "serial": {"baudrate": 19200},
            "logging": {"log_file": "iss_test_log.csv", "log_raw_hex": True}
        }

    baudrate = cfg["serial"]["baudrate"]
    log_file = cfg["logging"].get("log_file", "iss_test_log.csv")
    log_hex = cfg["logging"].get("log_raw_hex", True)

    # Open CSV log
    csv_f = open(log_file, "w", newline="")
    writer = csv.writer(csv_f)
    writer.writerow([
        "timestamp", "source", "type", "raw_hex",
        "crc_valid", "crc_expected", "crc_received", "decoded"
    ])

    stats = {"packets": 0, "crc_pass": 0, "crc_fail": 0}

    def on_data(label, dtype, data):
        ts = datetime.now().isoformat(timespec="milliseconds")

        if dtype == "line":
            text = data.decode("utf-8", errors="replace")
            print(f"  [{label}] {text}")
            writer.writerow([ts, label, "debug", text, "", "", "", ""])

        elif dtype == "packet":
            hex_str = data.hex(" ") if log_hex else ""
            valid = validate_packet_crc(data)
            stats["packets"] += 1

            if valid:
                stats["crc_pass"] += 1
                status = "CRC OK"
            else:
                stats["crc_fail"] += 1
                status = "CRC FAIL"

            # Extract CRC fields
            crc_received = int.from_bytes(data[6:8], "big") if len(data) >= 8 else 0
            crc_computed = crc_ccitt(data[:6])

            print(f"  [{label}] PKT: {hex_str}  {status}  "
                  f"(computed=0x{crc_computed:04X}  received=0x{crc_received:04X})")

            writer.writerow([
                ts, label, "packet", hex_str,
                valid, f"0x{crc_computed:04X}", f"0x{crc_received:04X}", ""
            ])
            csv_f.flush()

    # Start reader threads
    reader_a = TeensyReader(
        cfg["teensy_a"]["port"], baudrate,
        cfg["teensy_a"].get("label", "Teensy_A"), on_data
    )
    reader_b = TeensyReader(
        cfg["teensy_b"]["port"], baudrate,
        cfg["teensy_b"].get("label", "Teensy_B"), on_data
    )

    reader_a.start()
    reader_b.start()

    print("\n" + "=" * 60)
    print("  ISS Packet Engine — Test Bridge Running")
    print("=" * 60)
    print(f"  Teensy A: {cfg['teensy_a']['port']}  ({cfg['teensy_a'].get('label', '')})")
    print(f"  Teensy B: {cfg['teensy_b']['port']}  ({cfg['teensy_b'].get('label', '')})")
    print(f"  Baud:     {baudrate}")
    print(f"  Log:      {log_file}")
    print("=" * 60)
    print("  Press Ctrl+C to stop.\n")

    try:
        while True:
            time.sleep(5)
            total = stats["packets"]
            if total > 0:
                rate = (stats["crc_pass"] / total) * 100
                print(f"  [STATS] {total} packets | "
                      f"{stats['crc_pass']} pass | "
                      f"{stats['crc_fail']} fail | "
                      f"{rate:.1f}% CRC pass rate")
    except KeyboardInterrupt:
        print("\n  Shutting down...")
        reader_a.stop()
        reader_b.stop()
        csv_f.close()
        print("  Log saved to:", log_file)


if __name__ == "__main__":
    main()
