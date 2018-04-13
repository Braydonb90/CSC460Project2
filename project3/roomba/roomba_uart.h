
#ifndef ROOMBA_UART_H_
#define ROOMBA_UART_H_
#include <avr/io.h>

typedef enum _uart_bps
{
	UART_19200,
	UART_38400,
	UART_57600,
	UART_115200,
	UART_DEFAULT,
} UART_BPS;

#define UART_BUFFER_SIZE    32

void Roomba_Send_Byte(uint8_t data_out);
void Roomba_UART_Init(UART_BPS baud);

uint8_t uart_bytes_received(void);

void uart_reset_receive(void);

uint8_t uart_get_byte(int index);

#endif