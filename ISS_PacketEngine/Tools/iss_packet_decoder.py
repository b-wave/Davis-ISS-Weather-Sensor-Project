#!/usr/bin/env python3
"""
iss_packet_decoder.py — Davis ISS Packet Decoder & Validator
=============================================================
REV: 043026
VERS: 2.0

Decodes Davis Vantage Pro2 / Vue ISS 8-byte wireless packets.
Supports multiple input formats including the PacketSniffer V1.2
RAW CSV output for automated validation.

Usage:
    python iss_packet_decoder.py --selftest
    python iss_packet_decoder.py --packet "80 08 A1 2F C0 00 76 6C"
    python iss_packet_decoder.py --file capture.csv
    python iss_packet_decoder.py --file capture.csv --csv decoded.csv
    python iss_packet_decoder.py --serial COM7 --baud 115200
    python iss_packet_decoder.py --compare engine.csv --truth sniffer.csv

Dependencies:
    pip install pyserial    (only needed for --serial mode)
"""

import sys
import re
import csv
import argparse
from datetime import timedelta


# ============================================================================
# CRC-CCITT (0x1021, init=0x0000) — matches Davis ISS hardware
# ============================================================================

# Davis CRC table — extracted directly from ISS_packetEngine.ino PROGMEM.
# This table has 33 entries that differ from mathematically-generated
# CRC-CCITT (0x1021). The differences are in the Davis firmware's original
# lookup table and are required for correct CRC validation of real packets.
# Do NOT replace with a generated table — it will fail on real Davis packets.
CRC_TABLE = [
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x4864, 0x5845, 0x6826, 0x7807, 0x08E0, 0x18C1, 0x28A2, 0x38A3,
    0xC94C, 0xD96D, 0xE90E, 0xF92F, 0x89C8, 0x99E9, 0xA98A, 0xB9AB,
    0x5A75, 0x4A54, 0x7A37, 0x6A16, 0x1AF1, 0x0AD0, 0x3AB3, 0x2A92,
    0xDB7D, 0xCB5C, 0xFB3F, 0xEB1E, 0x9BF9, 0x8BD8, 0xABBB, 0xBB9A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x85A9, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
]

def crc_ccitt(data):
    """Compute CRC-CCITT over a bytes-like object."""
    crc = 0x0000
    for b in data:
        idx = ((crc >> 8) ^ b) & 0xFF
        crc = ((crc << 8) ^ CRC_TABLE[idx]) & 0xFFFF
    return crc

def validate_crc(pkt):
    """Validate an 8-byte Davis packet. Returns True if CRC matches and is non-zero."""
    if len(pkt) < 8:
        return False
    calc = crc_ccitt(pkt[:6])
    recv = (pkt[6] << 8) | pkt[7]
    return calc == recv and calc != 0


# ============================================================================
# Packet type names
# ============================================================================

PACKET_TYPES = {
    0x2: "Supercap/BattV",
    0x3: "Lightning",
    0x4: "UV Index",
    0x5: "Rain Rate",
    0x6: "Solar Radiation",
    0x7: "Solar PV Voltage",
    0x8: "Temperature",
    0x9: "Wind Gust",
    0xA: "Humidity",
    0xE: "Rain",
}


# ============================================================================
# Packet decoder
# ============================================================================

