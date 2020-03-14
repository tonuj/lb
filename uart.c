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

// buffer sizes
#define UART_GSM_TX_BUF_SIZE 150    // TELIT MODULE
#define UART_GSM_RX_BUF_SIZE 100

#define UART_GPS_TX_BUF_SIZE 4
#define UART_GPS_RX_BUF_SIZE 100

#define UART_RFID_RX_BUF_SIZE 70    // RFID also
#define UART_RFID_TX_BUF_SIZE 100   // GPS out

#define UART_DEBUG_RX_BUF_SIZE 70    // TIGER and also debug
#define UART_DEBUG_TX_BUF_SIZE 100   //


//  baudrates
#define TIGER_BAUD       115200lu
#define GSM_BAUD       115200lu

#define GPS_COUNTER_VALUE10  188     // Softuart @ 7.3728 4800bps
#define GPS_COUNTER_VALUE15 250
#define DEBUG_COUNTER_VALUE10  94 // Softuart @ 7.3728 9600bps
#define DEBUG_COUNTER_VALUE15 125

#define UARTS_FREE      0 // ready to send
#define UARTS_TX        1 // tx'ng
#define UARTS_TX_STOP   2 // this is when tx just has se(n)t stop bit
#define UARTS_RX        3 // rx'ng
#define UARTS_CLOSED    4 // must open first

UART_t UART;

static volatile unsigned char status[4] = { UARTS_FREE; UARTS_FREE, UARTS_FREE, UARTS_FREE };

static volatile unsigned char rxMask[2];
static volatile unsigned char txMask[2];
static volatile unsigned char rxData[2];
static volatile unsigned char txData[2];

static volatile RINGBUF *hardRxBuf[2];
static volatile RINGBUF *hardTxBuf[2];
static volatile RINGBUF *softRxBuf[2];
static volatile RINGBUF *softTxBuf[2];

static RINGBUF devNull;
static char devNullData[5];


static RINGBUF gpsTxBuf; static char gpsTxData[UART_GPS_TX_BUF_SIZE];
static RINGBUF gpsRxBuf; static char gpsRxData[UART_GPS_RX_BUF_SIZE];

static RINGBUF gsmTxBuf; static char gsmTxData[UART_GSM_TX_BUF_SIZE]; 
static RINGBUF gsmRxBuf; static char gsmRxData[UART_GSM_RX_BUF_SIZE]; 

static RINGBUF debugRxBuf; static char debugRxData[UART_DEBUG_RX_BUF_SIZE];
static RINGBUF debugTxBuf; static char debugTxData[UART_DEBUG_TX_BUF_SIZE];

static RINGBUF rfidRxBuf; static char debugRxData[UART_DEBUG_RX_BUF_SIZE];
static RINGBUF Com2TxBuf; static char debugTxData[UART_DEBUG_TX_BUF_SIZE];



void uart_init()
{
	ringbuf_init(gpsTxBuf, gpsTxData, UART_GPS_TX_BUF_SIZE);
	ringbuf_init(gpsRxBuf, gpsRxData, UART_GPS_RX_BUF_SIZE);

	ringbuf_init(debugRxBuf, debugRxData, UART_DEBUG_RX_BUF_SIZE);
	ringbuf_init(debugTxBuf, debugTxData, UART_DEBUG_TX_BUF_SIZE);


	uart_open(UartGPS,  gpsRxBuf, gpsTxBuf);
	uart_open(UartDebug, debugRxBuf, debugTxBuf);
	uart_open(UartDebug, debugRxBuf, debugTxBuf);

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
	if (nr == UartNull) return;

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

		open_hard(uint8_t rate);
	}
}

void uart_close(unsigned char nr)
{
	if (nr == UartNull) return;

	if (nr < 2)
		close_soft(nr);
	else
		close_hard(nr);
}

void uart_send_raw(unsigned char nr, const void * data, uint16_t len)
{
	uint8_t c;
	uint16_t i;
	const uint8_t * ptr = (const uint8_t *) data;

	if (nr == UartNull) return;

	for (i=0; i<len; i++) {
		c  = *(ptr++);

		if (nr < 2) {
			send_char_soft(nr, c);
		} else {
			send_char_hard(nr, c);
		}
	}
}

void uart_send_rn(unsigned char nr, const char * str)
{
	if (nr == UartNull) return;

	uart_send(nr, str);
	uart_send_P(nr, PSTR("\r\n"));
}

void uart_send(unsigned char nr, const char * str)
{
	char c;

	if (nr == UartNull) return;

	for (;;) {
		c  = *(str++);
		if (c==0) break;

		if (nr < 2) {
			send_char_soft(nr, c);
		} else {
			send_char_hard(nr, c);
		}
	}
}

