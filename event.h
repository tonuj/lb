#ifndef EVENT_H
#define EVENT_H

#include <inttypes.h>

typedef struct __mkap100_message__
{
	uint8_t time[3];
	uint16_t date;
	unsigned GPRMC_OK:1;
	unsigned GPRMC_bearing:7;
	long GPRMC_lat;
	long GPRMC_lon;
	uint16_t GPRMC_speed;
	uint8_t inputs;
	uint8_t outputs;
	uint8_t analog_input;
	uint8_t power_input;
	uint8_t status;
	uint16_t extra;
	uint16_t fuel;
} MKAP100_DATA;

typedef struct __event_record__
{
	uint16_t eventID;
	uint8_t code;
	MKAP100_DATA ds;
	uint16_t event_data_size;
	uint8_t event_data[0];
} EVENT_RECORD;



void event_init();
void event_post();


#endif
