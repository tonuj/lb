#include <string.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdlib.h>

#include "cmd.h"
#include "timer.h"
#include "gsm.h"
#include "inputs.h"
#include "uart.h"
#include "sch.h"
#include "mkx.h"

#define GSM_PIN_RESET 6
#define GSM_PIN_ONOFF 7

GSM_t GSM;

static void gsm_switch(GsmMode mode);
static void gsm_process_line();
static void gsm_process_byte();
static void gsm_ring_handler();

static GsmAnswer gsm_wait_answer(uint8_t timeout100ms);

static PROC worker1;
static PROC worker2;
static PROC worker3;

static SIGNAL init;

static RINGBUF gsmTxBuf; static char gsmTxData[UART_GSM_TX_BUF_SIZE]; 
static RINGBUF gsmRxBuf; static char gsmRxData[UART_GSM_RX_BUF_SIZE]; 

void gsm_init()
{
	memset(&GSM, 0, sizeof(GSM_t));

	GSM.mode = GSMModeCommand;

	ringbuf_init(gsmRxBuf, gsmRxData, UART_GSM_RX_BUF_SIZE);
	ringbuf_init(gsmTxBuf, gsmTxData, UART_GSM_TX_BUF_SIZE);
	uart_open(UartGSM,  gsmRxBuf, gsmTxBuf);

	/* INIT MODULE */
	DDRD |= _BV(GSM_PIN_RESET) | _BV(GSM_PIN_ONOFF); // gsm on off

        sch_reg(&worker1, PSTR("gsm_proc_ln"), 0,  &(gsmRxBuf->nlReady), 0, &gsm_process_line);
        sch_reg(&worker2, PSTR("gsm_init_module"), 1000, &init, 0, &gsm_init_module);
	sch_reg(&worker3, PSTR("gsm_ring"), 15000, &(INPUTS.chg[DigitalInputRing]), 0, &gsm_ring_handler);// 1 sec min chg time

	init = 0;
}

void gsm_init_module()
{
	PORTD &=  ~_BV(GSM_PIN_RESET);
	timer_busy_wait(100);
	PORTD |=  _BV(GSM_PIN_RESET);
	timer_busy_wait(1000);

	DBGN("GSM init...");

	if (GSM.mode != GSMModeCommand) {
		gsm_switch(GSMModeCommand);
	}

	if (gsm_at(pr_P(PSTR("AT")), 5, 3) != GSM_ANSWER_OK) {
		DBG(" NO response.");
	} else {
		DBG(" ok");
	}

	gsm_at(pr_P(PSTR("ATE0")), 5, 3);
	gsm_at(pr_P(PSTR("AT+CSQ=1")), 5, 3);
	//gsm_at(pr_P(PSTR("AT$GPSPS=2,10\r\n")), 5, 3);
	//gsm_at(pr_P(PSTR("AT#NITZ=1")), 5, 3);

	if (gsm_at(pr_P(PSTR("AT+CGSN")), 5, 3) == GSM_ANSWER_OK) {
		DBG("IMEI: %s", GSM.answer);
		strncpy(GSM.imei, GSM.answer, 18);
	}
}

static void gsm_switch(GsmMode mode)
{
	if (mode == GSMModeData) {
		GSM.tcp_opened = 1;
		GSM.mode = GSMModeData;

		worker1.func = &gsm_process_byte;
		worker1.flag = UART.gsm_byte_waiting;
	} else if (mode == GSMModeCommand) {
		GSM.tcp_closed = 1;
		GSM.mode = GSMModeCommand;

		worker1.func = &gsm_process_line;
		worker1.flag = UART.gsm_line_waiting;

		DBG("gsm: mode_comm");

                if (gsm_at(pr_P(PSTR("AT")), 5, 3) != GSM_ANSWER_OK) {
//			gsm_init_module();
		}
	} else if (mode == GSMModeDataCall) {
		GSM.mode = GSMModeDataCall;

		worker1.func = &gsm_process_line;
		worker1.flag = UART.gsm_line_waiting;

		DBG("gsm: mode_comm");
	}
}

uint8_t gsm_open_tcp(const char * addr, uint16_t port)
{
	uint8_t csq = 0;
	static uint8_t badcount = 0;

	if (GSM.mode != GSMModeCommand) {
		return GSM_ERROR;
	}

	if (badcount++ > 10) {
		badcount = 0;
		gsm_init_module();
	}

	if (gsm_at(pr_P(PSTR("AT+CSQ")), 5, 1) == GSM_ANSWER_OK) {
		uint8_t start = indexof(GSM.answer, ' ');
		uint8_t end = indexof(GSM.answer, ',');

		GSM.answer[end] = 0;

		csq = atoi(&GSM.answer[start]);
		
	} else {
		DBG("GSM error");
		return GSM_ERROR;
	}

	if (csq < 5 || csq == 99) return GSM_ERROR; // bad signal

	badcount = 0; // reset 

	DBG("CSQ: %d", csq);

	if (gsm_at(pr_P(PSTR("AT#GPRS?")), 5, 1) != GSM_ANSWER_OK) return GSM_ERROR;

	DBG("%s", GSM.answer);

	if (GSM.answer[7] == '0') {
		if (gsm_at(pr_P(PSTR("AT#E2ESC=1")), 5, 3) != GSM_ANSWER_OK) return GSM_ERROR;
		if (gsm_at(pr_P(PSTR("AT#SKIPESC=1")), 5, 3) != GSM_ANSWER_OK) return GSM_ERROR; 
		if (gsm_at(pr_P(PSTR("AT#DSTO=1")), 5, 3) != GSM_ANSWER_OK) return GSM_ERROR;    // wait 200ms then send. factory default is 5s !
		if (gsm_at(pr_P(PSTR("AT#USERID=\"gprs\"")), 5, 3) != GSM_ANSWER_OK) return GSM_ERROR;
		if (gsm_at(pr_P(PSTR("AT#PASSW=\"gprs\"")), 5, 3) != GSM_ANSWER_OK) return GSM_ERROR;
		if (gsm_at(pr_P(PSTR("AT+CGDCONT=1,\"IP\",\"internet.emt.ee\"")), 10, 3) != GSM_ANSWER_OK) return GSM_ERROR;

		DBGN("GPRS opening...");
		if (gsm_at(pr_P(PSTR("AT#GPRS=1")), 50, 3) != GSM_ANSWER_OK) {
			DBG("failed");
			return GSM_ERROR;
		}
		DBG("ok");
	}

	DBGN("open tcp: %s %d ... ", addr, port);
	if (gsm_at(pr_P(PSTR("AT#SKTD=0,%d,\"%s\""), port, addr), 100, 1) != GSM_ANSWER_CONNECT) {
		DBG("failed");
		return GSM_ERROR;
	}

	DBG("ok");
	gsm_switch(GSMModeData);

	return GSM_OK;
}