void uart_send_P(unsigned char nr, PGM_P str)
{
	char c;
	if (nr == UartNull) return;

	for (;;) {
		c  = pgm_read_byte(str++);
		if (c==0) break;

		if (nr < 2) {
			send_char_soft(nr, c);
		} else {
			send_char_hard(nr, c);
		}
	}
}

char uart_send_char(unsigned char nr, unsigned char c)
{
	if (nr == UartNull) return 0;
	if (nr < 2) {
#ifdef TELNET_VERSION
		if (nr == UartDebug && c == '\r') return 0;
#endif

		return send_char_soft(nr, c);
	} else {
		return send_char_hard(nr, c);
	}
}

void uart_open_hard(uint8_t nr, uint32_t rate)
{
	uint32_t baud = ((F_CPU)/(16lu * rate)) - 1;

	//baud = F_CPU / (16 * baud) - 1;

	if (nr == 0) {
		UBRR0H = (unsigned char)(baud>>8); 
		UBRR0L = (unsigned char)baud; 

		UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(TXCIE0) | _BV(RXCIE0);
		UCSR0C = _BV(URSEL0) | _BV(UCSZ00) | _BV(UCSZ01); 
	} else if (nr == 1)
		UBRR1H = (unsigned char)(baud>>8); 
		UBRR1L = (unsigned char)baud; 

		UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(TXCIE1) | _BV(RXCIE1);
		UCSR1C = _BV(URSEL1) | _BV(UCSZ10) | _BV(UCSZ11); 
	}
}

void close_hard(nr)
{
	if (nr == 0)
		UCSR0B = 0;
	else
		UCSR1B = 0;
}

char send_char_hard(uint8_t nr, unsigned char c)
{
	if (hardTxBuf[nr]->flagFull == 1) 
		return ERR_BUF_FULL;


	if (hardTxBuf[nr]->count > 0 || hardStatus != UARTS_FREE) {
		ringbuf_append(hardTxBuf[nr], c);
		return ERR_OK;
	}

	if (hardTxBuf[nr]->count == 0 && hardStatus == UARTS_FREE) {
		UDR = c;
	} else if (hardTxBuf[nr]->count > 0 && hardStatus == UARTS_FREE) {
		UDR = ringbuf_take(hardTxBuf[nr]);
	}
//		UDR = ringbuf_take(hardTxBuf[0]);

	hardStatus = UARTS_TX;

	return ERR_OK;
}

void open_soft(unsigned char nr)
{
	if (nr == UartNull) return;

	cli();

	if (nr == UartGPS) { // GPS
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
		EIMSK |= _BV(INT0);    // enable external interrupt
		
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
		EIMSK |= _BV(INT1);    // enable external interrupt


	}

	int i;
	for (i=0; i< 4000; i++) ;  // about 6ms @ 7.3mhz

	softStatus[nr] = UARTS_FREE;

	sei();
}


void close_soft(unsigned char nr)
{
	if (nr == UartNull) return;

	if (nr == UartGPS) { //GPS
		GIFR = _BV(INTF0);
		TIFR = _BV(OCF0); // clear int flags

		EIMSK &= ~_BV(INT0);    // enable external interrupt
		TIMSK &= ~_BV(OCIE0);   // disable timer interrupts
	} else {
		GIFR = _BV(INTF1);
		TIFR = _BV(OCF2); // clear int flags

		EIMSK &= ~_BV(INT1);    // enable external interrupt
		TIMSK &= ~_BV(OCIE2);   // disable timer interrupts
	}
	softStatus[nr] = UARTS_CLOSED;
}

