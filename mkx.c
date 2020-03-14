#include <inttypes.h>
#include <string.h>

#include "event.h"
#include "timer.h"
#include "log.h"
#include "param.h"
#include "cmd.h"
#include "mkx.h"
#include "str.h"
#include "gsm.h"
#include "sch.h"
#include "uart.h"

static void mkx_init_connection();
static void mkx_process_packet();
static void mkx_send_ident();
static inline uint8_t mkx_mktp_send(uint8_t op, const void *  data, uint16_t len, uint8_t log);
static void mkx_mktp_send_tr(uint8_t op, uint16_t tr,  const void *  data, uint16_t len, uint8_t log);
static uint8_t mkx_cs(void *data, uint16_t len);
static void mkx_send_log();

static uint16_t tr_id;

static uint8_t init_req;
static uint8_t is_closing;
static uint8_t is_open;
static uint8_t is_free;

static PROC worker1;
static PROC worker2;
static PROC worker3;

void mkx_init()
{
	is_free = 1;
	is_open = 0;
	tr_id = 0;
	is_closing = 0;
	init_req = 0;

	sch_reg(&worker1, PSTR("mkx_init_conn"), 0, &(GSM.tcp_opened), 0, &mkx_init_connection);
	sch_reg(&worker2, PSTR("mkx_proc_pkt"), 0, &(GSM.tcpBytesAvailable), 0, &mkx_process_packet);
	sch_reg(&worker3, PSTR("mkx_send_log"), 2000, &(LOG.available), 1, &mkx_send_log);
}

uint8_t mkx_open() 
{
	if (mkx_is_open()) return GSM_OK;
        //if (gsm_at(pr_P(PSTR("AT#SKTD=0,7733,\"62.65.217.75\"\r\n")), 50, 3) != GSM_ANSWER_CONNECT) return ERROR; //"AT#SKTD=0,10100,\"wall.peaksjah.com\"\r\n"
       //if (gsm_at(pr_P(PSTR("AT#SKTD=0,7008,\"194.126.102.12\"\r\n")), 50, 3) != GSM_ANSWER_CONNECT) return ERROR; //"AT#SKTD=0,10100,\"wall.peaksjah.com\"\r\n"
        //if (gsm_at(pr_P(PSTR("AT#SKTD=0,7733,\"bsd.ee\"\r\n")), 50, 3) != GSM_ANSWER_CONNECT) return ERROR; //"AT#SKTD=0,10100,\"wall.peaksjah.com\"\r\n"

	uint8_t type;
	uint8_t inode;
	uint16_t ptr;
	uint16_t port;
	char addr[50];

	inode = param_get_inode_P(PSTR("host"));
	ptr = param_get(inode, &type);
	param_get_str(addr, ptr, 49);

	inode = param_get_inode_P(PSTR("port"));
	ptr = param_get(inode, &type);
	port = param_get_u16(ptr);

	uint8_t ret = gsm_open_tcp(addr, port);

	if (ret == GSM_OK) init_req = 1; else init_req = 0;// reset request

	return ret;
}

void mkx_close() // app layer
{
	if (is_free) {
		mkx_mktp_send(MKTP_OP_CODE_CONNECTION_CLOSE, 0, 0, 0);
		timer_busy_wait(1000);
		gsm_close_tcp();
		is_open = 0;
	} else {
		is_closing = 1;
	}
}


uint8_t mkx_is_open()
{
	return is_open;//lkj;
}


static void mkx_init_connection()
{
	if (!init_req) return;

	is_free = 1;
	is_open = 0;
	tr_id = 0;
	is_closing = 0;
	init_req = 0;

	mkx_send_ident();
}

