
#include <RFM69registers.h>
#include <RFM69.h>
#include "DavisRadio.h"
#include "DavisConfig.h"

RFM69 radio;
DavisRadio davis(radio);

uint8_t hopIndex = 0;
uint32_t lastTx = 0;

//Pins

#define RFM69_RST     2
#define RF69_SPI_CS    SS // 10
//#define RF69_IRQ_PIN   3 //  digitalPinToInterrupt(2)
//#define RF69_IRQ_NUM   3  // Its in DavidRFM69.h but just to make sure for T3
#define RFM69_INT     digitalPinToInterrupt(2)

#define LED          14  // Moteinos have LEDs on D9
#define SERIAL_BAUD   115200

void blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
void setup() {
 //   Serial.begin(115200);
    Serial.begin(SERIAL_BAUD);
    delay(3000);

  Serial.println("Setting up Pins...");  
  //For Teensy 3.x and T4.x the following format is required to operate correctly
  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Teensy RFM69 DAVIS ISS TX Test!");
  Serial.println();
  blink(LED,300);

  Serial.println("Reset RFM69...");
  Serial.println();
    
// Toggle the reset line
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  //----- END TEENSY CONFIG

    Serial.println("Davis ISS Test Transmitter Starting...");
    davis.begin();
}

void loop() {
    uint32_t now = millis();

    if (now - lastTx >= DAVIS_TX_INTERVAL_MS) {
        lastTx = now;

        uint8_t packet[10];
        davis.buildTestPacket(packet, 12, 270);   // wind 12 mph, dir 270°

        davis.sendPacket(packet, 10, hopIndex);

        Serial.print("TX hop ");
        Serial.print(hopIndex);
        Serial.println(" sent.");
            delay(3000);
 blink(LED,500);
        hopIndex++;
        if (hopIndex >= 51)
            hopIndex = 0;
    }

    

}