def decode_packet(pkt):
    """
    Decode an 8-byte Davis ISS packet into a dictionary of fields.

    Decoding formulas sourced from:
      - DeKay's protocol reverse engineering
      - VP2_TFTd.ino decodePacket() function (independent cross-reference)
      - VPTools / kobuki wind calibration reference
    """
    if len(pkt) < 8:
        return {"error": "Packet too short"}

    result = {}

    # --- Header ---
    result["type_nibble"] = (pkt[0] >> 4) & 0x0F
    result["type_name"]   = PACKET_TYPES.get(result["type_nibble"], f"Unknown(0x{result['type_nibble']:X})")
    result["batt_low"]    = bool(pkt[0] & 0x08)
    result["tx_id"]       = pkt[0] & 0x07

    # --- Wind (always present in bytes 1-2) ---
    result["wind_speed_mph"] = pkt[1]
    if pkt[2] == 0:
        result["wind_dir_deg"] = None   # 0x00 = no reading
    else:
        result["wind_dir_deg"] = round(pkt[2] * 360.0 / 255.0, 1)

    # --- CRC ---
    result["crc_valid"] = validate_crc(pkt)
    result["crc_hex"]   = f"{pkt[6]:02X}{pkt[7]:02X}"

    # --- Sensor-specific data (bytes 3-5) ---
    tn = result["type_nibble"]

    if tn == 0x8:   # Temperature
        raw = (pkt[3] << 8) | pkt[4]
        # Signed 16-bit
        if raw >= 0x8000:
            raw -= 0x10000
        result["temp_f"] = raw / 160.0

    elif tn == 0xA:  # Humidity
        raw = ((pkt[4] >> 4) & 0x03) << 8 | pkt[3]
        result["humidity_pct"] = raw / 10.0

    elif tn == 0x9:  # Wind Gust
        result["gust_speed_mph"] = pkt[3]
        if pkt[5] == 0:
            result["gust_dir_deg"] = None
        else:
            result["gust_dir_deg"] = round(pkt[5] * 360.0 / 255.0, 1)

    elif tn == 0xE:  # Rain
        result["rain_bucket_tips"] = pkt[3]
        # Each tip = 0.01 inches

    elif tn == 0x5:  # Rain Rate
        if pkt[3] == 0xFF and pkt[4] == 0x71:
            result["rain_rate"] = "no rain"
        else:
            # Seconds since last tip
            raw = (pkt[3] << 8) | pkt[4]
            result["rain_rate_raw"] = raw
            if raw > 0:
                # Approximate: inches/hour from seconds between tips
                result["rain_rate_in_hr"] = round(3600.0 / raw * 0.01, 4)
            else:
                result["rain_rate_in_hr"] = 0.0

    elif tn == 0x2:  # Supercap / Battery Voltage
        raw = (pkt[3] << 2) | ((pkt[4] & 0xC0) >> 6)
        result["supercap_raw"] = raw
        result["supercap_voltage"] = raw / 100.0

    elif tn == 0x7:  # Solar PV Voltage
        raw = (pkt[3] << 2) | ((pkt[4] & 0xC0) >> 6)
        result["solar_pv_raw"] = raw
        result["solar_pv_voltage"] = raw / 100.0

    elif tn == 0x6:  # Solar Radiation
        if pkt[3] == 0xFF:
            result["solar_radiation"] = "no sensor"
        else:
            raw = ((pkt[3] << 8) | pkt[4]) >> 6
            result["solar_radiation_wm2"] = round(raw * 1.757936, 1)

    elif tn == 0x4:  # UV Index
        if pkt[3] == 0xFF:
            result["uv_index"] = "no sensor"
        else:
            raw = ((pkt[3] << 8) | pkt[4]) >> 6
            result["uv_index"] = round(raw / 50.0, 1)

    elif tn == 0x3:  # Lightning
        result["lightning_dist_km"] = pkt[3] & 0x3F
        result["lightning_energy"]  = pkt[4]
        result["lightning_event"]   = bool(pkt[5] & 0x80)

    # Raw hex for reference
    result["raw_hex"] = " ".join(f"{b:02X}" for b in pkt)

    return result


# ============================================================================
# Packet parser — handles multiple input formats
# ============================================================================

def parse_hex_bytes(text):
    """Extract 8 hex bytes from a string. Returns bytes or None."""
    # Try comma-separated (RAW CSV fields)
    tokens = re.findall(r'[0-9A-Fa-f]{2}', text)
    if len(tokens) >= 8:
        return bytes(int(t, 16) for t in tokens[:8])
    return None


def parse_csv_line(line):
    """
    Parse a PacketSniffer V1.2 RAW CSV line.
    Format: millis,hop,rssi,crc,b0,b1,b2,b3,b4,b5,b6,b7,type

    Returns dict with metadata + packet bytes, or None if not a data line.
    """
    line = line.strip()
    if not line or line.startswith('#') or line.startswith('millis'):
        return None   # comment, header, or empty

    parts = line.split(',')
    if len(parts) < 13:
        return None

    try:
        meta = {
            "millis":  int(parts[0]),
            "hop":     int(parts[1]),
            "rssi":    int(parts[2]),
            "crc_ok":  parts[3] == '1',
        }
        pkt_bytes = bytes(int(parts[i], 16) for i in range(4, 12))
        meta["packet"] = pkt_bytes
        return meta
    except (ValueError, IndexError):
        return None


