#include "SerialShell.h"

SerialShell::SerialShell(DavisRFM69* r, TxPacketEngine* p) {
    radio = r;
    engine = p;
}

void SerialShell::poll() {
    if (!Serial.available()) return;

    char c = Serial.read();
    handleCommand(c);
}

void SerialShell::handleCommand(char cmd) {
    switch (cmd) {

    case '?':
        Serial.println("Commands:");
        Serial.println("  ?   Help");
        Serial.println("  r   Radio register dump");
        Serial.println("  pN  Set TX power (0–31)");
        Serial.println("  iN  Set TX ID (1–8)");
        Serial.println("  s   Sensor dump");
        break;

    case 'r':
        radio->readAllRegs();
        break;

    case 'p': {
        int pwr = Serial.parseInt();
        radio->setPowerLevel(pwr);
        Serial.print("Power set to ");
        Serial.println(pwr);
        break;
    }

    case 'i': {
        int id = Serial.parseInt();
        Serial.print("TX ID set to ");
        Serial.println(id);
        break;
    }

    case 's':
        engine->dumpSensors();
        break;

    default:
        Serial.print("Unknown: ");
        Serial.println(cmd);
        break;
    }
}
