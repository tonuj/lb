/*
   full duplex hard-uart
   half duplex soft-uart
   tx is immediately halted in case of started rx
   tx is not started if rx in progress

   trust rx, dont't trust tx.
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

#define UART_GSM_TX_BUF_SIZE 150
#define UART_GSM_RX_BUF_SIZE 150
#define UART_DEBUG_RX_BUF_SIZE 70
#define UART_DEBUG_TX_BUF_SIZE 120
#define UART_GPS_TX_BUF_SIZE 4
#define UART_GPS_RX_BUF_SIZE 150

//#define HARDUART_BAUD      19200 
#define HARDUART_BAUD       9600lu
//#define SOFTUART_COUNTER_VALUE  22
//#define SOFTUART_COUNTER_VALUE15 33
#define GPS_COUNTER_VALUE10  188
#define GPS_COUNTER_VALUE15 250

#define DEBUG_COUNTER_VALUE10  94 
#define DEBUG_COUNTER_VALUE15 125

#define UARTS_FREE      0
#define UARTS_TX        1
#define UARTS_TX_STOP   2 //  this is when tx just has se(n)t stop bit
#define UARTS_RX        3
#define UARTS_CLOSED    4

UART_t UART;

static void open_hard();
static void close_hard();
static char send_char_hard(unsigned char c);
static void open_soft(unsigned char nr);
static void close_soft(unsigned char nr);
static char send_char_soft(unsigned char nr, unsigned char c);

static volatile unsigned char hardStatus = UARTS_FREE;
static volatile unsigned char softStatus[2] = { UARTS_FREE, UARTS_FREE };
static volatile unsigned char rxMask[2];
static volatile unsigned char txMask[2];
static volatile unsigned char rxData[2];
static volatile unsigned char txData[2];

static volatile RINGBUF *hardRxBuf[1];
static volatile RINGBUF *hardTxBuf[1];
static volatile RINGBUF *softRxBuf[2];
static volatile RINGBUF *softTxBuf[2];

static RINGBUF devNull;
static char devNullData[5];


static RINGBUF gpsTxBuf[1];
static char gpsTxData[UART_GPS_TX_BUF_SIZE];
static RINGBUF gpsRxBuf[1];
static char gpsRxData[UART_GPS_RX_BUF_SIZE];

static RINGBUF debugRxBuf[1];
static char debugRxData[UART_DEBUG_RX_BUF_SIZE];
static RINGBUF debugTxBuf[1];
static char debugTxData[UART_DEBUG_TX_BUF_SIZE];

static RINGBUF gsmTxBuf[1];
static char gsmTxData[UART_GSM_TX_BUF_SIZE]; 
static RINGBUF gsmRxBuf[1];
static char gsmRxData[UART_GSM_RX_BUF_SIZE]; 



void uart_init()
{
	ringbuf_init(gsmRxBuf, gsmRxData, UART_GSM_RX_BUF_SIZE);
	ringbuf_init(gsmTxBuf, gsmTxData, UART_GSM_TX_BUF_SIZE);

	ringbuf_init(gpsTxBuf, gpsTxData, UART_GPS_TX_BUF_SIZE);
	ringbuf_init(gpsRxBuf, gpsRxData, UART_GPS_RX_BUF_SIZE);

	ringbuf_init(debugRxBuf, debugRxData, UART_DEBUG_RX_BUF_SIZE);
	ringbuf_init(debugTxBuf, debugTxData, UART_DEBUG_TX_BUF_SIZE);

	uart_open(UART_GSM,  gsmRxBuf, gsmTxBuf);
	uart_open(UART_GPS,  gpsRxBuf, gpsTxBuf);
	uart_open(UART_DEBUG, debugRxBuf, debugTxBuf);

	UART.gps_line_waiting = &(gpsRxBuf->nlReady);
	UART.gps_rx = gpsRxBuf;

	UART.gsm_line_waiting = &(gsmRxBuf->nlReady);
	UART.gsm_byte_waiting = &(gsmRxBuf->flagReadyRead);
	UART.gsm_rx = gsmRxBuf;
	UART.gsm_tx = gsmTxBuf;

	UART.debug_line_waiting = &(debugRxBuf->nlReady);
	UART.debug_char_waiting = &(debugRxBuf->flagReadyRead);
	UART.debug_echo = 0;
	UART.debug_rx = debugRxBuf;
	UART.debug_tx = debugTxBuf;
}

void uart_open(unsigned char nr, RINGBUF * rxBuf, RINGBUF * txBuf)
{
	static char wasHere = 0;
	if (wasHere == 0) {
		wasHere = 1;
		ringbuf_init(&devNull, &devNullData[0], 5);
	}

	if (rxBuf == 0) rxBuf = &devNull; 
	if (txBuf == 0) txBuf = &devNull;

	if (nr < 2) {
		softRxBuf[nr] = rxBuf;
		softTxBuf[nr] = txBuf;

		open_soft(nr);
		
	} else {
		hardRxBuf[0] = rxBuf;
		hardTxBuf[0] = txBuf;

		open_hard();
	}
}

void uart_close(unsigned char nr)
{
	if (nr < 2)
		close_soft(nr);
	else
		close_hard();
}

void uart_send_raw(unsigned char nr, const void * data, uint16_t len)
{
	uint8_t c;
	uint16_t i;
	const uint8_t * ptr = (const uint8_t *) data;

	for (i=0; i<len; i++) {
		c  = *(ptr++);

		if (nr < 2) {
			send_char_soft(nr, c);
		} else {
			send_char_hard(c);
		}
	}
}

void uart_send(unsigned char nr, const char * str)
{
	char c;

	for (;;) {
		c  = *(str++);
		if (c==0) break;

		if (nr < 2) {
			send_char_soft(nr, c);
		} else {
			send_char_hard(c);
		}
	}
}

void uart_send_P(unsigned char nr, PGM_P str)
{
	char c;

	for (;;) {
		c  = pgm_read_byte(str++);
		if (c==0) break;

		if (nr < 2) {
			send_char_soft(nr, c);
		} else {
			send_char_hard(c);
		}
	}
}

char uart_send_char(unsigned char nr, unsigned char c)
{
	if (nr < 2) {
#ifdef TELNET_VERSION
		if (nr == UART_DEBUG && c == '\r') return 0;
#endif

		return send_char_soft(nr, c);
	} else {
		return send_char_hard(c);
	}
}

//// -- private stuff ----

void open_hard()
{
	uint32_t baud = ((F_CPU)/(16lu * HARDUART_BAUD)) - 1;

	//baud = F_CPU / (16 * baud) - 1;

	UBRRH = (unsigned char)(baud>>8); 
	UBRRL = (unsigned char)baud; 

	UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(TXCIE) | _BV(RXCIE);
	UCSRC = _BV(URSEL) | _BV(UCSZ0) | _BV(UCSZ1); 
}

void close_hard()
{
	UCSRB = 0;
}

char send_char_hard(unsigned char c)
{
	if (hardTxBuf[0]->flagFull == 1) 
		return ERR_BUF_FULL;


	if (hardTxBuf[0]->count > 0 || hardStatus != UARTS_FREE) {
		ringbuf_append(hardTxBuf[0], c);
		return ERR_OK;
	}

	if (hardTxBuf[0]->count == 0 && hardStatus == UARTS_FREE) {
		UDR = c;
	} else if (hardTxBuf[0]->count > 0 && hardStatus == UARTS_FREE) {
		UDR = ringbuf_take(hardTxBuf[0]);
	}
//		UDR = ringbuf_take(hardTxBuf[0]);

	hardStatus = UARTS_TX;

	return ERR_OK;
}

void open_soft(unsigned char nr)
{
	cli();

	if (nr == UART_GPS) { // GPS
		TCCR0 &= ~(_BV(WGM00) | _BV(CS02) | _BV(CS00));
		TCCR0 |= _BV(WGM01) | _BV(CS01); // CK/32 and Clear Ttimer on Compare match
		
		PORTD |= _BV(PIN5);

		DDRD &= ~_BV(DDD2); // rx pin is input
		DDRD |= _BV(DDD5); // tx pin is output

		MCUCR |=  _BV(ISC01);  // ext int 0 on falling edge
		MCUCR &= ~_BV(ISC00);

		// this mark has to last for a few bits...
		// because if you start to tx without holding tx line "1" for a while, other end can't sync
		GIFR = _BV(INTF0);
		GICR |= _BV(INT0);    // enable external interrupt
		
	} else {
		TCCR2 &= ~(_BV(WGM20) | _BV(CS22) | _BV(CS20));
		TCCR2 |= _BV(WGM21) | _BV(CS21); // CK/32 and Clear Ttimer on Compare match
		
		PORTD |= _BV(PIN4);

		DDRD &= ~_BV(DDD3); // rx pin is input
		DDRD |= _BV(DDD4); // tx pin is output

		MCUCR |=  _BV(ISC11);  // ext int 1 on falling edge
		MCUCR &= ~_BV(ISC10);

		// this mark has to last for a few bits...
		// because if you start to tx without holding tx line "1" for a while, other end can't sync
		GIFR = _BV(INTF1);
		GICR |= _BV(INT1);    // enable external interrupt


	}

	int i;
	for (i=0; i< 4000; i++) ;  // about 6ms @ 7.3mhz

	softStatus[nr] = UARTS_FREE;

	sei();
}


void close_soft(unsigned char nr)
{
	if (nr == UART_GPS) { //GPS
		GIFR = _BV(INTF0);
		TIFR = _BV(OCF0); // clear int flags

		GICR &= ~_BV(INT0);    // enable external interrupt
		TIMSK &= ~_BV(OCIE0);   // disable timer interrupts
	} else {
		GIFR = _BV(INTF1);
		TIFR = _BV(OCF2); // clear int flags

		GICR &= ~_BV(INT1);    // enable external interrupt
		TIMSK &= ~_BV(OCIE2);   // disable timer interrupts
	}
	softStatus[nr] = UARTS_CLOSED;
}

char send_char_soft(unsigned char nr, unsigned char c)
{
	if (softTxBuf[nr]->flagFull == 1) 
		return ERR_BUF_FULL;

	// dont bother to add one char to buf.. just send
	if (softTxBuf[nr]->count > 0 || softStatus[nr] != UARTS_FREE) {
		ringbuf_append(softTxBuf[nr], c);


		// char was put to buffer. uart is sending - nothing to do now.
		if (softStatus[nr] != UARTS_FREE) {
			return ERR_OK; // char was queued
		}
	}

	// dont bother to add one char to buf.. just send
	if (softTxBuf[nr]->count > 0 ) {
		txData[nr] = ringbuf_take(softTxBuf[nr]);
	} else {
		txData[nr] = c;
	}

	uint8_t sr = SREG;
	cli();
	if (softStatus[nr] == UARTS_FREE) {
		txMask[nr] = 1;
		softStatus[nr] = UARTS_TX;

		if (nr == UART_GPS) { //GPS
			PORTD &= ~_BV(PIN5);  // start bit

			TCNT0 = 0; // start sending
			OCR0 = GPS_COUNTER_VALUE10;

			TIFR = _BV(OCF0);
			TIMSK |= _BV(OCIE0);
		} else {
			PORTD &= ~_BV(PIN4);  // start bit

			TCNT2 = 0; // start sending
			OCR2 = DEBUG_COUNTER_VALUE10;

			TIFR = _BV(OCF2);
			TIMSK |= _BV(OCIE2);
		}
	}
	SREG = sr;

	return 0;
}

ISR(TIMER2_COMP_vect)
{

	if (softStatus[UART_DEBUG] == UARTS_RX) {

		if (rxMask[UART_DEBUG] == 0x01) {
			OCR2 = DEBUG_COUNTER_VALUE10;
			rxData[UART_DEBUG] = 0;
		}
			
		if ((PIND & _BV(PIN3)) != 0) rxData[UART_DEBUG] |= rxMask[UART_DEBUG];
		
		if (rxMask[UART_DEBUG] == 0x80) {
			TIFR = _BV(OCF2);    // clear any interrupts
			TIMSK &= ~_BV(OCIE2);

			GIFR = _BV(INTF1);
			GICR |= _BV(INT1);    // enable external interrupt


#ifdef TELNET_VERSION
			// data ready to be put into buf
			if (rxData[UART_DEBUG] == '\n') { // special case
				ringbuf_append(softRxBuf[UART_DEBUG], '\r');
			}
#endif
			ringbuf_append(softRxBuf[UART_DEBUG], rxData[UART_DEBUG]);
			if (rxData[UART_DEBUG] == '\r') { // special case
				ringbuf_append(softRxBuf[UART_DEBUG], '\n');
			}

			softStatus[UART_DEBUG] = UARTS_FREE;

			if (UART.debug_echo) uart_send_char(UART_DEBUG, rxData[UART_DEBUG]);
		} else {
			rxMask[UART_DEBUG] <<= 1;
		}
	
	} else if (softStatus[UART_DEBUG] == UARTS_TX) {
	
		if (txMask[UART_DEBUG] == 0x00) { // send stop bit
			PORTD |= _BV(PIN4);
			softStatus[UART_DEBUG] = UARTS_TX_STOP;
		} else {
			if ((txMask[UART_DEBUG] & txData[UART_DEBUG]) != 0) PORTD |= _BV(PIN4); else PORTD &= ~_BV(PIN4); // send data bit
			txMask[UART_DEBUG] <<= 1; //	if (nothing in buf) ... stop els send more)
		}
	
	} else if (softStatus[UART_DEBUG] == UARTS_TX_STOP) { // Stop bit sent
		softStatus[UART_DEBUG] = UARTS_FREE;


		// send more
		if (softTxBuf[UART_DEBUG]->count > 0) {
			txData[UART_DEBUG] = ringbuf_take(softTxBuf[UART_DEBUG]);
			txMask[UART_DEBUG] = 0x01;

			PORTD &= ~_BV(PIN4); // start bit

			softStatus[UART_DEBUG] = UARTS_TX;
		} else {
			TIFR = _BV(OCF2);    // clear any interrupts
			TIMSK &= ~_BV(OCIE2);
		}
	}

}

ISR(TIMER0_COMP_vect)
{

	if (softStatus[UART_GPS] == UARTS_RX) {

		if (rxMask[UART_GPS] == 0x01) {
			OCR0 = GPS_COUNTER_VALUE10;
			rxData[UART_GPS] = 0;
		}
			
		if ((PIND & _BV(PIN2)) != 0) rxData[UART_GPS] |= rxMask[UART_GPS];
		
		if (rxMask[UART_GPS] == 0x80) {
			TIFR = _BV(OCF0);    // clear any interrupts
			TIMSK &= ~_BV(OCIE0);

			GIFR = _BV(INTF0);
			GICR |= _BV(INT0);    // enable external interrupt

			// data ready to be put into buf
			ringbuf_append(softRxBuf[UART_GPS], rxData[UART_GPS]);

			softStatus[UART_GPS] = UARTS_FREE;
		} else {
			rxMask[UART_GPS] <<= 1;
		}
	
	} else if (softStatus[UART_GPS] == UARTS_TX) {
	
		if (txMask[UART_GPS] == 0x00) { // send stop bit
			PORTD |= _BV(PIN5);
			softStatus[UART_GPS] = UARTS_TX_STOP;
		} else {
			if ((txMask[UART_GPS] & txData[UART_GPS]) != 0) PORTD |= _BV(PIN5); else PORTD &= ~_BV(PIN5); // send data bit
			txMask[UART_GPS] <<= 1; //	if (nothing in buf) ... stop els send more)
		}
	
	} else if (softStatus[UART_GPS] == UARTS_TX_STOP) { // Stop bit sent
		softStatus[UART_GPS] = UARTS_FREE;


		// send more
		if (softTxBuf[UART_GPS]->count > 0) {

			txData[UART_GPS] = ringbuf_take(softTxBuf[UART_GPS]);
			txMask[UART_GPS] = 0x01;

			PORTD &= ~_BV(PIN5); // start bit

			softStatus[UART_GPS] = UARTS_TX;
		} else {
			TIFR = _BV(OCF0);    // clear any interrupts
			TIMSK &= ~_BV(OCIE0);
		}
	}

}

ISR(INT0_vect)
{
	OCR0 = GPS_COUNTER_VALUE15;
	TCNT0 = 0x00;

	TIFR = _BV(OCF0);    // clear any interrupts
	TIMSK |= _BV(OCIE0);

	GIFR = _BV(INTF0);
	GICR &= ~_BV(INT0);    // disable external interrupt

	if (softStatus[UART_GPS] != UARTS_FREE) PORTD |= _BV(PIN5);
	softStatus[UART_GPS] = UARTS_RX;
	rxMask[UART_GPS] = 1;
}
ISR(INT1_vect)
{
	OCR2 = DEBUG_COUNTER_VALUE15;
	TCNT2 = 0x00;

	TIFR = _BV(OCF2);    // clear any interrupts
	TIMSK |= _BV(OCIE2);

	GIFR = _BV(INTF1);
	GICR &= ~_BV(INT1);    // disable  external interrupt

	if (softStatus[UART_DEBUG] != UARTS_FREE) PORTD |= _BV(PIN4);
	softStatus[UART_DEBUG] = UARTS_RX;
	rxMask[UART_DEBUG] = 1;
}

ISR(USART_TXC_vect)
{
	if (hardTxBuf[0]->count > 0) {
		UCSRA |= _BV(TXC);

		UDR = ringbuf_take(hardTxBuf[0]);
		hardStatus = UARTS_TX;
	} else {
		hardStatus = UARTS_FREE;
	}
}

ISR(USART_RXC_vect)
{
	ringbuf_append(hardRxBuf[0], UDR);
}