def parse_verbose_line(line):
    """
    Parse a PacketSniffer V1.x verbose line.
    Formats handled:
      Channel=15 RSSI=-72 CRC OK  80 08 A1 2F C0 00 76 6C
      PACKET hop=15 RSSI=-72 CRC OK  80 08 A1 2F C0 00 76 6C
      00:05:30 Channel=15 RSSI=-72 CRC OK 80 08 A1 ...
    """
    line = line.strip()
    if not line:
        return None

    # Check for CRC indicator
    crc_ok = None
    if "CRC OK" in line:
        crc_ok = True
    elif "CRC FAIL" in line:
        crc_ok = False
    else:
        return None   # not a packet line

    # Extract RSSI
    rssi_match = re.search(r'RSSI[=:]?\s*(-?\d+)', line)
    rssi = int(rssi_match.group(1)) if rssi_match else None

    # Extract hop/channel
    hop_match = re.search(r'(?:hop|Channel)[=:]?\s*(\d+)', line)
    hop = int(hop_match.group(1)) if hop_match else None

    # Extract packet bytes — find 8 consecutive hex byte tokens
    # Look after the CRC status marker
    crc_marker = "CRC OK" if crc_ok else "CRC FAIL"
    crc_pos = line.find(crc_marker)
    after_crc = line[crc_pos + len(crc_marker):]
    pkt = parse_hex_bytes(after_crc)

    if pkt is None:
        return None

    return {
        "hop": hop,
        "rssi": rssi,
        "crc_ok": crc_ok,
        "packet": pkt,
    }


def parse_line(line):
    """Try to parse a line as CSV first, then verbose. Returns dict or None."""
    result = parse_csv_line(line)
    if result:
        result["format"] = "csv"
        return result

    result = parse_verbose_line(line)
    if result:
        result["format"] = "verbose"
        return result

    # Last resort: just try to find 8 hex bytes anywhere
    pkt = parse_hex_bytes(line)
    if pkt:
        return {
            "packet": pkt,
            "format": "raw",
            "hop": None,
            "rssi": None,
            "crc_ok": None,
        }

    return None


# ============================================================================
# Self-test — reference vectors from ISS_packetEngine stubs + DeKay
# ============================================================================

def run_selftest():
    """
    Validate decoder against known-good packets.
    Reference: DeKay's protocol.txt, VP2_TFTd.ino decodePacket(),
    and ISS_packetEngine verified stub output (4/29/2026).
    """
    print("\n  SELF-TEST: Validating decoder against reference vectors\n")

    passed = 0
    failed = 0

    def check(name, actual, expected, tolerance=None):
        nonlocal passed, failed
        if tolerance is not None:
            ok = abs(actual - expected) <= tolerance
        else:
            ok = actual == expected
        status = "PASS" if ok else "FAIL"
        if not ok:
            failed += 1
            print(f"    {status}: {name} = {actual} (expected {expected})")
        else:
            passed += 1
            print(f"    {status}: {name} = {actual}")

    # --- Test 1: Temperature packet from ISS_packetEngine stubs ---
    print("  Test 1: Packet Engine stub — Temperature 76.4F, Wind 8mph@228deg")
    pkt = bytes([0x80, 0x08, 0xA1, 0x2F, 0xC0, 0x00, 0x76, 0x6C])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0x8)
    check("type_name", d["type_name"], "Temperature")
    check("wind_speed_mph", d["wind_speed_mph"], 8)
    check("wind_dir_deg", d["wind_dir_deg"], 227.3, tolerance=1.0)
    check("temp_f", d["temp_f"], 76.4, tolerance=0.5)
    check("tx_id", d["tx_id"], 0)
    check("batt_low", d["batt_low"], False)
    print()

    # --- Test 2: Humidity packet from ISS_packetEngine stubs ---
    print("  Test 2: Packet Engine stub — Humidity 45%, Wind 8mph@228deg")
    pkt = bytes([0xA0, 0x08, 0xA1, 0xC2, 0x10, 0x00, 0xA5, 0xAB])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0xA)
    check("humidity_pct", d["humidity_pct"], 45.0, tolerance=0.5)
    print()

    # --- Test 3: Wind Gust packet from ISS_packetEngine stubs ---
    print("  Test 3: Packet Engine stub — Gust 12mph, dir 228deg")
    pkt = bytes([0x90, 0x08, 0xA1, 0x0C, 0x00, 0xA1, 0x11, 0xE9])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0x9)
    check("gust_speed_mph", d["gust_speed_mph"], 12)
    check("gust_dir_deg", d["gust_dir_deg"], 227.3, tolerance=1.0)
    print()

    # --- Test 4: Rain Rate (no rain) from ISS_packetEngine stubs ---
    print("  Test 4: Packet Engine stub — Rain Rate (no rain)")
    pkt = bytes([0x50, 0x08, 0xA1, 0xFF, 0x71, 0x00, 0xFC, 0x92])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0x5)
    check("rain_rate", d.get("rain_rate"), "no rain")
    print()

    # --- Test 5: Rain Bucket from ISS_packetEngine stubs ---
    print("  Test 5: Packet Engine stub — Rain bucket (0 tips)")
    pkt = bytes([0xE0, 0x08, 0xA1, 0x00, 0x01, 0x00, 0x26, 0xE6])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0xE)
    check("rain_bucket_tips", d.get("rain_bucket_tips"), 0)
    print()

    # --- Test 6: Solar PV Voltage from ISS_packetEngine stubs ---
    print("  Test 6: Packet Engine stub — Solar PV 5.10V")
    pkt = bytes([0x70, 0x08, 0xA1, 0x7F, 0x85, 0x00, 0xCB, 0x27])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0x7)
    check("solar_pv_voltage", d.get("solar_pv_voltage", 0), 5.1, tolerance=0.05)
    print()

    # --- Test 7: Lightning (quiet) from ISS_packetEngine stubs ---
    print("  Test 7: Packet Engine stub — Lightning (no event)")
    pkt = bytes([0x30, 0x08, 0xA1, 0x00, 0x05, 0x00, 0x4E, 0x16])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], True)
    check("type_nibble", d["type_nibble"], 0x3)
    check("lightning_event", d.get("lightning_event"), False)
    print()

    # --- Test 8: CRC validation — corrupt packet ---
    print("  Test 8: Corrupted packet — CRC should fail")
    pkt = bytes([0x80, 0x08, 0xA1, 0x2F, 0xC0, 0xFF, 0x76, 0x6C])
    d = decode_packet(pkt)
    check("crc_valid", d["crc_valid"], False)
    print()

    # --- Summary ---
    total = passed + failed
    print(f"  SELF-TEST RESULTS: {passed}/{total} passed", end="")
    if failed == 0:
        print(" -- ALL PASS")
    else:
        print(f" -- {failed} FAILED")
    print()

    return failed == 0


