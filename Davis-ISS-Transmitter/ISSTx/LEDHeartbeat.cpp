#include "LEDHeartbeat.h"
#include <Arduino.h>

#define LED_PIN 13

void LEDHeartbeat::init() {
    pinMode(LED_PIN, OUTPUT);
}

void LEDHeartbeat::flash() {
    digitalWrite(LED_PIN, HIGH);
    delay(10);
    digitalWrite(LED_PIN, LOW);
}
