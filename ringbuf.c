#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ringbuf.h"




#include "uart.h"
#include "timer.h"
static inline void  inc_start(register volatile RINGBUF *buf);

static inline void  inc_start(register volatile RINGBUF *buf)
{
	uint8_t bak = SREG;
	cli();

	buf->start++;
	if (buf->start == buf->size) buf->start = 0;
	buf->count--;

	if (buf->data[buf->start] == '\n') {
		buf->nlCount--;
	}

	SREG = bak;
}

void ringbuf_init(RINGBUF *buf, char * data, uint8_t size)
{
	memset(buf, 0, sizeof(RINGBUF));

	// d
	buf->size = size;
	buf->data = data;
}

// returns overflow -1
inline char ringbuf_append(register volatile RINGBUF *buf, char c)
{
	if (buf->count > (buf->size - 1)) {
		return -1;
	}

	uint8_t bak = SREG;
	cli();


	// overflow - discard deny appending
	if (buf->count == (buf->size - 1)) {
		c = '\n';
	}

	buf->end++;
	if (buf->end >= buf->size) buf->end = 0;
	buf->data[buf->end] = c;

	buf->flagReadyRead = 1;
	buf->count++;
	if (c == '\n') {
		buf->nlCount++;
		buf->nlReady = 1;
	}

	SREG = bak;

	return 0; // FIXME
}

// untakes chars if there are any
inline void ringbuf_unappend(register volatile RINGBUF *buf)
{
	uint8_t bak = SREG;
	cli();

	if (buf->count > 0) {
		uint8_t c = buf->data[buf->end];

		if (c == '\r') buf->nlCount--;
		if (buf->end == 0) buf->end = (buf->size - 1); else buf->end--;

		buf->count--;
	}

	SREG = bak;
}

// returns val. before calling this, check count
inline char ringbuf_take(register volatile RINGBUF *buf)
{
	if (buf->count == 0) return 0;

	inc_start(buf);

	return buf->data[buf->start];
}

uint8_t ringbuf_take_raw(RINGBUF *buf, uint8_t * ptr, uint8_t len)
{
	uint8_t count = 0;
	while (((len--) > 0) && (buf->count > 0)) {
		*(ptr++) = ringbuf_take(buf);
		count++;
	}
	return count;
}

char ringbuf_read(volatile RINGBUF *buf)
{
	if (buf->count == 0) return 0;

        uint8_t tmp = buf->start + 1; 
        if (tmp >= buf->size) tmp = 0;

	return buf->data[tmp];
}

// note. if nlCount is set, copy is interrupted when '\n' is seen.  size must contain 1 byte for str terminator.
uint8_t ringbuf_take_line(RINGBUF * buf, char * dst, uint8_t size)
{
	if (size < 2 || buf->nlCount == 0 ||  buf->count == 0) return 0;

	uint8_t ret = 0;
	*dst = 0; // string end

	size--; // size is size - 0-char
	size--; // size is size - 0-char

	while (buf->count> 0 && size > ret) {

		inc_start(buf);
		*(dst++) = buf->data[buf->start];
		ret++;

		if (buf->data[buf->start] == '\n') {
			break;
		}
	}
	*dst = 0;

	return ret;
}

// check count before skipping
void ringbuf_skip(RINGBUF * buf, uint8_t count) 
{
	uint8_t i;
	for (i=0; i< count && buf->count > 0; i++) {
		inc_start(buf);
	}

}

void ringbuf_skip_line(RINGBUF * buf) 
{
	char f = 1; // flag "no nl yet"
	while ((buf->count > 0) && f) {
		inc_start(buf);
		if (buf->data[buf->start] == '\n') {
			f=0;
		}
	}
}

