
#include <string.h>

#include "uart.h"
#include "timer.h"
#include "flash.h"
#include "mkx.h"
#include "log.h"

#define VALID(x) (x > 0 && x < 0xff)

static void log_print_between(uint16_t start, uint16_t count);
static void log_find_start_end(uint16_t * start, uint16_t * end);
//implements circular buf
// !! signal new log

LOG_t LOG;

void log_init()
{
	memset(&LOG, 0, sizeof(LOG_t));

	DBGN("Log scan...");
	log_find_start_end(&LOG.start, &LOG.end);
	DBG("ok");
}

void log_format()
{
	flash_erase_pages(0, DF_PAGE_COUNT);
	log_init();
}

uint16_t log_get_event_id()
{
	EVENT_RECORD r;
	uint32_t addr;

	if (LOG.end > 0) {
		addr = LOG.end - 1;
	} else {
		addr = log_slots_max() - 1;
	}

	flash_read(&r, addr * sizeof(EVENT_RECORD), sizeof(EVENT_RECORD));

	return r.eventID;
}

static void log_print_between(uint16_t start, uint16_t count)
{
	EVENT_RECORD r;
	uint16_t i;
	count += start;
	for (i=start; i<count; i++) {
		flash_read(&r, (uint32_t)i * sizeof(EVENT_RECORD), sizeof(EVENT_RECORD));
		DBG("%u: id:%u inp:%u ain:%u", i, r.eventID, r.ds.inputs, r.ds.analog_input);

		timer_busy_wait(30);
	}
}

void log_print()
{
	if (LOG.start == LOG.end) {
		DBG("No unsent log.");
	} else if (LOG.start > LOG.end) {
		log_print_between(LOG.start, log_slots_max() - LOG.start);
		log_print_between(0, LOG.end);
	} else {
		log_print_between(LOG.start, LOG.end - LOG.start);
	}
}

void log_push(EVENT_RECORD * record)
{
	if (log_slots_used() >= log_slots_max()) return;

	flash_write((uint32_t)LOG.end * sizeof(EVENT_RECORD), record, sizeof(EVENT_RECORD), WRITE_FUNC_NOERASE);

	uint16_t current_page;
	uint16_t next_page;

	current_page = flash_pagenum((uint32_t)LOG.end * sizeof(EVENT_RECORD) + sizeof(EVENT_RECORD) -  1); // page num of end of current

	LOG.end++;
	if (LOG.end >= log_slots_max()) LOG.end = 0;

	next_page = flash_pagenum((uint32_t)LOG.end * sizeof(EVENT_RECORD) + sizeof(EVENT_RECORD) - 1);

	if (current_page != next_page) {
		flash_erase_pages(next_page, 1);

		if (log_slots_used() >= (log_slots_max() - 20)) log_init();
	}

	LOG.available = 1;
} 

uint16_t log_slots_max()
{
	return (DF_SIZE_BYTES / sizeof(EVENT_RECORD)) - 5; // 4 extra slots less
}

uint16_t log_slots_used()
{
	if (LOG.start <= LOG.end) {

		return LOG.end - LOG.start;
	} else {
		uint32_t tmp = LOG.end;

		tmp += log_slots_max();

		return (uint16_t)(tmp - (uint32_t)LOG.start);
	}
}

/*! if has next and some was read, ret 1/ else 0 */
uint8_t log_read_next(EVENT_RECORD * record)
{
	if (!log_has_next()) return 0;

	flash_read(record, (uint32_t)LOG.start * sizeof(EVENT_RECORD), sizeof(EVENT_RECORD));

	return 1;
}

/*! if has next, ret 1/ else 0 . flashes current record to 0xff, moves ptr */
uint8_t log_move_next()
{
	if (!log_has_next()) return 0;

	flash_memset((uint32_t)LOG.start * sizeof(EVENT_RECORD) + 2 , 0x00, 1); // clear eventcode to 0x00 

	LOG.start++;
	if (LOG.start >= log_slots_max()) LOG.start = 0;

	return 1;
}

uint8_t log_has_next()
{
	return (LOG.end != LOG.start ? 1 : 0) ;
}

static void log_find_start_end(uint16_t * start, uint16_t * end)
{
	uint16_t slot_count = log_slots_max();
	uint16_t slot = 0;
	uint8_t buf[5]; // we want op code to be read - that is second in struct .. dont read all- to save time
	EVENT_RECORD * p = (EVENT_RECORD *)buf ;

	uint8_t prev = 0;
	uint16_t special = 0;
	*start = 0;
	*end = 0;
	while (slot < slot_count) {
		flash_read(buf, (uint32_t)slot * sizeof(EVENT_RECORD), 4);

		if (!VALID(prev) && VALID(p->code)) {
			*start = slot;
		}
		if (VALID(prev) && !VALID(p->code)) {
			*end = slot;
		}

		if (prev == 0x00 && p->code == 0xff) { // sepcial
			special = slot;
		}

		prev = p->code;

		if ((slot % 800) == 799) { DBGN("."); }

		slot++;
	}

	// no log but last log was written in the middle of the flash before reboot.. so contiue from that on
	if (special > 0 && (*start == 0 && *end == 0)) {
		*start = special;
		*end = special;
	}
}
