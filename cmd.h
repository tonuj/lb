

#ifndef CMD_H
#define CMD_H

#include <avr/pgmspace.h>
#include "uart.h"
#include "sch.h"

typedef struct __cmd {
	uint8_t updatereq;
} CMD_t;

extern CMD_t CMD;

typedef enum __command {
	CommandNL,
	CommandOutput,
	CommandAppOpen,
	CommandAppClose,
	CommandTcpOpen,
	CommandTcpClose,
	CommandUpdate,
	CommandHelp,
	CommandGPS,
	CommandSchedulerDebug,
	CommandReboot,
	CommandEcho,
	CommandLoad,
	CommandInputs,
	CommandGSM,
	CommandGSMInit,

	CommandLogStat,
	CommandLogFormat,
	CommandLogPrint,

#ifdef PRODUCT_W4A
	CommandW4AAdd,
	CommandW4ARemove,
#endif

	CommandMemdump,
	CommandUptime,
	CommandSetEnv,

	CommandGetRTC,
	CommandSetRTC,

	CommandO1Hi,
	CommandRemoteReset,

	CommandIOTest,

	CommandUnknown
} Command;

typedef enum __cmd_mode {
	CmdModeNormal,
	CmdModeIOTest
} CmdMode;

typedef struct __command_dict {
	PGM_P str;
	PGM_P help;
	Command cmd;
} COMMAND_DICT;

void cmd_init();
uint8_t cmd_exec(Uart nr, char * cmdline);
void cmd_switch(CmdMode mode);


#endif

