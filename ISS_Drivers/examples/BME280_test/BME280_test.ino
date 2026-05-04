//==============================================================================
//FILE: tests/BME280_test/BME280_test.ino
//==============================================================================
/**
 * BME280_test.ino — Standalone test sketch for BME280 sensor driver
 *
 * ISS Packet Engine — Sensor Driver Tests
 *
 * Wiring (Teensy 4.x):
 *   BME280 SDA → Pin 18 (SDA)
 *   BME280 SCL → Pin 19 (SCL)
 *   BME280 VCC → 3.3V
 *   BME280 GND → GND
 *   BME280 SDO → GND (address 0x76) or VCC (address 0x77)
 *
 * Output: Serial monitor at 115200 baud
 */

#include <SensorDriver.h>
#include <BME280Driver.h>
 
//#include "../../SensorDriver.h"
//#include "../../BME280Driver.h"

// Configuration
#define BME280_ADDR       0x76
#define READ_INTERVAL_MS  2000
#define TEMP_OFFSET_F     0.0f   // Adjust after comparing to reference thermometer

BME280Driver bme(BME280_ADDR, &Wire, TEMP_OFFSET_F);

uint32_t lastRead = 0;
uint32_t readCount = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000);  // Wait for USB serial (max 5s)

    Serial.println();
    Serial.println("========================================");
    Serial.println("  BME280 Sensor Driver Test");
    Serial.println("========================================");
    Serial.println();

    Wire.begin();

    // --- Initialization ---
    Serial.print("Initializing BME280 at 0x");
    Serial.print(BME280_ADDR, HEX);
    Serial.print("... ");

    if (bme.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED!");
        Serial.print("  Status: ");
        Serial.println(bme.getStatus());
        Serial.println("  Check wiring and I2C address.");
        Serial.println("  SDO pin → GND = 0x76, SDO → VCC = 0x77");
        while (1) delay(1000);  // Halt
    }

    // --- Self-test ---
    Serial.print("Self-test... ");
    if (bme.selfTest()) {
        Serial.println("PASS");
    } else {
        Serial.println("FAIL");
        Serial.print("  Status: ");
        Serial.println(bme.getStatus());
    }

    Serial.println();
    Serial.println("Reading every 2 seconds. Compare with a reference thermometer.");
    Serial.println("Temp offset can be set in TEMP_OFFSET_F define.");
    Serial.println();
    Serial.println("  #   Temp(F)  Temp(C)  Humid(%)  Press(hPa)  DavisTempRaw  DavisHumRaw  Status");
    Serial.println("  --- -------  -------  --------  ----------  ------------  -----------  ------");
}

void loop() {
    if (millis() - lastRead < READ_INTERVAL_MS) return;
    lastRead = millis();
    readCount++;

    if (bme.read()) {
        char buf[120];
        snprintf(buf, sizeof(buf),
                 "  %3lu  %6.1f   %6.1f    %5.1f     %7.1f       %6d        %5u      OK",
                 readCount,
                 bme.getTemperatureF(),
                 bme.getTemperatureC(),
                 bme.getHumidityPct(),
                 bme.getPressureHPa(),
                 bme.getDavisTemperatureRaw(),
                 bme.getDavisHumidityRaw());
        Serial.println(buf);
    } else {
        Serial.print("  ");
        Serial.print(readCount);
        Serial.print("  READ ERROR — status: ");
        Serial.println(bme.getStatus());
    }

    // Periodic self-test every 30 readings
    if (readCount % 30 == 0) {
        Serial.print("  [Self-test check: ");
        Serial.print(bme.selfTest() ? "PASS" : "FAIL");
        Serial.println("]");
    }
}

