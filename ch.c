

#define STATE_OPEN_GPRS 0
#define STATE_SOCKET_DIAL 1
#include "gsm.h"

//static uint8_t state;

//static void answer_handler(char * answer);

void open_ch()
{
//	send_at_P(PSTR("AT+GPRS=1"), &answer_handler);
//	state = STATE_OPEN_GPRS;
}

//send_ch()

//read_ch() 

//static void answer_handler(char * answer)
//{
//	switch (state) {
//		case STATE_OPEN_GPRS:
//			send_at_P(PSTR("AT#SOCKETD"), &get_answer);
//			state = STATE_SOCKET_DIAL;
//		break;
//		case STATE_SOCKET_DIAL:
//
//		break;
//	}
//}
