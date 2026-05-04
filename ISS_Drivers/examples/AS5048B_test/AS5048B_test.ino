==============================================================================
FILE: tests/AS5048B_test/AS5048B_test.ino
==============================================================================
/**
 * AS5048B_test.ino — Standalone test sketch for AS5048B wind direction sensor
 *
 * ISS Packet Engine — Sensor Driver Tests
 *
 * Wiring (Teensy 4.x):
 *   AS5048B SDA → Pin 18 (SDA)
 *   AS5048B SCL → Pin 19 (SCL)
 *   AS5048B VCC → 3.3V (or 5V — internal LDO)
 *   AS5048B GND → GND
 *   Diametrically magnetized disc magnet on shaft above IC
 *
 * Testing:
 *   - Slowly rotate a magnet by hand above the sensor
 *   - Verify smooth 0-360° output with no dead zones
 *   - Check AGC value stays in 30-200 range (magnet health)
 *   - Verify Davis direction byte maps correctly
 *
 * Output: Serial monitor at 115200 baud
 */

#include "../../SensorDriver.h"
#include "../../AS5048BDriver.h"

// Configuration
#define AS5048B_ADDR      0x40
#define READ_INTERVAL_MS  250     // Fast updates for manual rotation testing

AS5048BDriver vane(AS5048B_ADDR, &Wire);

uint32_t lastRead = 0;
uint32_t readCount = 0;
float    lastAngle = -1.0f;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  AS5048B Wind Direction Sensor Test");
    Serial.println("========================================");
    Serial.println();

    Wire.begin();

    // --- Initialization ---
    Serial.print("Initializing AS5048B at 0x");
    Serial.print(AS5048B_ADDR, HEX);
    Serial.print("... ");

    if (vane.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED!");
        Serial.println("  Check wiring and I2C address.");
        Serial.println("  Default: 0x40 (A1/A2 pads open)");
        Serial.println("  Scanning I2C bus...");

        // I2C scanner
        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.print("  Found device at 0x");
                Serial.println(addr, HEX);
            }
        }
        while (1) delay(1000);
    }

    // --- Self-test ---
    Serial.print("Self-test... ");
    if (vane.selfTest()) {
        Serial.println("PASS");
    } else {
        Serial.println("FAIL");
        Serial.print("  AGC: ");
        Serial.println(vane.getAGC());
        Serial.print("  Magnet OK: ");
        Serial.println(vane.isMagnetOK() ? "YES" : "NO");
    }

    // --- Initial read ---
    vane.read();
    Serial.print("Initial angle: ");
    Serial.print(vane.getAngleDeg(), 1);
    Serial.println(" deg");
    Serial.print("AGC: ");
    Serial.print(vane.getAGC());
    Serial.print(" (healthy range: 30-200, yours: ");
    Serial.print(vane.isMagnetOK() ? "OK" : "OUT OF RANGE");
    Serial.println(")");

    Serial.println();
    Serial.println("Rotate the magnet slowly. Watch for smooth 0-360 output.");
    Serial.println("Davis byte 0x00 = no reading. Valid range: 0x01-0xFF.");
    Serial.println();
    Serial.println("  #     Angle    Raw14   DavisByte  AGC  Magnet  Delta");
    Serial.println("  ----  -------  ------  ---------  ---  ------  -----");
}

void loop() {
    if (millis() - lastRead < READ_INTERVAL_MS) return;
    lastRead = millis();
    readCount++;

    if (!vane.read()) {
        Serial.print("  ");
        Serial.print(readCount);
        Serial.println("  READ ERROR");
        return;
    }

    float angle = vane.getAngleDeg();
    float delta = 0.0f;
    if (lastAngle >= 0.0f) {
        delta = angle - lastAngle;
        // Handle wrap-around
        if (delta > 180.0f) delta -= 360.0f;
        if (delta < -180.0f) delta += 360.0f;
    }
    lastAngle = angle;

    char buf[100];
    snprintf(buf, sizeof(buf),
             "  %4lu  %6.1f   %5u     0x%02X     %3u    %s   %+.1f",
             readCount,
             angle,
             vane.getAngleRaw14(),
             vane.getDavisDirectionByte(),
             vane.getAGC(),
             vane.isMagnetOK() ? " OK " : "WARN",
             delta);
    Serial.println(buf);

    // Warn about sudden jumps (potential dead zone or loose magnet)
    if (abs(delta) > 30.0f && readCount > 2) {
        Serial.println("  *** LARGE JUMP DETECTED — check magnet alignment ***");
    }

    // Compass-style direction label every 20 readings
    if (readCount % 20 == 0) {
        Serial.print("  [Direction: ");
        if      (angle < 22.5f  || angle >= 337.5f) Serial.print("N");
        else if (angle < 67.5f)  Serial.print("NE");
        else if (angle < 112.5f) Serial.print("E");
        else if (angle < 157.5f) Serial.print("SE");
        else if (angle < 202.5f) Serial.print("S");
        else if (angle < 247.5f) Serial.print("SW");
        else if (angle < 292.5f) Serial.print("W");
        else                     Serial.print("NW");
        Serial.println("]");
    }
}
