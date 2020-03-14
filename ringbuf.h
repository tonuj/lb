#ifndef RINGBUF_H
#define RINGBUF_H

#include <inttypes.h>

#include "sch.h"

struct __ringbuf {

	SIGNAL flagFull;
	SIGNAL flagReadyRead;
	SIGNAL nlReady;

	uint8_t start;
	uint8_t end;
	uint8_t count; /*< number bytes unread */
	uint8_t nlCount; // uart may use this to indicate receival of full line, SPI that it has requested n bytes

	char * data;
	uint8_t size;
};

typedef struct __ringbuf RINGBUF;


void ringbuf_init(RINGBUF *buf, char * data, uint8_t size);

inline char ringbuf_append(register volatile RINGBUF *buf, char c);
inline void ringbuf_unappend(register volatile RINGBUF *buf);
inline char ringbuf_take(register volatile RINGBUF *buf);

char ringbuf_read(volatile RINGBUF *buf);
uint8_t ringbuf_take_line(RINGBUF * buf, char * dst, uint8_t size);
void ringbuf_skip(RINGBUF * buf, uint8_t count); 
void ringbuf_skip_line(RINGBUF * buf);
uint8_t ringbuf_take_raw(RINGBUF *buf, uint8_t * ptr, uint8_t len);


#endif

