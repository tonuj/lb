#include <avr/io.h>
#include "uart.h"

#define HARDUART_BAUD       9600lu

void uart_init()
{
	uint32_t baud = ((F_CPU)/(16lu * HARDUART_BAUD)) - 1;

	UBRRH = (unsigned char)(baud>>8); 
	UBRRL = (unsigned char)baud; 

	UCSRB = _BV(RXEN) | _BV(TXEN); //| _BV(TXCIE) | _BV(RXCIE);
	UCSRC = _BV(URSEL) | _BV(UCSZ0) | _BV(UCSZ1); 
}

void uart_send_char(uint8_t c)
{
	while ( !( UCSRA & (1<<UDRE)) ) ;
	UDR = c; 
}

uint8_t uart_receive_char()
{
	while ( !(UCSRA & (1<<RXC))) ;
	return UDR;
}

void uart_send(const char * str)
{
	uint8_t c;
	while (( c = *(str++))) {
		uart_send_char(c);
	}
}

void uart_receive_str(char * dst, uint8_t maxlen)
{
	while((*(dst++) = uart_receive_char()) > 0 && (maxlen-- > 0)) ;
}
