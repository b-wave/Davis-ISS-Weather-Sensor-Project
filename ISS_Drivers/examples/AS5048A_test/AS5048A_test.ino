/**
This is just a quick test to check hardware. 
We are using an AS5048A w/ SPI interface
Works! 
CS= Pin 9 (CS2) on the Teensy 3.2

*/

#include <AS5048A.h> //Very quick & dirty, Need: AS5048ADriver + SensorDriver.h 

AS5048A angleSensor(9, true); 

void setup()
{
	Serial.begin(19200);
	angleSensor.begin();
}

void loop()
{
	delay(1000);

	float val = angleSensor.getRotationInDegrees();
	Serial.print("\nRotation: ");
	Serial.println(val);
	Serial.println("State: ");
	angleSensor.printState();
	Serial.println("Errors: ");
	Serial.println(angleSensor.getErrors());
	Serial.println("Diagnostics: ");
	Serial.println(angleSensor.getDiagnostic());
}
