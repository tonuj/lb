#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "eeprom.h"
#include "mkx.h"
#include "param.h"
#include "timer.h"
#include "cmd.h"
#include "sch.h"
#include "gsm.h"
#include "inputs.h"
#include "outputs.h"
#include "gps.h"

#include "mkx.h"

#define CMD_STACK_SIZE 50

#define COMMAND_COUNT 27
#define CMD_ARGS_COUNT 5

static PROC worker1;
static PROC worker2;

CMD_t CMD;

static void cmd_update_fw();
static void cmd_process_line();
static void cmd_process_byte();
static uint8_t cmd_exec_internal(Uart nr, uint8_t argc, char ** argv);

static COMMAND_DICT commands[COMMAND_COUNT];

void cmd_init()
{
	uint8_t i = 0;

	memset(&CMD, 0, sizeof(CMD_t));
	memset(commands, 0, sizeof(COMMAND_DICT) * COMMAND_COUNT);

	commands[i].str = PSTR("");		commands[i++].cmd = CommandNL;

	// peripherals
	commands[i].str = PSTR("output");	commands[i].help = PSTR("<o_nr> <state>"); commands[i++].cmd = CommandOutput;
	commands[i].str = PSTR("input");	commands[i].help = PSTR(" // AIn1 and AIn2 values are 0..1023");	commands[i++].cmd = CommandInputs;
	commands[i].str = PSTR("gps");		commands[i].help = PSTR("<state>"); 		commands[i++].cmd = CommandGPS;
	commands[i].str = PSTR("gsm");		commands[i].help = PSTR("<ATC>");		commands[i++].cmd = CommandGSM;
	commands[i].str = PSTR("gsminit");	commands[i++].cmd = CommandGSMInit;

	// app
	commands[i].str = PSTR("open");		commands[i].help = PSTR(" // connect appserv (set host/port using command: env)");	commands[i++].cmd = CommandAppOpen;
	commands[i].str = PSTR("close");	commands[i++].cmd = CommandAppClose;
	commands[i].str = PSTR("tcpo");		commands[i].help = PSTR("<addr> <port>");	commands[i++].cmd = CommandTcpOpen;
	commands[i].str = PSTR("tcpc");		commands[i++].cmd = CommandTcpClose;

	commands[i].str = PSTR("update");	commands[i++].cmd = CommandUpdate;

	// systemi
	commands[i].str = PSTR("reboot");	commands[i++].cmd = CommandReboot;
	commands[i].str = PSTR("echo");		commands[i].help = PSTR("<state>");		commands[i++].cmd = CommandEcho;
	commands[i].str = PSTR("schdbg");	commands[i].help = PSTR("<state>");		commands[i++].cmd = CommandSchedulerDebug;
	commands[i].str = PSTR("sysload");	commands[i++].cmd = CommandLoad;
	commands[i].str = PSTR("rtc");		commands[i++].cmd = CommandGetRTC;
	commands[i].str = PSTR("setrtc");	commands[i].help = PSTR("<epochsec>");		commands[i++].cmd = CommandSetRTC;
	commands[i].str = PSTR("memdump");	commands[i++].cmd = CommandMemdump;
	commands[i].str = PSTR("uptime");	commands[i++].cmd = CommandUptime;
	commands[i].str = PSTR("setenv");	commands[i].help = PSTR("[[param] newval]");	commands[i++].cmd = CommandSetEnv;

	commands[i].str = PSTR("logstat");	commands[i++].cmd = CommandLogStat;
	commands[i].str = PSTR("logformat");	commands[i++].cmd = CommandLogFormat;
	commands[i].str = PSTR("log");	commands[i++].cmd = CommandLogPrint;
	
#ifdef PRODUCT_W4A
	commands[i].str = PSTR("w4a_add");	commands[i++].cmd = CommandW4AAdd;
	commands[i].str = PSTR("w4a_rm");	commands[i++].cmd = CommandW4ARemove;
	commands[i].str = PSTR("MK+o=3,1");	commands[i++].cmd = CommandO1Hi;
	commands[i].str = PSTR("MK+RESET");	commands[i++].cmd = CommandRemoteReset;
#endif

	commands[i].str = PSTR("help");		commands[i++].cmd = CommandHelp;
	commands[i].str = PSTR("iotest");	commands[i].help = PSTR("// not functional");	commands[i++].cmd = CommandIOTest;

	sch_reg(&worker1, PSTR("cmd_proc_ln"), 10, UART.debug_line_waiting, 0, &cmd_process_line);
	sch_reg(&worker2, PSTR("cmd_update"), 5000, 0, 0, &cmd_update_fw);
#ifdef DEBUG
	UART.debug_echo = 1;
#endif
#ifdef TELNET_VERSION
	UART.debug_echo = 0;
#endif
}

