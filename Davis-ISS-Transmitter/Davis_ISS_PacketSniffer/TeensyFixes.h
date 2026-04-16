//TeensyFixes.h
#pragma once
//
// -----------------------------------------------------------------------------
// Teensy-specific timing and SPI adjustments
// This file isolates all architecture quirks so the Davis logic stays clean.
// -----------------------------------------------------------------------------

#ifdef __IMXRT1062__   // Teensy 4.x
  #define TEENSY_SPI_DELAY() delayMicroseconds(1)
#else
  #define TEENSY_SPI_DELAY()
#endif
