#ifndef GSM_H
#define GSM_H

#include <inttypes.h>
#include <avr/pgmspace.h>

#include "ringbuf.h"

#define GSM_OK    0
#define GSM_ERROR 1

typedef enum __gsm_mode {
	GSMModeCommand,
	GSMModeDataCall,
	GSMModeData // gprs
} GsmMode;

typedef struct __gsm_flags {
	GsmMode mode; 

	SIGNAL tcp_opened;
	SIGNAL tcp_closed;
	SIGNAL tcpBytesAvailable;

	char answer[30];

	char imei[18];
} GSM_t;

typedef enum __gsm_answer {
	GSM_ANSWER_OK,
	GSM_ANSWER_ERROR,
	GSM_ANSWER_TIMEOUT,
	GSM_ANSWER_CONNECT,
	GSM_ANSWER_NO_CARRIER,
	GSM_ANSWER_NA // if module in data mode
} GsmAnswer;

extern GSM_t GSM;

void gsm_init();

void gsm_init_module();
uint8_t gsm_open_tcp(const char * addr, uint16_t port);
void gsm_close_tcp();

void gsm_send_tcp(const void * data, uint8_t len);
uint8_t gsm_read_tcp(void * data, uint8_t len);

GsmAnswer gsm_at(const char * s, uint8_t timeout100ms, uint8_t tries);


#endif