static void cmd_help(Uart nr)
{
	OUT(nr, "Help:");

	uint8_t i;
// no CommandNL
	for (i=1; i<COMMAND_COUNT; i++) {
		if (commands[i].str) {
			OUT(nr, "\t%p %p", commands[i].str, commands[i].help);
			timer_busy_wait(20);
		}
	}

	OUT(nr, "<state> is number 0 or 1. (on/off)");
}

void cmd_switch(CmdMode mode)
{
	if (mode == CmdModeIOTest) {
		worker1.func = &cmd_process_byte;
	} else if (mode == CmdModeNormal) {

		worker1.func = &cmd_process_line;
	}
}

static void cmd_process_byte()
{
//	test_proc();
}

static void cmd_process_line()
{
	char cmdbuf[CMD_STACK_SIZE];
	uint8_t max_times = 10;

	while ((UART.debug_rx->nlCount > 0) && (max_times--)) {
		ringbuf_take_line(UART.debug_rx, cmdbuf, CMD_STACK_SIZE);

		cmd_exec(UartDebug, cmdbuf);
	}
}

uint8_t cmd_exec(Uart nr, char * cmdline)
{
	uint8_t f;
	uint8_t argc;
	char *argv[CMD_ARGS_COUNT];

	argv[0] = cmdline;
	f = 0;
	argc = 0;

	while (*cmdline) {
		if (*cmdline != ' ' && *cmdline != '\r' && *cmdline != '\n')  {
			if (!f) {
				argv[argc] = cmdline;
				argc++;
				f = 1;

				if (argc >= CMD_ARGS_COUNT) break;
			}
		} else {
			f = 0;
			*cmdline = 0;
		}
		cmdline++;
	}

	return cmd_exec_internal(nr, argc, argv);
}

