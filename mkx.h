#ifndef MKX_H
#define MKX_H

#include "event.h"

#define EVENT_CODE_TRACE                        0x10 // takes up 3 codes for trace 1, 2 and 3

#define MKTP_OP_CODE_IDENT                      0x02
#define MKTP_OP_CODE_IDENT_RESPONSE             0x03
#define MKTP_OP_CODE_CONNECTION_CLOSE           0x04
#define MKTP_OP_CODE_START_TRANSACTION          0x06
#define MKTP_OP_CODE_TRANSACTION_RESULT         0x07

typedef struct __mktp_header
{
	uint16_t tID;
	uint8_t op;
	uint16_t dataLen;
	uint8_t cs;
} MKTP_HEADER;

/*
typedef struct __first_record
{
	char type[11];
	EVENT_RECORD rec;
	char end[10]; // DATA_END
} FIRST_RECORD;
*/

typedef union __mk3_time {
	uint32_t time;
	char bytes[4];
} MK3_TIME;

void mkx_init();
uint8_t mkx_open();
uint8_t mkx_is_open();
void mkx_close(); 

#endif

