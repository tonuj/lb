
#ifndef LOG_H
#define LOG_H

#include <inttypes.h>
#include "sch.h"
#include "event.h"

typedef struct __log {
	SIGNAL available;

	uint16_t start;
	uint16_t end;
} LOG_t;

extern LOG_t LOG;

void log_init();
void log_push(EVENT_RECORD * record);
uint16_t log_slots_max();
uint16_t log_slots_used();
uint8_t log_read_next(EVENT_RECORD * record);
uint8_t log_move_next();
uint8_t log_has_next();
void log_format();
void log_print();
uint16_t log_get_event_id();

#endif
