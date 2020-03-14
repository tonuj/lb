#ifndef TIMER_H
#define TIMER_H

#include <inttypes.h>

typedef uint64_t TIME;

typedef struct __timer {
	TIME millis;
	TIME rtc;

	volatile uint8_t soft_wdt;

	volatile uint8_t interrupt;
} TIMER_t;

extern TIMER_t TIMER;

void timer_init();
TIME timer_millis(); // returns millis since boot
void timer_busy_wait(uint16_t millis);

void timer_set_epoch(TIME time);
void timer_update_rtc();

#define ELAPSED(x, y)  ((x+y) <= TIMER.millis)

#endif
