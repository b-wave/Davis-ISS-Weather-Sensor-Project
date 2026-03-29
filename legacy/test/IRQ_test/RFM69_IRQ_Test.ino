/*
This is quick test to RFM69 to verify the IRQ hardware
Added standalone IRQ test for validating DIO0 → Teensy interrupt chain
Useful as a bench diagnostic tool before integrating full RX logic

REVISIONS: 
0.1  First test using Teensy 3.2 
  -Includes temporary register defines missing from the Davis fork
  -Confirms FIFO drain + RX restart logic
  -Observed real ISS timing patterns (40–70s gaps, double IRQs at hop boundaries)
  -Sketch will also trigger on RF noise or partial packets — expected behavior
   -First Run: 
              16:48:17.985 -> RFM69 IRQ Test
              16:48:17.985 -> Setting up Pins...
              16:48:17.985 -> Reset RFM69...
              16:48:17.985 -> 
              16:48:17.985 -> Set up IRQ
              16:48:17.985 -> Waiting for IRQ...
              16:49:12.868 -> >>> IRQ fired! DIO0 is working.
*/
#include <SPI.h>
#include "DavisRFM69_Teensy.h"

// Temporary RFM69 register defines for IRQ test
#define REG_IRQFLAGS2              0x28
#define RF_IRQFLAGS2_PAYLOADREADY  0x04
#define RF_IRQFLAGS2_FIFOOVERRUN   0x10

#define REG_FIFO                   0x00
#define REG_OPMODE                 0x01

#define RF_OPMODE_STANDBY          0x04
#define RF_OPMODE_RECEIVER         0x10


#define RFM69_RST     3
#define RF69_SPI_CS   SS // 10
#define RF69_IRQ_PIN  2 //  
#define RF69_IRQ_NUM  2  // 

#define LED          13  // Moteinos have LEDs on D9
volatile bool irqFired = false;

void isr() {
  irqFired = true;
}

DavisRFM69 radio(RF69_SPI_CS, RF69_IRQ_PIN , false);

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("RFM69 IRQ Test");
  Serial.println("Setting up Pins...");  
  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);

  Serial.println("Reset RFM69...");
  Serial.println();
    // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  Serial.println("Set up IRQ");
   delay(100);
  //Set up IRQ
  pinMode(RF69_IRQ_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RF69_IRQ_PIN), isr, RISING);

  radio.initialize();
  radio.setChannel(0);   
  Serial.println("Waiting for IRQ...");
}

void loop() 
{
  if (irqFired) {
    irqFired = false;
    Serial.println(">>> IRQ fired! DIO0 is working.");

    // 1. Clear IRQ flags first (write 1s to clear)
    radio.writeReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN | RF_IRQFLAGS2_PAYLOADREADY);

    // 2. Drain FIFO completely
    while (radio.readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
      radio.readReg(REG_FIFO);
    }

    // 3. Restart RX mode (forces PayloadReady to drop)
    radio.writeReg(REG_OPMODE, RF_OPMODE_STANDBY);
    delayMicroseconds(100);
    radio.writeReg(REG_OPMODE, RF_OPMODE_RECEIVER);
  }
}



