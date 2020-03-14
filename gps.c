#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "gps.h"
#include "ringbuf.h"

#include "uart.h"
#include "str.h"
#include "sch.h"

#define GPRMC_STACK_BUF_SIZE 70

/*!
 *  generate ev on turn - save always previous or pre-previous coord when we turn, generate with old coord
 */

GPS_t GPS;

static void gps_gprmc(char * line);
static void gps_remove_dot(char * ptr);
static PROC worker;

static void gps_process_line();

void gps_init()
{
	memset(&GPS, 0, sizeof(GPS_t));

	GPS.months = 1; // default values. if this was zero then event generator calculates (0-1)*x
	GPS.days = 1;

	sch_reg(&worker, PSTR("gps_proc_ln"), 10, UART.gps_line_waiting, 0, &gps_process_line);
}

/*!
 * Takes line from GPS ringbuf and saves coords, etc locally for later use.
*/
static void gps_process_line()
{
	RINGBUF * buf = UART.gps_rx;
	// read and save nmea to static char[]

	char *ptr;
	uint8_t count;

	char gprmc_line[GPRMC_STACK_BUF_SIZE];

	uint8_t max_times = 10;
	while (UART.gps_rx->nlCount > 0 && (max_times--)) {
		uint8_t max_tries;

		if (ringbuf_take(buf) != '$' ) {
			ringbuf_skip_line(buf);
			continue;
		}

		ringbuf_skip(buf, 4); // skip "GPRM"

		if (ringbuf_take(buf) != 'C') {
			ringbuf_skip_line(buf);
			 continue;// err
		}

		ringbuf_skip(buf, 1); // skip ","

		// read the rest of the line
		count = ringbuf_take_line(buf, gprmc_line, GPRMC_STACK_BUF_SIZE);

		// now we are at first comma
		ptr = gprmc_line;

		// CS for "GPRMC,"
		uint8_t cs = 103;

		while (count-- > 0 && *ptr != '*') {
			cs ^= *(ptr++);
		}

		// counted to zero and dint get '*'
		if (*ptr != '*') continue;

		ptr++;
		// take new line - this was messed up
		if (touint8(ptr) != cs) continue; 

		// was ok
		if (GPS.dbg) {
			uart_send_P(UartDebug, PSTR("$GPRMC,"));
			uart_send(UartDebug, gprmc_line);
		}

		gps_gprmc(gprmc_line);
	}
}

static void gps_remove_dot(char * ptr)
{
	while (*ptr != '.' && *ptr != 0)
		ptr++;

	while (*ptr != 0) {
		 *ptr = *(ptr+1);
		ptr++;
	}
}

/*!
 * $GPRMC,094453.003,A,5917.8129,N,02439.3896,E,1.17,59.11,061007,,,A*59
 */
static void gps_gprmc(char * line)
{
	char *lat, *lon, *speed, *bearing;
	char * ptr = line;

	char tmp[5];
	tmp[2] = 0;

	// hours
	tmp[0] = *(ptr++);
	tmp[1] = *(ptr++);
	GPS.hours = atoi(tmp);

	//minutes
	tmp[0] = *(ptr++);
	tmp[1] = *(ptr++);
	GPS.minutes = atoi(tmp);

	//seconds
	tmp[0] = *(ptr++);
	tmp[1] = *(ptr++);
	GPS.seconds = atoi(tmp);

	//  UTC
	TONEXTCOMMA(ptr);
	ptr++;

	if (*ptr != ',') {
		if (*ptr == 'A') {
			if (GPS.valid == 0) GPS.got_valid = 1; // raising edge

			GPS.valid = 1;
		} else {
			GPS.valid = 0;
		}
		ptr++;
	}
	ptr++;

	lat = ptr;
        TONEXTCOMMA(ptr);
	*ptr = 0;

	ptr++;

	if (*ptr != ',') {
		if (*ptr == 'N') GPS.north = 1; else GPS.north = 0;
		ptr++;
	}

	ptr++;

	lon = ptr;
        TONEXTCOMMA(ptr);
	*ptr = 0;

	ptr++;

	if (*ptr != ',') {
		if (*ptr == 'E') GPS.east = 1; else GPS.east = 0;
		ptr++;
	}

	ptr++;

	speed = ptr;
        TONEXTCOMMA(ptr);
	*ptr = 0;

	ptr++;

	bearing = ptr;
        TONEXTCOMMA(ptr);
	*ptr = 0;

	ptr++;

	tmp[0] = *(ptr++);
	tmp[1] = *(ptr++);
	GPS.days = atoi(tmp);

	tmp[0] = *(ptr++);
	tmp[1] = *(ptr++);
	GPS.months= atoi(tmp);

	tmp[0] = *(ptr++);
	tmp[1] = *(ptr++);
	GPS.years = atoi(tmp);

	gps_remove_dot(lat);
	gps_remove_dot(lon);
	gps_remove_dot(speed);
	gps_remove_dot(bearing);

	GPS.lat = atol(lat);
	GPS.lon = atol(lon);
	GPS.speed = atol(speed);
	GPS.bearing = atol(bearing);

	if (GPS.north == 0) GPS.lat *= -1; //must be west
	if (GPS.east == 0) GPS.lon *= -1; //must be west

	GPS.initialized = 1;
}

