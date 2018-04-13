/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include <util/delay.h>
#include <math.h>
#include "../os/os.h"
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

uint8_t power_colour = POWER_RED;		// red
uint8_t power_intensity = 255;	// full intensity

ROOMBA_STATE state = SAFE_MODE;
MOVE_STATE m_state = STAND_MODE;


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

	//Roomba_UART_Init(UART_19200);
	uart2_init(BAUD_CALC(19200));
	//asm volatile ("sei"::);
	
	// start the Roomba's SCI
	uart2_putc(START);
	_delay_ms(20);
	
	

	uart2_putc(BAUD_RATE);
	uart2_putc(ROOMBA_38400BPS);
	_delay_ms(100);

	// change the AVR's UART clock to the new baud rate.
	uart2_init(BAUD_CALC(38400));

	// put the Roomba into safe mode.
	uart2_putc(CONTROL);
	_delay_ms(20);
	printf("Roomba_Init complete\n");
}

/**
 * Use this function instead of the while loops in Roomba_UpdateSensorPacket if you have a system
 * clock.  This will add a timeout when it's waiting for the bytes to come in, so that the
 * function doesn't enter an infinite loop if a byte is missed.  You'll have to modify this function
 * and insert it into Roomba_UpdateSensorPacket to suit your application.
 */

uint8_t wait_for_bytes(uint8_t num_bytes, uint8_t timeout)
{
	uint16_t start;
	start = Now();	// current system time
	while (Now() - start < timeout && uart2_bytes_received() < num_bytes);
	if (uart2_bytes_received() >= num_bytes)
		return TRUE;
	else
		return FALSE;
}

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{
	uart2_reset_receive();
	uart2_putc(SENSORS);
	uart2_putc(group);
	switch(group)
	{
	case EXTERNAL:
		// environment sensors
		if(wait_for_bytes(10, 50) == FALSE) {printf("Failed\n"); break;}
		//while (uart_bytes_received() != 10);
		sensor_packet->bumps_wheeldrops = uart2_get_byte(0);
		sensor_packet->wall = uart2_get_byte(1);
		sensor_packet->cliff_left = uart2_get_byte(2);
		sensor_packet->cliff_front_left = uart2_get_byte(3);
		sensor_packet->cliff_front_right = uart2_get_byte(4);
		sensor_packet->cliff_right = uart2_get_byte(5);
		sensor_packet->virtual_wall = uart2_get_byte(6);
		sensor_packet->motor_overcurrents = uart2_get_byte(7);
		sensor_packet->dirt_left = uart2_get_byte(8);
		sensor_packet->dirt_right = uart2_get_byte(9);
		break;
	case CHASSIS:
		// chassis sensors
		if(wait_for_bytes(6, 50) == FALSE) break;
		//while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart2_get_byte(0);
		sensor_packet->buttons = uart2_get_byte(1);
		sensor_packet->distance.bytes.high_byte = uart2_get_byte(2);
		sensor_packet->distance.bytes.low_byte = uart2_get_byte(3);
		sensor_packet->angle.bytes.high_byte = uart2_get_byte(4);
		sensor_packet->angle.bytes.low_byte = uart2_get_byte(5);
		break;
	case INTERNAL:
		// internal sensors
		if(wait_for_bytes(10, 50) == FALSE) break;
		//while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart2_get_byte(0);
		sensor_packet->voltage.bytes.high_byte = uart2_get_byte(1);
		sensor_packet->voltage.bytes.low_byte = uart2_get_byte(2);
		sensor_packet->current.bytes.high_byte = uart2_get_byte(3);
		sensor_packet->current.bytes.low_byte = uart2_get_byte(4);
		sensor_packet->temperature = uart2_get_byte(5);
		sensor_packet->charge.bytes.high_byte = uart2_get_byte(6);
		sensor_packet->charge.bytes.low_byte = uart2_get_byte(7);
		sensor_packet->capacity.bytes.high_byte = uart2_get_byte(8);
		sensor_packet->capacity.bytes.low_byte = uart2_get_byte(9);
		break;
	}
	uart2_reset_receive();
}

void Roomba_ChangeState(ROOMBA_STATE newState)
{
	if (newState == SAFE_MODE)
	{
		if (state == PASSIVE_MODE)
			uart2_putc(CONTROL);
		else if (state == FULL_MODE)
			uart2_putc(SAFE);
	}
	else if (newState == FULL_MODE)
	{
		Roomba_ChangeState(SAFE_MODE);
		uart2_putc(FULL);
	}
	else if (newState == PASSIVE_MODE)
	{
		uart2_putc(POWER);
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
	uart2_putc(DRIVE);
	
	if(m_state == STAND_MODE) 
	{
		velocity = 0;
		radius = (radius > 0) ? 1 : -1;
	}
	
	uart2_putc(HIGH_BYTE(velocity));
	uart2_putc(LOW_BYTE(velocity));
	uart2_putc(HIGH_BYTE(radius));
	uart2_putc(LOW_BYTE(radius));
}

/**
 * Update the LEDs on the Roomba to match the configured state
 */
void update_leds()
{
	// The status, spot, clean, max, and dirt detect LED states are combined in a single byte.
	uint8_t leds = status << 4 | spot << 3 | clean << 2 | max << 1 | dd;

	uart2_putc(LEDS);
	uart2_putc(leds);
	uart2_putc(power_colour);
	uart2_putc(power_intensity);
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

	uart2_putc(SONG);
	uart2_putc(songNum);
	uart2_putc(numNotes);

	for (i=0; i<numNotes; i++)
	{
		uart2_putc(notes[i]);
		uart2_putc(notelengths[i]);
	}
}

void Roomba_PlaySong(int songNum)
{
	uart2_putc(PLAY);
	uart2_putc(songNum);
}

void Roomba_ChangeDriveState() 
{
	m_state = (m_state == CRUISE_MODE) ? STAND_MODE : CRUISE_MODE;
	Roomba_ConfigPowerLED((m_state == CRUISE_MODE) ? POWER_RED : POWER_BLUE, 255);
}

uint8_t Roomba_BumperActivated(roomba_sensor_data_t* sensor_data)
{
	// if either of the bumper bits is set, then return true.
	return (sensor_data->bumps_wheeldrops & 0x03) != 0;
}

uint8_t Roomba_RiverHit(roomba_sensor_data_t* sensor_data)
{
	// if virtual wall is set, then return true.
	return (sensor_data->virtual_wall) == 1;
}

