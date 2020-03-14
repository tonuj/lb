#include <avr/io.h>
#include <string.h>

#include "timer.h"
#include "gsm.h"
#include "gps.h"
#include "sch.h"
#include "outputs.h"

static void outputs_leds();

static PROC worker;

void outputs_init()
{
	DDRA |= _BV(PIN2) | _BV(PIN3) | _BV(PIN4) | _BV(PIN5); // outputs
	PORTA &= ~(_BV(PIN2) | _BV(PIN3) | _BV(PIN4) | _BV(PIN5));// set low

	//init
	uint8_t i;
	for (i=0;i<10;i++) {
		PORTA ^= _BV(PIN4) | _BV(PIN5);
		timer_busy_wait(50);
	}
	sch_reg(&worker, PSTR("outp_leds"), 500, 0, 0, &outputs_leds);
}

static void outputs_leds()
{
	static uint8_t slow_led = 0;
	static uint8_t t1 = 0;
	static uint8_t t2 = 0;

	if (((GSM.mode == GSMModeCommand) && ((slow_led % 4) == 0)) ||
		(GSM.mode == GSMModeData)) {
		outputs_set(OutputLED1, (t1 % 2));
		t1++;
	}

	if (GSM.mode == GSMModeData) t2 = t1; // blink in opposite phases

	if (GPS.valid) {
		outputs_set(OutputLED2, (t2 % 2));
		t2++;
	} else {
		outputs_set(OutputLED2, 0);
	}

	slow_led++;
}

void outputs_set(Output nr, uint8_t val)
{
	if (nr == Output1) nr = PIN2;
	else if (nr == Output2) nr = PIN3;

	else if (nr == OutputLED1) nr = PIN5;
	else if (nr == OutputLED2) nr = PIN4;

	if (val) PORTA |= _BV(nr); else PORTA &= ~_BV(nr);
}

uint8_t outputs_get(Output nr)
{
	if (nr == Output1) nr = PIN2;
	else if (nr == Output2) nr = PIN3;

	else if (nr == OutputLED1) nr = PIN5;
	else if (nr == OutputLED2) nr = PIN4;

	return (PINA & _BV(nr) ? 1 : 0);
}

