#ifndef UART_H
#define UART_H

#include <avr/pgmspace.h>

#include "sch.h"
#include "ringbuf.h"
#include "str.h"

typedef enum {
	UartDebug = 0,
	UartGPS = 1,
	UartGSM = 2,
	UartNull
} Uart;

#ifdef DEBUG
#define OUT(nr, fmt, args... ) { uart_send_rn(nr, pr_P(PSTR(fmt) , ## args )); }
#define OUTN(nr, fmt, args... ) { uart_send(nr, pr_P(PSTR(fmt) , ## args )); }

#define DBG(fmt, args... ) { uart_send_rn(UartDebug, pr_P(PSTR(fmt) , ## args )); }
#define DBGN(fmt, args... ) { uart_send(UartDebug, pr_P(PSTR(fmt) , ## args )); } 
#else
#define DBG(fmt, args... ) 
#define DBGN(fmt, args... ) 
#endif


/// #define UART_GPS    1
// #define UART_DEBUG  0
/// #define UART_GSM    2

#define ERR_OK       0
#define ERR_BUF_FULL 1

typedef struct __uart {
	SIGNAL *  gps_line_waiting; 
	RINGBUF * gps_rx;

	SIGNAL *  gsm_line_waiting; 
	SIGNAL *  gsm_byte_waiting; 
	RINGBUF * gsm_rx;
	RINGBUF * gsm_tx;

	SIGNAL *  debug_line_waiting; 
	SIGNAL *  debug_char_waiting; 
	uint8_t  debug_echo;
	RINGBUF * debug_rx;
	RINGBUF * debug_tx;

} UART_t;

extern UART_t UART;

void uart_init();

#endif
