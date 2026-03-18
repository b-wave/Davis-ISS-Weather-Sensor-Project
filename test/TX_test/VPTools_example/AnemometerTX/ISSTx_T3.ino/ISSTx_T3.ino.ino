/*



*/

#include <DavisRFM69_Teensy.h>

DavisRFM69 radio;

void setup() {
  // initialize radio
  radio.initialize();

  // set safe power level for RFM69HW @ 3.3V
  //radio.setPowerLevel(10);   // conservative, safe

  // no high-power mode!
  // radio.setHighPower(false);  // only if your library exposes this
}

void loop() {
  // placeholder for canned packet send
}