# ============================================================================
# File mode — decode all packets from a capture file
# ============================================================================

def decode_file(filepath, csv_out=None, verbose=False):
    """Read a capture file, parse and decode all packets."""
    packets = []
    crc_pass = 0
    crc_fail = 0

    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, 1):
            parsed = parse_line(line)
            if parsed is None:
                continue

            pkt = parsed["packet"]
            decoded = decode_packet(pkt)

            # Merge metadata
            decoded["line_num"]  = line_num
            decoded["hop"]      = parsed.get("hop")
            decoded["rssi"]     = parsed.get("rssi")
            decoded["format"]   = parsed.get("format")
            decoded["millis"]   = parsed.get("millis")

            if decoded["crc_valid"]:
                crc_pass += 1
            else:
                crc_fail += 1

            packets.append(decoded)

            if verbose:
                print(f"  [{line_num:5d}] {decoded['type_name']:18s} "
                      f"CRC={'OK' if decoded['crc_valid'] else 'FAIL':4s} "
                      f"{decoded['raw_hex']}")
                # Print decoded value
                for key in ("temp_f", "humidity_pct", "wind_speed_mph",
                            "wind_dir_deg", "gust_speed_mph", "rain_rate",
                            "rain_bucket_tips", "supercap_voltage",
                            "solar_pv_voltage", "solar_radiation_wm2",
                            "uv_index", "lightning_event"):
                    if key in decoded:
                        print(f"           {key} = {decoded[key]}")

    # Summary
    total = crc_pass + crc_fail
    print(f"\n  File: {filepath}")
    print(f"  Total packets: {total}")
    if total > 0:
        print(f"  CRC Pass:      {crc_pass} ({crc_pass/total*100:.1f}%)")
        print(f"  CRC Fail:      {crc_fail} ({crc_fail/total*100:.1f}%)")

    # Type distribution
    type_counts = {}
    for p in packets:
        tn = p.get("type_name", "Unknown")
        type_counts[tn] = type_counts.get(tn, 0) + 1
    if type_counts:
        print("\n  Packet type distribution:")
        for tn, count in sorted(type_counts.items(), key=lambda x: -x[1]):
            print(f"    {tn:20s}: {count:5d}")

    # CSV export
    if csv_out and packets:
        fieldnames = ["line_num", "millis", "hop", "rssi", "crc_valid",
                       "type_nibble", "type_name", "tx_id", "batt_low",
                       "wind_speed_mph", "wind_dir_deg", "raw_hex"]
        # Add all decoded fields
        all_keys = set()
        for p in packets:
            all_keys.update(p.keys())
        extra = sorted(all_keys - set(fieldnames) - {"error", "format", "crc_hex"})
        fieldnames.extend(extra)

        with open(csv_out, 'w', newline='') as cf:
            writer = csv.DictWriter(cf, fieldnames=fieldnames, extrasaction='ignore')
            writer.writeheader()
            for p in packets:
                writer.writerow(p)
        print(f"\n  Decoded CSV written to: {csv_out}")

    return packets


