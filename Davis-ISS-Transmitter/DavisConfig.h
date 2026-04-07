#pragma once
#include <stdint.h>

// -----------------------------------------------------------------------------
// Davis ISS RF Configuration Constants
// This file isolates all Davis-specific RF parameters so the protocol layer
// stays clean and easy to modify or port.
// -----------------------------------------------------------------------------
#define DAVIS_TX_ID  5   // choose any 0–7 that your console is not using

// Bitrate: 19200 bps (Davis ISS standard)
#define DAVIS_BITRATE_MSB   0x1A
#define DAVIS_BITRATE_LSB   0x0B

// Frequency deviation: ~25 kHz
#define DAVIS_FDEV_MSB      0x00
#define DAVIS_FDEV_LSB      0x52

// Sync word (Davis uses 0xAA 0x2D)
#define DAVIS_SYNC1         0xAA
#define DAVIS_SYNC2         0x2D

// Preamble length (bytes)
#define DAVIS_PREAMBLE_LEN  8

// Transmit interval (milliseconds)
#define DAVIS_TX_INTERVAL_MS 2563


// Davis ISS hop table (51 channels)
extern const uint32_t DAVIS_FREQ_TABLE[51];