static uint8_t cmd_exec_internal(Uart nr, uint8_t argc, char ** argv)
{
	uint8_t i;
	uint8_t ret = 0;
	Command com = CommandUnknown;

	if (argc == 0) com = CommandNL;

	for (i=0; i<COMMAND_COUNT; i++) {
		if (!strcasecmp_P(argv[0], commands[i].str)) {
			com = commands[i].cmd;
			break;
		}
	}

	switch (com) {
		case CommandNL:

		break;
		case CommandIOTest:
			cmd_switch(CmdModeIOTest);
//			test_start();
		break;
		case CommandSetEnv:
		if (argc > 1)
		{
			uint16_t len;
			uint8_t type;
			uint16_t ptr;
			uint8_t inode;

			inode = param_get_inode(argv[1]);
			DBGN("ino:%d", inode);
			ptr = param_get(inode,  &type);
			DBGN(" addr:%d", ptr);
				ptr += 5; //skip head
			DBGN(" type:%d ",type);

			DBGN( "%s=", argv[1]);

			if (type == 0x00) {
				len = eeprom_read_str(*argv, ptr, 50);
				eeprom_read_str(*argv, ptr + len + 1, 50);
				DBG( "%s", *argv);
			}	
			else if (type == 0x02) {
				len = eeprom_read_str(*argv, ptr, 50);
				eeprom_read(&len, ptr + len + 1, 2);
				DBG( "%d", len);
				
			}
		}

		break;
#ifdef PRODUCT_W4A
		case CommandW4AAdd:
		break;
		case CommandW4ARemove:

//			param_get_id(0, cmdbuf, CMD_STACK_SIZE);
//			DBG("got: %s=%s", cmdbuf, cmdbuf + strlen(cmdbuf) + 1);
//
//			param_get_id(1, cmdbuf, CMD_STACK_SIZE);
//			DBG("got: %s=%s", cmdbuf, cmdbuf + strlen(cmdbuf) + 1);
//
//			param_get_id(2, cmdbuf, CMD_STACK_SIZE);
//			DBG("got: %s=%s", cmdbuf, cmdbuf + strlen(cmdbuf) + 1);
		break;
		case CommandO1Hi:
			outputs_set(Output1, 1);
		break;
		case CommandRemoteReset:
			mkx_close();
			sch_reboot(10);
		break;
#endif
		case CommandUptime:
//			DBG("uptime: %l:%d:%d.%d\r\n", (int32_t)(timer_millis() / 3600000), (int8_t)((timer_millis() / 60000) % 60), (int8_t)((timer_millis() / 1000) % 60), (int16_t)(timer_millis() % 1000)); // this line adds 8k to code :-D
			OUT(nr, "UPTIME: %u",  timer_millis());
		break;
		case CommandOutput:
			if (argc > 2) {
				outputs_set(atoi(argv[1]), atoi(argv[2]));
			}
		break;
		case CommandAppOpen:
			if (mkx_open() != GSM_ERROR) {
				OUT(nr, "connected");
			} else {
				OUT(nr, "err");
			}
		break;
		case CommandAppClose:
			mkx_close();
		break;
		case CommandUpdate:
		{
			if (mkx_is_open()) {
				mkx_close();
			}

			CMD.updatereq = 1;
		}
		break;
		case CommandTcpOpen:
			if (argc > 2) {
				if (gsm_open_tcp(argv[1], atoi(argv[2])) != GSM_ERROR) {
					OUT(nr, "connected");
				} else {
					OUT(nr, "err");
				}
			}
		break;
		case CommandTcpClose:
			if (GSM.mode == GSMModeData) {
				OUT(nr, "closing...");
				gsm_close_tcp();
			}
		break;
		case CommandHelp:
			cmd_help(nr);
		break;
		case CommandSetRTC:
			if (argc > 1) {
				timer_set_epoch(atol(argv[1]));
			}
		break;
		case CommandGetRTC:
			OUT(nr, "RTC: %l", (int32_t)TIMER.rtc); // FIXME   %d annab signed
		break;
		case CommandMemdump:
		{
			uint16_t counter;
			char * mem = (char*)0x60;

			for (counter=0x60;counter<2148;counter++) {
				uart_send_char(nr, *(mem++));

				if ((counter % 128) == 0) 
					timer_busy_wait(200);

				wdt_reset();
			}
		}
		break;
		case CommandLogStat:
			OUT(nr, "%d/%d %d to %d", log_slots_used(), log_slots_max(), LOG.start, LOG.end);
		break;
		case CommandLogPrint:
			log_print();
		break;
		case CommandLogFormat:
			OUTN(nr, "Formatting...");
			log_format();
			OUT(nr, "OK");
		break;
		case CommandGPS:
			if (argv[1][0] == '0') GPS.dbg = 0;
			if (argv[1][0] == '1') GPS.dbg = 1;

		break;
		case CommandSchedulerDebug:
			if (argv[1][0] == '0') SCH.dbg = 0;
			if (argv[1][0] == '1') SCH.dbg = 1;
		break;
		case CommandReboot:
			sch_reboot(3);
			OUT(nr, "Rebooting...");
			worker1.interval = 10000; // next scheduling not before reboot
		break;
		case CommandEcho:
			if (argc > 1) {
				if (argv[1][0] == '0') UART.debug_echo = 0;
				else if (argv[1][0] == '1') UART.debug_echo = 1;
				else cmd_help(nr);
			} else {
				cmd_help(nr);
			}
		break;
		case CommandLoad:
			OUT(nr, "SYSLOAD: %d%%", SCH.load);
		break;
		case CommandInputs:
			inputs_print();
		break;
		case CommandGSMInit:
			gsm_init_module();
		break;
		case CommandGSM:
			if ( GSM.mode != GSMModeCommand) {
				OUT(nr, "Can't! session up.");

			} else if (argc > 1) {

				OUT(nr, "gsm: sent: %s", argv[1]);

				uint8_t ans = gsm_at(argv[1], 15, 1);

				if (ans == GSM_ANSWER_TIMEOUT) OUT(nr, "gsm: timeout");
				if (ans == GSM_ANSWER_ERROR) OUT(nr, "gsm: error");

				if (ans == GSM_ANSWER_OK) {
					if (GSM.answer[0] > 0) {
						OUT(nr, "gsm: ok: %s", GSM.answer);
					} else {
						OUT(nr, "gsm: ok");
					}
				}
	
			} else
				cmd_help(nr);
		break;
		case CommandUnknown:
		default:

			OUT(nr, "sh: %s: command not found", argv[0]);

			ret = 0xff;
	}
//DBG("s:%d e:%d co:%d nlc:%d nlr:%d int:%d ", UART.gps_rx->start, UART.gps_rx->end,  UART.gps_rx->count,  UART.gps_rx->nlCount,  UART.gps_rx->nlReady, (GICR & _BV(INT0)? 1: 0));
	OUTN(nr, ">");

	return ret;
}


static void cmd_update_fw() {
	static TIME last = 0;

	// go if updatereq or if tme passed
	if (((timer_millis() - last) > 86400000) || CMD.updatereq) {

		if (GSM.mode == GSMModeCommand) {

			char str[25];
			strcpy_P(str, PSTR("devel.innotal.ee"));

			if (gsm_open_tcp(str, 6666) != GSM_ERROR) {

				last = timer_millis();
				CMD.updatereq = 0;

				sch_reboot(2); // FIXME this is told by server // so needs to be removed
#warning / FIXME this is told by server // so needs to be removed
			}
		}
	}
}
