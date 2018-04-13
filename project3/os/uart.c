
#include "uart.h"
/* http://www.ermicro.com/blog/?p=325 */

const FILE uart_output = (FILE)FDEV_SETUP_STREAM(uart_putchar_debug, NULL, _FDEV_SETUP_WRITE);
const FILE uart_input = (FILE)FDEV_SETUP_STREAM(NULL, uart_getchar_debug, _FDEV_SETUP_READ);

/* http://www.cs.mun.ca/~rod/Winter2007/4723/notes/serial/serial.html */


void uart_init_debug() {
	
	// For output debugging
		#if USE_2X
			UCSR0A |= _BV(U2X0);
		#else
			UCSR0A &= ~(_BV(U2X0));
		#endif

		UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */ 
		UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */    
		
		UBRR0 = MYBRR(9600);
}

void uart_putchar_debug(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar_debug('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

char uart_getchar_debug(FILE *stream) {
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}
/*
void uart_init(UART_BPS bitrate) {
    
	// For Roomba
	//UCSR1A = _BV(U2X1);
	UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
	//UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);

	int baud = 9600;
	//UBRR1H = 0;	
	switch(bitrate) {
		case UART_19200:
			//UBRR1L = 103;
			MYBBR(19200);
			printf("Bitrate 19200\n");
			break;
		case UART_38400:
			printf("Bitrate 38400\n");
			//UBRR1L = 51;
			MYBBR(38400);
			break;
		case UART_57600:
			printf("Bitrate 57600\n");
			//UBRR1L = 34;
			MYBBR(57600);
			break;
		default:
			//UBRR1L = 103;
			MYBBR(19200);
	}
	
    uart_buffer_index = 0;
}
*/


/**
 * Transmit one byte
 * NOTE: This function uses busy waiting
 *
 * @param byte data to trasmit
 */
void uart_putchar(uint8_t byte)
{
    /* wait for empty transmit buffer */
    while (!( UCSR1A & (1 << UDRE1)));

    /* Put data into buffer, sends the data */
    UDR1 = byte;
}

/**
 * Receive a single byte from the receive buffer
 *
 * @param index
 *
 * @return
 *
uint8_t uart_get_byte(int index)
{
    if (index < UART_BUFFER_SIZE)
    {
        return uart_buffer[index];
    }
    return 0;
}

/**
 * Get the number of bytes received on UART
 *
 * @return number of bytes received on UART
 *
uint8_t uart_bytes_received(void)
{
    return uart_buffer_index;
}

/**
 * Prepares UART to receive another payload
 *
 *
void uart_reset_receive(void)
{
    uart_buffer_index = 0;
}

/**
 * UART receive byte ISR
 *
ISR(USART1_RX_vect)
{
	printf("USART1_RX_vect\n");
	while(!(UCSR1A & (1<<RXC1)));
    uart_buffer[uart_buffer_index] = UDR1;
    uart_buffer_index = (uart_buffer_index + 1) % UART_BUFFER_SIZE;
}
/*
void uart_print(uint8_t* output, int size)
{
	uint8_t i;
	for (i = 0; i < size && output[i] != 0; i++)
	{
		uart_putchar(output[i]);
	}
}
*/

