#ifndef GPS_H
#define GPS_H

#include <inttypes.h>

#include "ringbuf.h"
#include "sch.h"

typedef struct __gps_data {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;

	uint8_t days;
	uint8_t months;
	uint8_t years;
	
	int32_t lat;
	int32_t lon;

	uint8_t valid:1;
	uint8_t north:1;
	uint8_t east:1;

	uint16_t bearing;
	uint16_t speed;

	uint8_t dbg;

	SIGNAL got_valid;

	uint8_t initialized;
} GPS_t;

extern GPS_t GPS;

void gps_init();

#endif

