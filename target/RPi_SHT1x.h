/*
Raspberry Pi SHT1x communication library.
By:      John Burns (www.john.geek.nz)
Date:    14 August 2012
License: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/

This is a derivative work based on
	Name: Nice Guy SHT11 library
	By: Daesung Kim
	Date: 04/04/2011
	Source: http://www.theniceguy.net/2722
*/

#ifndef RPI_SHT1x_H_
#define RPI_SHT1x_H_

// Includes
#include <unistd.h>
#include <stdio.h>

// Defines
#define	TRUE	1
#define	FALSE	0
#define SHT1x_DELAY usleep(2)

// Define the Raspberry Pi GPIO Pins for the SHT1x
#define RPI_GPIO_SHT1x_SCK  3
#define RPI_GPIO_SHT1x_DATA 4

/* Definitions of all known SHT1x commands */
#define SHT1x_MEAS_T	0x03			// Start measuring of temperature.
#define SHT1x_MEAS_RH	0x05			// Start measuring of humidity.
#define SHT1x_STATUS_R	0x07			// Read status register.
#define SHT1x_STATUS_W	0x06			// Write status register.
#define SHT1x_RESET	0x1E			// Perform a sensor soft reset.

void SHT1x_Transmission_Start( void );
unsigned char SHT1x_Readbyte( unsigned char sendAck );
unsigned char SHT1x_Sendbyte( unsigned char value );
void SHT1x_InitPins( void );
unsigned char SHT1x_Measure_Start( int type );
unsigned char SHT1x_Get_Measure_Value(unsigned short int * value );
void SHT1x_Reset(void);
unsigned char SHT1x_Mirrorbyte(unsigned char value);
void SHT1x_Xrc_check(unsigned char value);
void SHT1x_Calc(float *p_humidity ,float *p_temperature);
#endif
