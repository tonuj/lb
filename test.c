#include <inttypes.h>

#include "timer.h"
#include "test.h"
#include "outputs.h"
#include "uart.h"

void test_start()
{

}

void test_proc()
{

}

void test_xx()
{
#if 0 
	static uint8_t state = 0;

	Output nr = Output1;

	if (state == 0) {
		// set output, get value
		outputs_set(nr, 0);
		timer_busy_wait(5);

		uart_send_char(UART_DEBUG, 'o');
		uart_send_char(UART_DEBUG, 0);
		}
	else if  (state == 1) {
	// read answer

		outputs_set(nr, 1);
		timer_busy_wait(5);

		uart_send_char(UART_DEBUG, 'o');
		uart_send_char(UART_DEBUG, 0);
	} else {
	// read answer

		outputs_set(nr, 1);

	// if all done, switch to cmd to normal mode
	}
#endif
}
