#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "sch.h"
#include "timer.h"

#include "uart.h"

static uint8_t proc_num;
static PROC first;
static PROC * last;

static PROC haltproc;

static int8_t noFlag = 0;

static uint64_t work_time = 0;
static uint64_t lastLoadCalc = 0;
static uint32_t work_time_sum = 0;
static uint32_t work_time_span = 0;

SCH_t SCH;

static void sch_halter();

static uint8_t allow_reg = 0;

void sch_reg(PROC *proc, PGM_P name, uint32_t interval, SIGNAL * flag, uint8_t orred, void (*f)())
{
	if (!allow_reg) {
		DBGN("reg not allowed");
		for (;;);
		return;
	}

	if (flag > 0) {
		proc->flag = flag;
	} else {
		proc->flag = &noFlag;
	}

	proc->orred = orred;
	proc->name = name;
	proc->interval = interval;
	proc->func = f;
	proc->last_exec_time = TIMER.millis;
	proc->next = 0;

	last->next = proc;
	last = proc;
}

void sch_init()
{
	allow_reg = 1;

	last = &first;
	proc_num = 0;
	first.next = 0;

	SCH.load = 0;
	SCH.rebooting = 0;
}

void sch_reboot(uint8_t half_sec)
{
	SCH.rebooting = half_sec;
	sch_reg(&haltproc, PSTR("sch_reboot"), 500, &(SCH.rebooting), 0, &sch_halter);
}

static void sch_halter()
{
	DBGN("%d..", SCH.rebooting);

	if ( SCH.rebooting == 0 ) {
		wdt_enable(WDTO_15MS); 
		for (;;) ;
	}
}

void sch_run()
{
	PROC * p =  &first;
	uint8_t result;

	work_time = timer_millis();

	while (p->next) {
		p = p->next;

		if (p->flag == &noFlag) noFlag = 1;

		if (p->orred) {
			result = (ELAPSED(p->interval, p->last_exec_time) || (*(p->flag) > 0 )) ;
		} else {
			result = (ELAPSED(p->interval, p->last_exec_time) && (*(p->flag) > 0 )) ;
		}

		if (result) {
			p->last_exec_time = TIMER.millis;

			if ((*(p->flag)) > 0) (*(p->flag))--;

			if (SCH.dbg) {
				uint64_t tst = timer_millis();
				DBGN("%p:\t", p->name);

				p->func();

				DBG("%dms", (int16_t)(timer_millis() - tst));
			} else {
				p->func();
			}

			// ---
			if ((SREG & _BV(7)) == 0) {
				/// VERY BAD -- interrupts are disabled somewhere (and not re-enabled)
				sei(); // fix
				DBG("NO INT!!");
			}
		}
	}

	work_time_sum  += timer_millis() - work_time;
	if (ELAPSED(2000, lastLoadCalc)) {
		work_time_span = timer_millis() - lastLoadCalc;

		SCH.load = (int16_t)(((int32_t)work_time_sum * 100)/(int32_t)work_time_span);

		work_time_sum = 0;
		lastLoadCalc = timer_millis();
	}
}

