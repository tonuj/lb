#include <avr/io.h>
#include "display.h"

// Samsung S6A0070
// Novatek NT7603

/*
DB0 DB1 DB2 DB3 - I/O MPU Lower 4 tri-state bi-directional data bus for transmitting data between MPU and NT7603. Not used during 4-bit operation. 
DB4 DB5 DB6 DB7 - I/O MPU Higher 4 tri-state bi-directional data bus for transmitting data between MPU and NT7603. DB7 is also used as busy flag. 

PORTB:
DB0 - PIN0
DB1 - PIN1
... etc
*/
#define DISPLAY_DATA_PORT      PORTB
#define DISPLAY_DATA_DDR       DDRB

#define DISPLAY_CONTROL_PORT   PORTA
#define DISPLAY_CONTROL_DDR    DDRA
#define DISPLAY_CONTROL_RS_PIN PIN2 // 0: Instruction register (write), Busy flag, address counter (read), 1: Data register (write, read) 
#define DISPLAY_CONTROL_RW_PIN PIN1 // 0: Write      1: Read 
#define DISPLAY_CONTROL_EN_PIN PIN0 // Read/Write start signal 

void display_exec(uint8_t data, uint8_t rs, uint8_t rw) {
	uint32_t i;


	if (rs) DISPLAY_CONTROL_PORT |= _BV(DISPLAY_CONTROL_RS_PIN); else DISPLAY_CONTROL_PORT &= ~_BV(DISPLAY_CONTROL_RS_PIN);
	if (rw) DISPLAY_CONTROL_PORT |= _BV(DISPLAY_CONTROL_RW_PIN); else DISPLAY_CONTROL_PORT &= ~_BV(DISPLAY_CONTROL_RW_PIN);

	DISPLAY_CONTROL_PORT |= _BV(DISPLAY_CONTROL_EN_PIN);
	DISPLAY_DATA_PORT = data;
	for (i=0;i<1000;i++) ;
	DISPLAY_CONTROL_PORT &= ~_BV(DISPLAY_CONTROL_EN_PIN);
	for (i=0;i<1000;i++) ;
}

void display_init()
{
	// outputs
	DISPLAY_CONTROL_DDR |= _BV(DISPLAY_CONTROL_RS_PIN);
	DISPLAY_CONTROL_DDR |= _BV(DISPLAY_CONTROL_RW_PIN);
	DISPLAY_CONTROL_DDR |= _BV(DISPLAY_CONTROL_EN_PIN);
	DISPLAY_DATA_DDR |= 0xff; // all outputs
	DISPLAY_DATA_PORT = 0;

	DISPLAY_CONTROL_PORT &= ~_BV(DISPLAY_CONTROL_RS_PIN);
	DISPLAY_CONTROL_PORT &= ~_BV(DISPLAY_CONTROL_RW_PIN);
	DISPLAY_CONTROL_PORT &= ~_BV(DISPLAY_CONTROL_EN_PIN);

	display_exec(0x10 | 0x08 | 0x20, 0, 0);
	display_exec(0x08 | 0x04 | 0x02, 0, 0);
	display_exec(0x01, 0, 0); // clear
	display_exec(0x04 | 0x02, 0, 0); // Disp on, cursor
while (1) {
	display_exec(0x02, 0, 0);  // go home
	display_exec(0x80 + 5, 0, 0);
	display_exec('A', 1, 0);
}
}

void display_send(const char * str)
{

}
