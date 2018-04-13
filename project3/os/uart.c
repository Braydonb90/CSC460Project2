
#include "uart.h"
/* http://www.ermicro.com/blog/?p=325 */

FILE uart0_output = (FILE)FDEV_SETUP_STREAM(uart0_putc, NULL, _FDEV_SETUP_WRITE);
FILE uart0_input = (FILE)FDEV_SETUP_STREAM(NULL, uart0_getc, _FDEV_SETUP_READ);
FILE uart1_output = (FILE)FDEV_SETUP_STREAM(uart1_putc_stream, NULL, _FDEV_SETUP_WRITE);
FILE uart1_input = (FILE)FDEV_SETUP_STREAM(NULL, uart1_getc_stream, _FDEV_SETUP_WRITE);

/* http://www.cs.mun.ca/~rod/Winter2007/4723/notes/serial/serial.html */


static volatile uint8_t uart1_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart1_buffer_index;

static volatile uint8_t uart2_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart2_buffer_index;

void uart0_init() {
	
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

void uart0_putc(char c, FILE *stream) {
    if (c == '\n') {
        uart0_putc('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

char uart0_getc(FILE *stream) {
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

void uart1_init(uint16_t ubrr_value) {
    	
	// For Roomba
	UBRR1L = (uint8_t) ubrr_value;
	UBRR1H = (ubrr_value>>8);
	
	UCSR1B = (1<<TXEN1)|(1<<RXEN1)|(1<<RXCIE1);
	
    uart1_buffer_index = 0;
}

void uart2_init(uint16_t ubrr_value) {
    	
	// For Roomba
	UBRR2L = (uint8_t) ubrr_value;
	UBRR2H = (ubrr_value>>8);
	
	UCSR2B = (1<<TXEN2)|(1<<RXEN2)|(1<<RXCIE2);
	
    uart2_buffer_index = 0;
}



/**
 * Transmit one byte
 * NOTE: This function uses busy waiting
 *
 * @param byte data to trasmit
 */
void uart1_putc(char byte)
{
    /* wait for empty transmit buffer */
    //while (!( UCSR1A & (UDRE1)));
	while (!( UCSR1A & (1 << UDRE1)));

    /* Put data into buffer, sends the data */
    UDR1 = byte;
}
void uart1_putc_stream(char c, FILE *stream) {
    if (c == '\n') {
        uart1_putc_stream('\r', stream);
    }
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = c;
}

char uart1_getc_stream(FILE *stream) {
    loop_until_bit_is_set(UCSR1A, RXC1);
    return UDR1;
}

void uart2_putc(char byte)
{
    /* wait for empty transmit buffer */
    //while (!( UCSR1A & (UDRE1)));
	while (!( UCSR2A & (1 << UDRE2)));

    /* Put data into buffer, sends the data */
    UDR2 = byte;
}

/**
 * Receive a single byte from the receive buffer
 *
 * @param index
 *
 * @return
 */
uint8_t uart1_get_byte(int index)
{
    if (index < UART_BUFFER_SIZE)
    {
        return uart1_buffer[index];
    }
    return 0;
}

uint8_t uart2_get_byte(int index)
{
    if (index < UART_BUFFER_SIZE)
    {
        return uart2_buffer[index];
    }
    return 0;
}

/**
 * Get the number of bytes received on UART
 *
 * @return number of bytes received on UART
 */
uint8_t uart1_bytes_received(void)
{
    return uart1_buffer_index;
}

uint8_t uart2_bytes_received(void)
{
    return uart2_buffer_index;
}

/**
 * Prepares UART to receive another payload
 *
 */
void uart1_reset_receive(void)
{
    uart1_buffer_index = 0;
}

void uart2_reset_receive(void)
{
    uart2_buffer_index = 0;
}

/**
 * UART receive byte ISR
 */
ISR(USART1_RX_vect)
{
//	printf("USART1_RX_vect\n");
	while(!(UCSR1A & (1<<RXC1)));
    uart1_buffer[uart1_buffer_index] = UDR1;
    uart1_buffer_index = (uart1_buffer_index + 1) % UART_BUFFER_SIZE;
}

ISR(USART2_RX_vect)
{
//	printf("USART2_RX_vect\n");
	while(!(UCSR2A & (1<<RXC2)));
    uart2_buffer[uart2_buffer_index] = UDR2;
    uart2_buffer_index = (uart2_buffer_index + 1) % UART_BUFFER_SIZE;
}

void uart1_print(uint8_t* output, int size)
{
	uint8_t i;
	for (i = 0; i < size; i++)
	{
		uart1_putc(output[i]);
	}
}

void uart2_print(uint8_t* output, int size)
{
	uint8_t i;
	for (i = 0; i < size && output[i] != 0; i++)
	{
		uart2_putc(output[i]);
	}
}


