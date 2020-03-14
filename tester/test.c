#include "uart.h"
#include "io.h"
#include "str.h"
#include "display.h"

#define SEND(fmt, args... ) { uart_send(pr_P(PSTR(fmt) , ## args )); } 
#define DISPLAY(fmt, args... ) { display_send(pr_P(PSTR(fmt) , ## args )); } 

// display text must  fit
#define RCV_BUF_SIZE  50 

char buf[RCV_BUF_SIZE];
/*!
 * proto. no spaces/nl's
 * d text	display
 * o nr         get inp
 * i nr state   set output
 */
void test_start()
{
	uint8_t c, c2;

	c = uart_receive_char(); 

	switch (c) {
	case 'o': // for testing outputs
		c = uart_receive_char(); 

		uart_send_char(io_get(c));
	break;
	case 'i': // for testing outputs
		c = uart_receive_char(); 
		c2 = uart_receive_char(); 

		io_set(c,c2);
	break;
	case 'd':
		// now receive text until 0
		uart_receive_str(buf, RCV_BUF_SIZE);
		display_send(buf);
	}

	uart_send_char(0); // end of transfer
}