static char send_char_soft(unsigned char nr, unsigned char c)
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

		if (nr == UartGPS) { //GPS
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

	if (softStatus[UartDebug] == UARTS_RX) {

		if (rxMask[UartDebug] == 0x01) {
			OCR2 = DEBUG_COUNTER_VALUE10;
			rxData[UartDebug] = 0;
		}
			
		if ((PIND & _BV(PIN3)) != 0) rxData[UartDebug] |= rxMask[UartDebug];
		
		if (rxMask[UartDebug] == 0x80) {
			TIFR = _BV(OCF2);    // clear any interrupts
			TIMSK &= ~_BV(OCIE2);

			GIFR = _BV(INTF1);
			EIMSK |= _BV(INT1);    // enable external interrupt


#ifdef TELNET_VERSION
			// data ready to be put into buf
			if (rxData[UartDebug] == '\n') { // special case
				ringbuf_append(softRxBuf[UartDebug], '\r');
			}
#endif
			softStatus[UartDebug] = UARTS_FREE;
			if (rxData[UartDebug] != 0x08) {// backspace
				ringbuf_append(softRxBuf[UartDebug], rxData[UartDebug]);

				if (UART.debug_echo) {
					uart_send_char(UartDebug, rxData[UartDebug]);
				}
			} else {
				if (UART.debug_echo) {
					if (softRxBuf[UartDebug]->count > 0) { 
						uart_send_char(UartDebug, 0x08);
						uart_send_char(UartDebug, ' ');
						uart_send_char(UartDebug, 0x08);
					} else {
//						uart_send_char(UART_DEBUG, rxData[UART_DEBUG]);
					}
				}
				ringbuf_unappend(softRxBuf[UartDebug]);
			}

			if (rxData[UartDebug] == '\r') { // special case
				ringbuf_append(softRxBuf[UartDebug], '\n');
			}
		} else {
			rxMask[UartDebug] <<= 1;
		}
	
	} else if (softStatus[UartDebug] == UARTS_TX) {
	
		if (txMask[UartDebug] == 0x00) { // send stop bit
			PORTD |= _BV(PIN4);
			softStatus[UartDebug] = UARTS_TX_STOP;
		} else {
			if ((txMask[UartDebug] & txData[UartDebug]) != 0) PORTD |= _BV(PIN4); else PORTD &= ~_BV(PIN4); // send data bit
			txMask[UartDebug] <<= 1; //	if (nothing in buf) ... stop els send more)
		}
	
	} else if (softStatus[UartDebug] == UARTS_TX_STOP) { // Stop bit sent
		softStatus[UartDebug] = UARTS_FREE;


		// send more
		if (softTxBuf[UartDebug]->count > 0) {
			txData[UartDebug] = ringbuf_take(softTxBuf[UartDebug]);
			txMask[UartDebug] = 0x01;

			PORTD &= ~_BV(PIN4); // start bit

			softStatus[UartDebug] = UARTS_TX;
		} else {
			TIFR = _BV(OCF2);    // clear any interrupts
			TIMSK &= ~_BV(OCIE2);
		}
	}

}

ISR(TIMER0_COMP_vect)
{

	if (softStatus[UartGPS] == UARTS_RX) {

		if (rxMask[UartGPS] == 0x01) {
			OCR0 = GPS_COUNTER_VALUE10;
			rxData[UartGPS] = 0;
		}
			
		if ((PIND & _BV(PIN2)) != 0) rxData[UartGPS] |= rxMask[UartGPS];
		
		if (rxMask[UartGPS] == 0x80) {
			TIFR = _BV(OCF0);    // clear any interrupts
			TIMSK &= ~_BV(OCIE0);

			GIFR = _BV(INTF0);
			EIMSK |= _BV(INT0);    // enable external interrupt

			// data ready to be put into buf
			ringbuf_append(softRxBuf[UartGPS], rxData[UartGPS]);

			softStatus[UartGPS] = UARTS_FREE;
		} else {
			rxMask[UartGPS] <<= 1;
		}
	
	} else if (softStatus[UartGPS] == UARTS_TX) {
	
		if (txMask[UartGPS] == 0x00) { // send stop bit
			PORTD |= _BV(PIN5);
			softStatus[UartGPS] = UARTS_TX_STOP;
		} else {
			if ((txMask[UartGPS] & txData[UartGPS]) != 0) PORTD |= _BV(PIN5); else PORTD &= ~_BV(PIN5); // send data bit
			txMask[UartGPS] <<= 1; //	if (nothing in buf) ... stop els send more)
		}
	
	} else if (softStatus[UartGPS] == UARTS_TX_STOP) { // Stop bit sent
		softStatus[UartGPS] = UARTS_FREE;


		// send more
		if (softTxBuf[UartGPS]->count > 0) {

			txData[UartGPS] = ringbuf_take(softTxBuf[UartGPS]);
			txMask[UartGPS] = 0x01;

			PORTD &= ~_BV(PIN5); // start bit

			softStatus[UartGPS] = UARTS_TX;
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
	EIMSK &= ~_BV(INT0);    // disable external interrupt

	if (softStatus[UartGPS] != UARTS_FREE) PORTD |= _BV(PIN5);
	softStatus[UartGPS] = UARTS_RX;
	rxMask[UartGPS] = 1;
}
ISR(INT1_vect)
{
	OCR2 = DEBUG_COUNTER_VALUE15;
	TCNT2 = 0x00;

	TIFR = _BV(OCF2);    // clear any interrupts
	TIMSK |= _BV(OCIE2);

	GIFR = _BV(INTF1);
	EIMSK &= ~_BV(INT1);    // disable  external interrupt

	if (softStatus[UartDebug] != UARTS_FREE) PORTD |= _BV(PIN4);
	softStatus[UartDebug] = UARTS_RX;
	rxMask[UartDebug] = 1;
}

ISR(USART_TXC_vect)
{
	if (hardTxBuf[nr]->count > 0) {
		UCSRA |= _BV(TXC);

		UDR = ringbuf_take(hardTxBuf[nr]);
		hardStatus = UARTS_TX;
	} else {
		hardStatus = UARTS_FREE;
	}
}

ISR(USART_RXC_vect)
{
	ringbuf_append(hardRxBuf[nr], UDR);
}

