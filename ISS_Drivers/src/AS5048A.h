/**
 * AS5048ADriver.h — AS5048A Magnetic Angle Sensor (Wind Direction)
 *
 * ISS Packet Engine — Sensor Driver Architecture
 *
 * Reads absolute angular position from a diametrically magnetized
 * disc magnet via I2C. 14-bit resolution (0.022°/LSB).
 * Contactless — no potentiometer dead zone, no wear.
 *
 * Davis direction byte encoding:
 *   davis_dir = (uint8_t)(angle_deg * 255.0 / 360.0)
 *   Direction 0x00 = "no reading" in the Davis protocol
 * NOTE: AS45058A is the SPI version 
 * Fork from: https://github.com/eborghi10/AS5048A/tree/master
 */

#ifndef AS5048A_H
#define AS5048A_H

#include "SensorDriver.h"

//#ifndef as5048_h
//#define as5048_h

#include <SPI.h>

// 14-bit resolution: 16384 positions per revolution
#define AS5048A_RESOLUTION     16384

class AS5048A
{
private:
	uint8_t _cs;
	bool errorFlag;
	bool ocfFlag; // Avoid printing OCF flag everytime
	uint16_t position;
	bool debug;
	uint8_t esp32_delay;

	SPISettings settings;

	uint8_t spiCalcEvenParity(uint16_t);

	/**
	 * Set the delay acording to the microcontroller architecture
	 */
	void setDelay();

	/**
	 * Closes the SPI connection
	 */
	void close();

	/*
	 * Read a register from the sensor
	 * Takes the address of the register as a 16 bit uint16_t
	 * Returns the value of the register
	 */
	uint16_t read(uint16_t registerAddress);

	/*
	 * Write to a register
	 * Takes the 16-bit  address of the target register and the 16 bit uint16_t of data
	 * to be written to that register
	 * Returns the value of the register after the write has been performed. This
	 * is read back from the sensor to ensure a sucessful write.
	 */
	uint16_t write(uint16_t registerAddress, uint16_t data);

	/**
	 * Get the rotation of the sensor relative to the zero position.
	 *
	 * @return {int16_t} between -2^13 and 2^13
	 */
	int16_t getRotation();

	/*
	 * Check if an error has been encountered.
	 */
	bool error();

public:
	/**
	 *	Constructor
	 */
	AS5048A(uint8_t arg_cs, bool debug = false);

	/**
	 * Initialiser
	 * Sets up the SPI interface
	 */
	void begin();

	/**
	 * Returns the raw angle directly from the sensor
	 */
	int16_t getRawRotation();

	/**
	 * Get the rotation of the sensor relative to the zero position in degrees.
	 *
	 * @return {double} between 0 and 360
	 */
	double getRotationInDegrees();

	/**
	 * Get the rotation of the sensor relative to the zero position in radians.
	 *
	 * @return {double} between 0 and 2 * PI
	 */
	double getRotationInRadians();

	/**
	 * returns the value of the state register
	 * @return 16 bit uint16_t containing flags
	 */
	uint16_t getState();

	/**
	 * Print the diagnostic register of the sensor
	 */
	void printState();

	/**
	 * Returns the value used for Automatic Gain Control (Part of diagnostic
	 * register)
	 */
	uint8_t getGain();

	/*
	 * Get and clear the error register by reading it
	 */
	String getErrors();

	/**
	 * Get diagnostic
	 */
	String getDiagnostic();

	/*
	 * Set the zero position
	 */
	void setZeroPosition(uint16_t arg_position);

	/*
	 * Returns the current zero position
	 */
	uint16_t getZeroPosition();
};
#endif // AS5048A_DRIVER_H
