#include <avr/io.h>

#include "io.h"
#include "display.h"
#include "uart.h"
#include "test.h"


int main(void)
{
	uart_init();
	io_init();

uint32_t i; for (i=0;i<1000000;i++) ;
	io_powerup();

 for (i=0;i<1000000;i++) ;
	display_init();
	for (;;) {
		test_start();
	}
	io_powerdown();

    return 0;
}

