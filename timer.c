#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <string.h>

#include "gps.h"
#include "timer.h"
#include "uart.h"
#include "sch.h"

static PROC worker;


uint16_t timer_days_for_month(uint8_t month, uint8_t year);
void timer_set_rtc(uint8_t years, uint8_t months, uint8_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);


TIMER_t TIMER;

volatile uint8_t timer_flag = 0;
volatile uint8_t mod = 0;

ISR(TIMER1_COMPA_vect)
{
	TIMER.interrupt = 1;
	TIMER.millis += 10;

	if (mod++ == 99) {
		mod = 0;
		TIMER.rtc++;
	}
}

void timer_init()
{
	memset(&TIMER, 0, sizeof(TIMER));

	TIMER.interrupt = 1;

	// set timer
	TCNT1H = 0x00;
	TCNT1L = 0x00;
	//OCR1AH = 0x0B; // 100ms tick
	//OCR1AL = 0x40;

	OCR1AH = 0x01; // 10ms tick
	OCR1AL = 0x20;

	//waveform gen mode 4. see datasheet.
	TCCR1A = 0x00;
	TCCR1B = _BV(WGM12) | _BV(CS12) ;// ck/256

	TIMSK |= _BV(OCIE1A); // enable interrupt

	sch_reg(&worker, PSTR("timer_upd_rtc"), 60000, &(GPS.got_valid), 1, &timer_update_rtc);
}

TIME timer_millis()
{
	return TIMER.millis + TCNT1 / 29;
}

/*!
 * busy wait and NO WDT RESET
 */
void timer_busy_wait(uint16_t millis)
{
	TIME start_time = timer_millis();
	while ((timer_millis() - start_time) < millis) wdt_reset(); 
}

// for february return all days till feb 1. for august return days till aug 1
uint16_t timer_days_for_month(uint8_t month, uint8_t year)
{
	if (month < 2 && month > 12) return 0;

	uint16_t days = 0;

	     if (month == 2) days = 31;
	else if (month == 3) days = 59;
	else if (month == 4) days = 90;
	else if (month == 5) days = 120;
	else if (month == 6) days = 151;
	else if (month == 7) days = 181;
	else if (month == 8) days = 212;
	else if (month == 9) days = 243;
	else if (month == 10) days = 273;
	else if (month == 11) days = 304;
	else if (month == 12) days = 334;

	if ((month >= 3) && ((year % 4) == 0)) days++; // for feb

	return days;
}

void timer_update_rtc()
{
	timer_set_rtc(GPS.years, GPS.months, GPS.days, GPS.hours, GPS.minutes, GPS.seconds);
}

void timer_set_epoch(TIME time)
{
	TIMER.rtc = time;
}

void timer_set_rtc(uint8_t years, uint8_t months, uint8_t days, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	if (years < 7) return;

	TIME time = 0; // 1970 1st jan

	time += (((uint32_t)years + 2000 - 1970) * 365) * 86400; // years
	time += (((uint32_t)years + 2000 - 1970 + 1) / 4)  * 86400; // years that have +1 day

	time += (uint32_t)(timer_days_for_month(months, years)) * 86400;
	time += (uint32_t)(days - 1) * 86400;

	time += (uint32_t)hours * 3600;
	time += (uint32_t)minutes * 60;
	time += (uint32_t)seconds;

	TIMER.rtc = time;
}

