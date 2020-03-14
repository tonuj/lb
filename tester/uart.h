
#ifndef UART_H
#define UART_H

#include <inttypes.h>
#include <avr/pgmspace.h>


void uart_init();
void uart_send_char(uint8_t c);
uint8_t uart_receive_char();
void uart_send(const char *str);

#endif
