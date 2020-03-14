#ifndef SCH_H
#define SCH_H

#include <avr/pgmspace.h>
#include <inttypes.h>

typedef int8_t SIGNAL; // ptr to some var that signals change. scheduler constantly monitors this

struct __proc {
	SIGNAL * flag; // when this is 1 schedule and reset
	uint8_t orred; // are interval_elapsed and flag OR'ed? (otherwise ANDed)
	uint32_t interval; // regular interval
	void (*func)();

//	uint8_t is_and; // if false, INTERVAL || FLAG , if true INTERVAL && FLAG

	uint64_t last_exec_time;
	struct __proc *next;

	PGM_P name; // 8 bytes for name
};

typedef struct __proc PROC;

typedef struct __sch {
	uint8_t dbg; // if true, dbg sent to UART_DEBUG
	uint8_t load; // in percents
	SIGNAL rebooting;
} SCH_t;

extern SCH_t SCH;

// for system
void sch_init();
void sch_run();

// for user
void sch_reboot(uint8_t half_sec);
void sch_reg(PROC *proc, PGM_P name, uint32_t interval, SIGNAL * flag, uint8_t orred, void (*f)());

#endif

