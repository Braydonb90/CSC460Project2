
#ifndef __UART_H__
#define __UART_H__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "common.h"


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define MYBRR(baud_rate) (F_CPU / 16 / (baud_rate) - 1)
#define BAUD_CALC(x) ((F_CPU+(x)*8UL) / (16UL*(x))-1UL)

#define UART_BUFFER_SIZE    32

void uart0_init(void);
void uart0_putc(char c, FILE *stream);
char uart0_getc(FILE *stream);

extern FILE uart0_output;
extern FILE uart0_input;
extern FILE uart1_output;
extern FILE uart1_input;

void uart1_init(uint16_t ubbr_value);
void uart2_init(uint16_t ubbr_value);

void uart1_putc(char byte);
void uart1_putc_stream(char byte, FILE* stream);
char uart1_getc_stream(FILE* stream);
void uart2_putc(char byte);

void uart1_print(uint8_t* output, int size);
void uart2_print(uint8_t* output, int size);

void uart1_reset_receive(void);
void uart2_reset_receive(void);

uint8_t uart1_bytes_received(void);
uint8_t uart2_bytes_received(void);

uint8_t uart1_get_byte(int index);
uint8_t uart2_get_byte(int index);


#endif