void gsm_close_tcp()
{
	uart_send_P(UartGSM, PSTR("+++"));
	timer_busy_wait(1500);
	uart_send_P(UartGSM, PSTR("ATH\r\n"));

	//timer_busy_wait(100);
	//if (gsm_wait_answer(10) != GSM_ANSWER_NO_CARRIER) gsm_init_module();

	timer_busy_wait(1000);
	uart_send_P(UartGSM, PSTR("AT\r\n"));
	
	gsm_switch(GSMModeCommand);
}

GsmAnswer gsm_at(const char * s, uint8_t timeout100ms, uint8_t tries)
{
	static uint8_t bad = 0;
	uint8_t ans;

	GSM.answer[0] = 0;

	if (GSM.mode != GSMModeCommand) return GSM_ANSWER_NA;

	if (init == 1) return GSM_ANSWER_ERROR;

	do {
		uart_send_rn(UartGSM, s);
		ans = gsm_wait_answer(timeout100ms);

		timer_busy_wait(100);
	} while (ans != GSM_ANSWER_OK && ans != GSM_ANSWER_CONNECT &&
			 --tries > 0);

	if (ans == GSM_ANSWER_TIMEOUT ) {
		bad++;
		if (bad > 10) {
			bad = 0;
			init = 1;
		}
	}

	return ans;
}

static GsmAnswer gsm_wait_answer(uint8_t timeout100ms) // bloackwaits
{
	char ans[31];

	uint64_t send_time = TIMER.millis;

	GSM.answer[0] = 0;
	while (!ELAPSED((timeout100ms * 100), send_time)) {
		if (UART.gsm_rx->nlCount > 0) {

			ringbuf_take_line(UART.gsm_rx, ans, 30);

			// empty line
			if (ans[0] == '\r') continue; 

			if (GSM.answer[0] == 0 && ans[0] > 0x20) {//ans[0] == '+' || ans[0] == '$' || ans[0] == '#') {
				char * src = ans;
				char * dst = GSM.answer;
				uint8_t count = 0;

				while ((*(dst++) = *(src++))) count++; //strcpy

				if (count > 2) *(dst - 3) = 0;
			}
			if (ans[0] == 'O') 
				return GSM_ANSWER_OK;

			if (ans[0] == 'E') return GSM_ANSWER_ERROR;
			if (ans[0] == 'C') return GSM_ANSWER_CONNECT;
			if (ans[0] == 'N') return GSM_ANSWER_NO_CARRIER;
		}

		sleep_enable();
		sleep_cpu();
                sleep_disable();

		//TIMER.soft_wdt = 0;
		wdt_reset();
	}

	return GSM_ANSWER_TIMEOUT;
}

void gsm_process_byte()
{
	GSM.tcpBytesAvailable = 1;
}

static void gsm_process_line()
{
	char ans[30];

	while (UART.gsm_rx->nlCount > 0) {


		ringbuf_take_line(UART.gsm_rx, ans,  30);

		if (!strcmp_P(ans, PSTR("NO CARRIER\r\n"))) {
			gsm_switch(GSMModeCommand);
			DBG("no carr!");
		}
DBG("aa");
		if (!strncmp_P(ans, PSTR("CONNECT "), 8)) {
DBG("bb");
//SWITCH TO RAW? ! 
			gsm_switch(GSMModeDataCall);
DBG("jeeeee");
		}

		if (GSM.mode == GSMModeDataCall) {
			cmd_exec(UartGSM, ans);
		} else {
			uart_send(UartDebug, ans);
		}
	}
}

uint8_t gsm_read_tcp(void * data, uint8_t len)
{
	if (GSM.mode == GSMModeData)
		return ringbuf_take_raw(UART.gsm_rx, data, len);

	return 0;
}

void gsm_send_tcp(const void * data, uint8_t len)
{
	if (GSM.mode == GSMModeData)
		uart_send_raw(UartGSM, data, len);
}

static void gsm_ring_handler()
{
DBG("A");
	if (GSM.mode == GSMModeCommand && inputs_digital_get(DigitalInputRing)) {
		OUT(UartGSM, "ATA");
DBG("x");
	} else if (GSM.mode == GSMModeData) {
		mkx_close();
	}
DBG("B");
}