# ============================================================================
# Single packet mode
# ============================================================================

def decode_single(hex_str):
    """Decode a single packet from hex string."""
    pkt = parse_hex_bytes(hex_str)
    if pkt is None:
        print(f"  ERROR: Could not parse hex bytes from: {hex_str}")
        return

    d = decode_packet(pkt)

    print(f"\n  Packet: {d['raw_hex']}")
    print(f"  CRC:    {'VALID' if d['crc_valid'] else 'INVALID'} ({d['crc_hex']})")
    print(f"  Type:   0x{d['type_nibble']:X} = {d['type_name']}")
    print(f"  TX ID:  {d['tx_id']}")
    print(f"  Batt:   {'LOW' if d['batt_low'] else 'OK'}")
    print(f"  Wind:   {d['wind_speed_mph']} mph @ "
          f"{d['wind_dir_deg'] if d['wind_dir_deg'] else 'N/A'} deg")

    # Print type-specific fields
    for key in ("temp_f", "humidity_pct", "gust_speed_mph", "gust_dir_deg",
                "rain_rate", "rain_rate_in_hr", "rain_rate_raw",
                "rain_bucket_tips", "supercap_voltage", "solar_pv_voltage",
                "solar_radiation_wm2", "solar_radiation",
                "uv_index", "lightning_dist_km", "lightning_energy",
                "lightning_event"):
        if key in d:
            print(f"  {key}: {d[key]}")
    print()


# ============================================================================
# Serial mode — live decode from PacketSniffer
# ============================================================================

def decode_serial(port, baud=115200):
    """Read packets from a serial port and decode live."""
    try:
        import serial
    except ImportError:
        print("  ERROR: pyserial not installed. Run: pip install pyserial")
        return

    print(f"\n  Listening on {port} @ {baud} baud...")
    print(f"  Press Ctrl+C to stop.\n")

    crc_pass = 0
    crc_fail = 0

    try:
        ser = serial.Serial(port, baud, timeout=2.0)
        while True:
            line = ser.readline().decode('ascii', errors='replace').strip()
            if not line:
                continue

            parsed = parse_line(line)
            if parsed is None:
                continue

            d = decode_packet(parsed["packet"])

            if d["crc_valid"]:
                crc_pass += 1
            else:
                crc_fail += 1

            total = crc_pass + crc_fail
            pct = crc_pass / total * 100.0 if total > 0 else 0

            # Compact live display
            value_str = ""
            if "temp_f" in d:
                value_str = f"Temp={d['temp_f']:.1f}F"
            elif "humidity_pct" in d:
                value_str = f"RH={d['humidity_pct']:.1f}%"
            elif "gust_speed_mph" in d:
                value_str = f"Gust={d['gust_speed_mph']}mph"
            elif "rain_rate" in d:
                value_str = f"RainRate={d['rain_rate']}"
            elif "rain_bucket_tips" in d:
                value_str = f"Rain={d['rain_bucket_tips']}tips"
            elif "supercap_voltage" in d:
                value_str = f"BattV={d['supercap_voltage']:.2f}V"
            elif "solar_pv_voltage" in d:
                value_str = f"SolPV={d['solar_pv_voltage']:.2f}V"
            elif "lightning_event" in d:
                value_str = f"Ltng={'EVENT' if d['lightning_event'] else 'quiet'}"

            print(f"  CRC {'OK' if d['crc_valid'] else 'FAIL':4s} [{pct:5.1f}%] "
                  f"{d['type_name']:18s} "
                  f"Wind={d['wind_speed_mph']}mph@{d.get('wind_dir_deg','?')}deg "
                  f"{value_str}")

    except KeyboardInterrupt:
        total = crc_pass + crc_fail
        if total > 0:
            print(f"\n\n  Stopped. {total} packets: "
                  f"{crc_pass} pass, {crc_fail} fail "
                  f"({crc_pass/total*100:.1f}% pass rate)")
        else:
            print("\n\n  Stopped. No packets received.")
    except Exception as e:
        print(f"  Serial error: {e}")


