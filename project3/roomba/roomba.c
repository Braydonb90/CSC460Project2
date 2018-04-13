/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include <util/delay.h>
#include "roomba_uart.h"
#include "../os/common.h"
#include "roomba.h"
#include "roomba_sci.h"

#define DD_DDR DDRC
#define DD_PORT PORTC
#define DD_PIN PC5

STATUS_LED_STATE status = LED_OFF;
LED_STATE spot = LED_OFF;
LED_STATE clean = LED_OFF;
LED_STATE max = LED_OFF;
LED_STATE dd = LED_OFF;
uint8_t power_colour = 128;		// green
uint8_t power_intensity = 255;	// full intensity

ROOMBA_STATE state = SAFE_MODE;

static void update_leds();

void Roomba_Init()
{
	printf("Roomba_Init\n");
	uint8_t i;
	BIT_SET(DD_DDR, DD_PIN);
			
	// Wake up the Roomba by driving the DD pin low for 500 ms.
	BIT_RESET(DD_PORT, DD_PIN);
	_delay_ms(500);
	BIT_SET(DD_PORT, DD_PIN);

	// Wait for 2 seconds, Then pulse the DD pin 3 times to set the Roomba to operate at 19200 baud.
	// This ensures that we know what baud rate to talk at.
	_delay_ms(2000);
	for (i = 0; i < 3; i++)
	{
		BIT_RESET(DD_PORT, DD_PIN);
		_delay_ms(250);
		BIT_SET(DD_PORT, DD_PIN);
		_delay_ms(250);
	}

	Roomba_UART_Init(UART_19200);

	// start the Roomba's SCI
	Roomba_Send_Byte(START);
	_delay_ms(20);
	
	

	Roomba_Send_Byte(BAUD_RATE);
	Roomba_Send_Byte(ROOMBA_38400BPS);
	_delay_ms(100);

	// change the AVR's UART clock to the new baud rate.
	Roomba_UART_Init(UART_38400);

	// put the Roomba into safe mode.
	Roomba_Send_Byte(CONTROL);
	_delay_ms(20);

	// Set the Roomba's LEDs to the defaults defined above (to verify defaults).
	//update_leds();
	Roomba_Drive(100, 32768);
	printf("Roomba_Init finish\n");
}

/**
 * Use this function instead of the while loops in Roomba_UpdateSensorPacket if you have a system
 * clock.  This will add a timeout when it's waiting for the bytes to come in, so that the
 * function doesn't enter an infinite loop if a byte is missed.  You'll have to modify this function
 * and insert it into Roomba_UpdateSensorPacket to suit your application.
 */
/*
uint8_t wait_for_bytes(uint8_t num_bytes, uint8_t timeout)
{
	uint16_t start;
	start = Now();	// current system time
	while (Now() - start < timeout && uart_bytes_received() != num_bytes);
	if (uart_bytes_received() >= num_bytes)
		return 1;
	else
		return 0;
}*/

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{
	// No, I don't feel bad about manual loop unrolling.
	uart_putchar(SENSORS);
	uart_putchar(group);
	switch(group)
	{
	case EXTERNAL:
		// environment sensors
		while (uart_bytes_received() != 10);
		sensor_packet->bumps_wheeldrops = uart_get_byte(0);
		sensor_packet->wall = uart_get_byte(1);
		sensor_packet->cliff_left = uart_get_byte(2);
		sensor_packet->cliff_front_left = uart_get_byte(3);
		sensor_packet->cliff_front_right = uart_get_byte(4);
		sensor_packet->cliff_right = uart_get_byte(5);
		sensor_packet->virtual_wall = uart_get_byte(6);
		sensor_packet->motor_overcurrents = uart_get_byte(7);
		sensor_packet->dirt_left = uart_get_byte(8);
		sensor_packet->dirt_right = uart_get_byte(9);
		break;
	case CHASSIS:
		// chassis sensors
		while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart_get_byte(0);
		sensor_packet->buttons = uart_get_byte(1);
		sensor_packet->distance.bytes.high_byte = uart_get_byte(2);
		sensor_packet->distance.bytes.low_byte = uart_get_byte(3);
		sensor_packet->angle.bytes.high_byte = uart_get_byte(4);
		sensor_packet->angle.bytes.low_byte = uart_get_byte(5);
		break;
	case INTERNAL:
		// internal sensors
		while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart_get_byte(0);
		sensor_packet->voltage.bytes.high_byte = uart_get_byte(1);
		sensor_packet->voltage.bytes.low_byte = uart_get_byte(2);
		sensor_packet->current.bytes.high_byte = uart_get_byte(3);
		sensor_packet->current.bytes.low_byte = uart_get_byte(4);
		sensor_packet->temperature = uart_get_byte(5);
		sensor_packet->charge.bytes.high_byte = uart_get_byte(6);
		sensor_packet->charge.bytes.low_byte = uart_get_byte(7);
		sensor_packet->capacity.bytes.high_byte = uart_get_byte(8);
		sensor_packet->capacity.bytes.low_byte = uart_get_byte(9);
		break;
	}
	uart_reset_receive();
}


