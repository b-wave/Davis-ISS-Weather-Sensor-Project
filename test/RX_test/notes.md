# Custom Davis-Integrated Weather Station

## Reciever Test Software
- Uses VPTest as basis
- Demonstrates changes to make VPTools run on a Teensey 3.2
- Flash can be used Logging (not used)
- transmitter temp status can be monitored (not used)

## Library Versions (Tested and Verified)

- RFM69: LowPowerLab RFM69 v1.3.0
- SPIFlash: SPIFlash v2.0.0 (not used, but referenced)
- Arduino core for Teensy: Teensyduino v1.59
- RFM69registers.h (‎2/‎7/‎2025 ‏‎7:45 AM)
- DavisRFM69.h (‎2/‎7/‎2025 ‏‎7:45 AM)
- ‎PacketFifo.h (2/‎7/‎2025 ‏‎7:45 AM)

## Reciever Test outputs
NOTE: Only the packets starting with E2, 52, 82,  92... 
are valid and being tranmitted from a Davis ISS on channel #2

8:59:09.320 -> Teensy RFM69 RX Test!
18:59:09.320 -> 
18:59:09.320 -> No SPI Flash Found.
18:59:09.320 -> Waiting for signal. This can take some time...
18:59:37.788 -> 0 - Data: 24 58 0C 10 92 41 00 F2 C5 06   RSSI: -94
19:03:21.604 -> 1 - Data: E2 03 D4 8F 01 09 39 2B FF FF   RSSI: -76
19:03:24.278 -> 2 - Data: 52 03 CF FF 73 00 E0 25 FF FF   RSSI: -82
19:03:26.980 -> 3 - Data: 82 02 D0 31 29 0E 1C C5 FF FF   RSSI: -81
19:03:29.660 -> 4 - Data: 92 03 D0 07 11 5C 2D 9E FF FF   RSSI: -84
19:03:32.371 -> 5 - Data: E2 02 C9 8F 01 0F D1 9D FF FF   RSSI: -76
19:03:35.035 -> 6 - Data: 52 02 C9 FF 71 07 7B 68 FF FF   RSSI: -79
19:03:37.726 -> 7 - Data: 82 02 C9 31 2B 00 73 B9 FF FF   RSSI: -90
19:03:40.384 -> 8 - Data: A2 01 CF 10 19 0E BC 35 FF FF   RSSI: -81
