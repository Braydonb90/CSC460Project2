
#ifndef __UART_H__
#define __UART_H__

#include <avr/io.h>
#include <stdio.h>


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif
#include <util/setbaud.h>

typedef enum _uart_bps
{
	UART_19200,
	UART_38400,
	UART_57600,
	UART_DEFAULT,
} UART_BPS;

#define UART_BUFFER_SIZE    32

void uart_init(UART_BPS bitrate);
void uart_putchar(uint8_t byte);
uint8_t uart_get_byte(int index);
uint8_t uart_bytes_received(void);
void uart_reset_receive(void);
void uart_print(uint8_t* output, int size);

void uart_init_debug(void);
void uart_putchar_debug(char c, FILE *stream);
char uart_getchar_debug(FILE *stream);
extern const FILE uart_output;
extern const FILE uart_input;

#endif