void Roomba_ChangeState(ROOMBA_STATE newState)
{
	if (newState == SAFE_MODE)
	{
		if (state == PASSIVE_MODE)
			uart_putchar(CONTROL);
		else if (state == FULL_MODE)
			uart_putchar(SAFE);
	}
	else if (newState == FULL_MODE)
	{
		Roomba_ChangeState(SAFE_MODE);
		uart_putchar(FULL);
	}
	else if (newState == PASSIVE_MODE)
	{
		uart_putchar(POWER);
	}
	else
	{
		// already in the requested state
		return;
	}

	state = newState;
	_delay_ms(20);
}

void Roomba_Drive( int16_t velocity, int16_t radius )
{
	uart_putchar(DRIVE);
	uart_putchar(HIGH_BYTE(velocity));
	uart_putchar(LOW_BYTE(velocity));
	uart_putchar(HIGH_BYTE(radius));
	uart_putchar(LOW_BYTE(radius));
}

/**
 * Update the LEDs on the Roomba to match the configured state
 */
void update_leds()
{
	// The status, spot, clean, max, and dirt detect LED states are combined in a single byte.
	uint8_t leds = status << 4 | spot << 3 | clean << 2 | max << 1 | dd;

	uart_putchar(LEDS);
	uart_putchar(leds);
	uart_putchar(power_colour);
	uart_putchar(power_intensity);
}

void Roomba_ConfigPowerLED(uint8_t colour, uint8_t intensity)
{
	power_colour = colour;
	power_intensity = intensity;
	update_leds();
}

void Roomba_ConfigStatusLED(STATUS_LED_STATE state)
{
	status = state;
	update_leds();
}

void Roomba_ConfigSpotLED(LED_STATE state)
{
	spot = state;
	update_leds();
}

void Roomba_ConfigCleanLED(LED_STATE state)
{
	clean = state;
	update_leds();
}

void Roomba_ConfigMaxLED(LED_STATE state)
{
	max = state;
	update_leds();
}

void Roomba_ConfigDirtDetectLED(LED_STATE state)
{
	dd = state;
	update_leds();
}

void Roomba_LoadSong(uint8_t songNum, uint8_t* notes, uint8_t* notelengths, uint8_t numNotes)
{
	uint8_t i = 0;

	uart_putchar(SONG);
	uart_putchar(songNum);
	uart_putchar(numNotes);

	for (i=0; i<numNotes; i++)
	{
		uart_putchar(notes[i]);
		uart_putchar(notelengths[i]);
	}
}

void Roomba_PlaySong(int songNum)
{
	uart_putchar(PLAY);
	uart_putchar(songNum);
}

uint8_t Roomba_BumperActivated(roomba_sensor_data_t* sensor_data)
{
	// if either of the bumper bits is set, then return true.
	return (sensor_data->bumps_wheeldrops & 0x03) != 0;
}