# ============================================================================
# Compare mode — cross-validate two capture files
# ============================================================================

def compare_files(engine_file, truth_file):
    """Compare Packet Engine output against sniffer ground truth."""
    print(f"\n  Cross-Validation Report")
    print(f"  =======================")
    print(f"    Engine: {engine_file}")
    print(f"    Truth:  {truth_file}")

    print(f"\n  --- Engine File ---")
    engine_pkts = decode_file(engine_file)

    print(f"\n  --- Truth File ---")
    truth_pkts  = decode_file(truth_file)

    # Compare type coverage
    engine_types = set(p["type_nibble"] for p in engine_pkts if p["crc_valid"])
    truth_types  = set(p["type_nibble"] for p in truth_pkts if p["crc_valid"])

    print(f"\n  --- Type Coverage Comparison ---")
    all_types = sorted(engine_types | truth_types)
    for t in all_types:
        in_engine = "YES" if t in engine_types else "---"
        in_truth  = "YES" if t in truth_types  else "---"
        name = PACKET_TYPES.get(t, f"0x{t:X}")
        print(f"    0x{t:X} {name:20s}  Engine={in_engine:3s}  Truth={in_truth:3s}")

    missing = truth_types - engine_types
    extra   = engine_types - truth_types
    if missing:
        print(f"\n  WARNING: Engine missing types present in truth: "
              f"{', '.join(f'0x{t:X}' for t in sorted(missing))}")
    if extra:
        print(f"\n  NOTE: Engine has types not in truth: "
              f"{', '.join(f'0x{t:X}' for t in sorted(extra))}")
    if not missing and not extra:
        print(f"\n  Engine and truth have identical type coverage "
              f"({len(truth_types)} types).")

    # CRC comparison
    engine_pass = sum(1 for p in engine_pkts if p["crc_valid"])
    truth_pass  = sum(1 for p in truth_pkts if p["crc_valid"])
    print(f"\n  --- CRC Summary ---")
    print(f"    Engine: {engine_pass}/{len(engine_pkts)} CRC pass "
          f"({engine_pass/len(engine_pkts)*100:.1f}%)" if engine_pkts else "")
    print(f"    Truth:  {truth_pass}/{len(truth_pkts)} CRC pass "
          f"({truth_pass/len(truth_pkts)*100:.1f}%)" if truth_pkts else "")


# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Davis ISS Packet Decoder & Validator v2.0",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --selftest
  %(prog)s --packet "80 08 A1 2F C0 00 76 6C"
  %(prog)s --file capture.csv --verbose
  %(prog)s --file capture.csv --csv decoded.csv
  %(prog)s --serial COM7 --baud 115200
  %(prog)s --compare engine.csv --truth sniffer.csv
        """)

    parser.add_argument("--selftest", action="store_true",
                        help="Run decoder self-test against reference vectors")
    parser.add_argument("--packet", type=str,
                        help="Decode a single packet (hex string)")
    parser.add_argument("--file", type=str,
                        help="Decode all packets from a capture file")
    parser.add_argument("--csv", type=str,
                        help="Export decoded results to CSV (use with --file)")
    parser.add_argument("--verbose", action="store_true",
                        help="Show detailed decode for each packet")
    parser.add_argument("--serial", type=str,
                        help="Live decode from serial port")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Serial baud rate (default: 115200)")
    parser.add_argument("--compare", type=str,
                        help="Engine capture file for cross-validation")
    parser.add_argument("--truth", type=str,
                        help="Ground truth capture file (use with --compare)")

    args = parser.parse_args()

    if args.selftest:
        success = run_selftest()
        sys.exit(0 if success else 1)

    elif args.packet:
        decode_single(args.packet)

    elif args.file:
        decode_file(args.file, csv_out=args.csv, verbose=args.verbose)

    elif args.serial:
        decode_serial(args.serial, args.baud)

    elif args.compare and args.truth:
        compare_files(args.compare, args.truth)

    else:
        parser.print_help()
        print("\n  Tip: Start with --selftest to verify the decoder.\n")


if __name__ == "__main__":
    main()
