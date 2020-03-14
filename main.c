#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "log.h"
#include "event.h"
#include "param.h"
#include "spi.h"
#include "flash.h"
#include "timer.h"
#include "main.h"
#include "gps.h"
#include "gsm.h"
#include "ringbuf.h"
#include "inputs.h"
#include "outputs.h"
#include "sch.h"
#include "cmd.h"
#include "mkx.h"

uint32_t check __attribute__ ((section (".noinit"))); // just one random number at the end of bss

/*
 * Mirror of the MCUCSR register, taken early during startup.
 */
uint8_t mcucsr __attribute__((section(".noinit")));

/*
 * Read out and reset MCUCSR early during startup.
 */
void handle_mcucsr(void)
  __attribute__((section(".init3")))
  __attribute__((naked));
void handle_mcucsr(void)
{
  mcucsr = MCUCSR;
  MCUCSR = 0;
}

void main_init()
{
	DBG("\r\n\r\nLittle brother 0.02\r\n");

	if ((mcucsr & 0x1e) != 0) { // crashed !!
		DBGN("Reset: ");

		if (mcucsr & _BV(BORF)) { DBG("brown out"); }
		else if (mcucsr & _BV(EXTRF)) { DBG("ext"); }
		else if (mcucsr & _BV(WDRF)) { DBG("wdt"); }
		else {  DBG("?"); }
	}
}

int main(void)
{
	check = 0x00223344;

	sei();

	/// stage 1
	sch_init();

	/// stage 2
	timer_init();
	spi_init();
	param_init();
	inputs_init();
	outputs_init();

	/// stage 3
	uart_init();
	main_init();

	/// stage 4
	flash_init();
	log_init();
	gsm_init();
	gps_init();
	cmd_init();

	mkx_init();

	set_sleep_mode(SLEEP_MODE_IDLE);
	wdt_enable(WDTO_2S); 

	gsm_init_module();

	event_init();

	DBG("Type 'help[enter]'");
	DBGN(">");

	for (;;) {
		if (check != 0x00223344) {
			DBG("stack ovf!!");
			sch_reboot(10);
		}

	 	sleep_enable();
		while (!(TIMER.interrupt)) {
			sleep_cpu();
		}
		TIMER.interrupt = 0;
		sleep_disable();

		sch_run();

		wdt_reset();
	}

    return 0;
}

