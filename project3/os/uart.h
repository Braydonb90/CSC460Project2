
#ifndef __UART_H__
#define __UART_H__

#include <avr/io.h>
#include <stdio.h>
#include "common.h"


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define MYBRR(baud_rate) (F_CPU / 16 / (baud_rate) - 1)


void uart_init_debug(void);
void uart_putchar_debug(char c, FILE *stream);
char uart_getchar_debug(FILE *stream);
extern const FILE uart_output;
extern const FILE uart_input;

#endif