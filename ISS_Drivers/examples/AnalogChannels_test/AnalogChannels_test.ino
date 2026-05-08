//==============================================================================
//FILE: tests/AnalogChannels_test/AnalogChannels_test.ino
//==============================================================================
/**
 * AnalogChannels_test.ino — Test all analog input channels
 *
 * ISS Packet Engine — Sensor Driver Tests
 *
 * Wiring (Teensy 3.x/4.x):
 *   A1 (pin 15) — Solar cell voltage via resistor divider (100K/100K)
 *   A2 (pin 16) — Battery voltage via resistor divider (100K/200K)
 *   A8 (pin 22) — Battery thermistor (10K NTC + 10K fixed)
 *   A9 (pin 23) — LDR luminosity (LDR + 10K fixed)
 *
 * Bench testing:
 *   Apply known voltages with a bench supply or precision divider
 *   and verify the displayed readings match your multimeter.
 *
 * Output: Serial monitor at 115200 baud
 */ 
 
#include <SensorDriver.h>
#include <AnalogDriver.h>
//#include "../../SensorDriver.h"
//#include "../../AnalogDriver.h"

// Channel configuration
//                              pin   divider  mode              oversample
AnalogDriver solarV(            15,   2.0f,    ANALOG_VOLTAGE,   8);
AnalogDriver battV(             16,   1.5f,    ANALOG_VOLTAGE,   8);
AnalogDriver battTherm(         22,   1.0f,    ANALOG_NTC_THERM, 16);
AnalogDriver ldrLight(          23,   1.0f,    ANALOG_RELATIVE,  8);

#define READ_INTERVAL_MS  2000
uint32_t lastRead = 0;
uint32_t readCount = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  Analog Channels Test");
    Serial.println("========================================");
    Serial.println();

    // Name each channel for clear output
    solarV.setName("Solar_V");
    battV.setName("Battery_V");
    battTherm.setName("BattTherm");
    ldrLight.setName("LDR_Light");

    // Configure battery low threshold
    battV.setBatteryLowThreshold(3.3f);  // Li-Ion low threshold

    // Configure thermistor (10K NTC, B=3950, 10K fixed resistor)
    battTherm.setNTCParams(10000.0f, 10000.0f, 298.15f, 3950.0f);

    // Configure LDR thresholds for day/night detection
    ldrLight.setThreshold(10.0f, 60.0f);  // Below 10% = night, above 60% = bright

    // Initialize all channels
    const char* names[] = {"Solar V", "Battery V", "Batt Therm", "LDR Light"};
    SensorDriver* sensors[] = {&solarV, &battV, &battTherm, &ldrLight};

    for (int i = 0; i < 4; i++) {
        Serial.print("  Init ");
        Serial.print(names[i]);
        Serial.print("... ");
        Serial.println(sensors[i]->begin() ? "OK" : "FAIL");
    }

    Serial.println();
    Serial.println("  #   Solar(V)  Batt(V) BattLow  Therm(C)  LDR(%)  DayNight");
    Serial.println("  --- --------  ------- -------  --------  ------  --------");
}

void loop() {
    if (millis() - lastRead < READ_INTERVAL_MS) return;
    lastRead = millis();
    readCount++;

    solarV.read();
    battV.read();
    battTherm.read();
    ldrLight.read();

    // Determine day/night
    const char* dayNight = "---";
    if (ldrLight.isBelowLow())       dayNight = "DARK";
    else if (ldrLight.isAboveHigh())  dayNight = "BRIGHT";
    else                              dayNight = "DIM";

    char buf[120];
    snprintf(buf, sizeof(buf),
             "  %3lu   %5.2f     %5.2f   %s    %6.1f    %5.1f   %s",
             readCount,
             solarV.readVoltage(),
             battV.readVoltage(),
             battV.isDavisBatteryLow() ? " YES " : " no  ",
             battTherm.readScaled(),
             ldrLight.readScaled(),
             dayNight);
    Serial.println(buf);

    // Detailed raw values every 10 readings
    if (readCount % 10 == 0) {
        Serial.println("  --- Raw ADC values ---");
        Serial.print("    Solar:  raw="); Serial.print(solarV.readRaw());
        Serial.print("  pin_V="); Serial.println(solarV.readRaw() / 1023.0f * 3.3f, 3);
        Serial.print("    Batt:   raw="); Serial.print(battV.readRaw());
        Serial.print("  pin_V="); Serial.println(battV.readRaw() / 1023.0f * 3.3f, 3);
        Serial.print("    Therm:  raw="); Serial.print(battTherm.readRaw());
        Serial.print("  pin_V="); Serial.println(battTherm.readRaw() / 1023.0f * 3.3f, 3);
        Serial.print("    LDR:    raw="); Serial.print(ldrLight.readRaw());
        Serial.print("  pin_V="); Serial.println(ldrLight.readRaw() / 1023.0f * 3.3f, 3);
        Serial.println("  ----------------------");
        Serial.println("  #   Solar(V)  Batt(V) BattLow  Therm(C)  LDR(%)  DayNight");
        Serial.println("  --- --------  ------- -------  --------  ------  --------");
    }
}