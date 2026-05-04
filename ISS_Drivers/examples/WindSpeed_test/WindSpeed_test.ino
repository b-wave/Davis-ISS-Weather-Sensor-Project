
//==============================================================================
//FILE: tests/WindSpeed_test/WindSpeed_test.ino
//==============================================================================
/**
 * WindSpeed_test.ino — Standalone test sketch for Wind Speed driver
 *
 * ISS Packet Engine — Sensor Driver Tests
 *
 * Wiring (Teensy 4.x):
 *   Hall-effect switch → Pin 2 (interrupt-capable)
 *   Switch other lead  → GND
 *   (Internal pullup enabled — no external resistor needed)
 *
 * Testing without real cups:
 *   - Touch a wire between Pin 2 and GND to simulate pulses
 *   - Or use a second Arduino/Teensy to generate a known-frequency
 *     square wave on a GPIO pin connected to Pin 2
 *
 * Output: Serial monitor at 115200 baud
 */
#include <SensorDriver.h>
#include <DavisCalibration.h>
#include <WindSpeedDriver.h>

//#include "../../SensorDriver.h"
//#include "../../DavisCalibration.h"
//#include "../../WindSpeedDriver.h"

// Configuration
#define WIND_PIN          5       // Interrupt pin
#define ENABLE_EC         true    // Enable Davis error correction
#define SAMPLE_INTERVAL   2500    // ms — matches Davis 2.5s packet interval
#define SIMULATED_DIR     270     // Simulated wind direction for EC testing

WindSpeedDriver wind(WIND_PIN, ENABLE_EC);

uint32_t lastSample = 0;
uint32_t sampleCount = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  Wind Speed Driver Test");
    Serial.println("  Davis OEM Error Correction: ");
    Serial.print("  ");
    Serial.println(ENABLE_EC ? "ENABLED" : "DISABLED");
    Serial.println("========================================");
    Serial.println();

    // --- Initialization ---
    Serial.print("Initializing wind speed on pin ");
    Serial.print(WIND_PIN);
    Serial.print("... ");

    if (wind.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED!");
        while (1) delay(1000);
    }

    // --- Self-test ---
    Serial.print("Self-test (pin pullup check)... ");
    Serial.println(wind.selfTest() ? "PASS" : "FAIL");

    // Set simulated wind direction for EC
    wind.setWindDirection(SIMULATED_DIR);
    Serial.print("Wind direction for EC: ");
    Serial.print(SIMULATED_DIR);
    Serial.println(" deg");

    Serial.println();
    Serial.println("Sampling every 2.5s. Tap a wire to GND on pin 2 to simulate pulses.");
    Serial.println();
    Serial.println("  #   Pulses  Period(us)  RawMPH   CorrMPH  GustMPH  Status");
    Serial.println("  --- ------  ----------  -------  -------  -------  ------");
}

void loop() {
    if (millis() - lastSample < SAMPLE_INTERVAL) return;
    lastSample = millis();
    sampleCount++;

    wind.read();

    char buf[120];
    snprintf(buf, sizeof(buf),
             "  %3lu  %5u   %9lu   %6.1f    %3u      %3u     %s",
             sampleCount,
             wind.getPulseCount(),
             (unsigned long)wind.getLastPeriodUs(),
             wind.getSpeedRawMPH(),
             wind.getSpeedMPH(),
             wind.getGustMPH(),
             wind.getSpeedMPH() > 0 ? "WIND" : "CALM");
    Serial.println(buf);

    // Show EC effect at higher speeds
    if (wind.getSpeedRawMPH() >= 20.0f) {
        float diff = wind.getSpeedRawMPH() - (float)wind.getSpeedMPH();
        Serial.print("  [EC correction: ");
        Serial.print(diff, 1);
        Serial.println(" mph adjustment]");
    }

    // Reset gust every 10 samples (simulating gust packet cycle)
    if (sampleCount % 10 == 0) {
        Serial.print("  [Gust reset — peak was ");
        Serial.print(wind.getGustMPH());
        Serial.println(" mph]");
        wind.resetGust();
    }
}