// btw we always get "tid:2573 op:78 len:8271 cs:67" when "NO CARRIER" is gotten :)
static void mkx_process_packet()
{
	static MKTP_HEADER head;
	static uint8_t state = 0;

	// header rcv
	if (state == 0) {
		if (UART.gsm_rx->count >= sizeof (MKTP_HEADER)) {

			gsm_read_tcp(&head, sizeof(MKTP_HEADER));

			if (head.op > 8) {
				is_open = 0;
				gsm_close_tcp();
				return;
			}

			if (head.dataLen > 256) head.dataLen = 256;
			state = 1;
		}
	}

#warning mem problem iif datalen > buf
	// data rcv
	if (state == 1) {
		if (UART.gsm_rx->count >= head.dataLen) {
			uint8_t cs;
			uint8_t buf[80];

			gsm_read_tcp(&buf, head.dataLen);

			buf[head.dataLen] = 0;

			cs = mkx_cs((uint8_t*)&head, sizeof(MKTP_HEADER)); 
			cs += mkx_cs(buf, head.dataLen);

			// if cs == 0 then ok

			if (head.op == MKTP_OP_CODE_IDENT_RESPONSE || head.op == MKTP_OP_CODE_TRANSACTION_RESULT) {

#warning some code to retry ...
				if (!strcmp_P((char*)buf, PSTR("OK"))) {

					if(head.op == MKTP_OP_CODE_TRANSACTION_RESULT)
						log_move_next();

					is_free = 1;
					is_open = 1;
				} else { //error
				

					is_free = 1;
				}

				if (is_closing)
					mkx_close();
			}

			if (head.op == MKTP_OP_CODE_START_TRANSACTION) {

				if (cmd_exec(UartNull, (char *)buf) == 0) { // ok
				}

				if (cs == 0) { // ok
					strcpy_P((char*)buf, PSTR("OK\r\n"));
				} else {
					strcpy_P((char*)buf, PSTR("ERROR\r\n"));
				}

				mkx_mktp_send_tr(MKTP_OP_CODE_TRANSACTION_RESULT, head.tID, buf, 4, 0);
			}

			state = 0;
		}
	}
}

static void mkx_send_ident() // app layer
{
	char * ident = pr_P(PSTR("%s,MKAP1.10"), GSM.imei);
	uint16_t len = strlen(ident);

	mkx_mktp_send(MKTP_OP_CODE_IDENT, ident, len, 0);
}

static void mkx_send_log()
{
	if (!log_has_next()) return;

	// if not open, open
	if (!mkx_is_open()) {
		mkx_open();
		return;
	}

	if (!is_free) {
		return;
	}

	static uint8_t badcount = 0;

	EVENT_RECORD record;
	log_read_next(&record);

	if (mkx_mktp_send(MKTP_OP_CODE_START_TRANSACTION, &record, sizeof(EVENT_RECORD), 1) == GSM_OK)  {

		badcount = 0;
	} else {
		badcount++;
	}

	if (badcount > 5) {
		is_free = 1;
		mkx_close();
		badcount = 0;
	}
}

static inline uint8_t mkx_mktp_send(uint8_t op, const void *  data, uint16_t len, uint8_t log)
{
	if (GSM.mode != GSMModeData) {
		mkx_close();
	
		return GSM_ERROR;
	}

	if (is_free) {
		tr_id++;
		mkx_mktp_send_tr(op, tr_id, data, len, log);

	} else {
		return GSM_ERROR;
	}


	return GSM_OK;
}

static void mkx_mktp_send_tr(uint8_t op, uint16_t tr,  const void *  data, uint16_t len, uint8_t log)
{

	MKTP_HEADER header;

	is_free = 0;

	if (tr > tr_id) tr_id = tr;


	header.tID = tr_id;
	header.op = op;
	header.cs = 0;


	if (log) {
		header.dataLen = len + 21;

		char type[11];
		char end[10];

		memcpy_P(type, PSTR("LOG1.10;0\r\n"), 11);
		memcpy_P(end, PSTR("DATA_END\r\n"), 10);

		header.cs += mkx_cs((void*)&header, sizeof(MKTP_HEADER));
		header.cs += mkx_cs(type, 11);
		header.cs += mkx_cs((void*)data, len);
		header.cs += mkx_cs(end, 10);

		header.cs = (~(header.cs)) + 1;

		gsm_send_tcp((void*)&header, sizeof(MKTP_HEADER));
		gsm_send_tcp(type, 11);
		gsm_send_tcp(data, len);
		gsm_send_tcp(end, 10);
	} else {
		header.dataLen = len;

		header.cs += mkx_cs((void*)&header, sizeof(MKTP_HEADER));
		header.cs += mkx_cs((void*)data, len);

		header.cs = (~(header.cs)) + 1;

		gsm_send_tcp((void*)&header, sizeof(MKTP_HEADER));
		gsm_send_tcp(data, len);
	}
}


static uint8_t mkx_cs(void *data, uint16_t len)
{
	uint16_t i;
	uint8_t cs = 0;
	uint8_t *ptr = (uint8_t*)data;

	for(i = 0; i < len; i++)
	{
		cs += *(ptr++);
	}

	return cs;
} 
