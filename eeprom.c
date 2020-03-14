#include <avr/io.h>

#include "eeprom.h"

unsigned char eeprom_read_byte(uint16_t uiAddress) 
{ 
	while(EECR & (1<<EEWE)) ; 

	EEAR = uiAddress; 
	EECR |= (1<<EERE); 

	return EEDR; 
}

void eeprom_write(unsigned int uiAddress, unsigned char ucData) 
{ 
	/* Wait for completion of previous write */ 
	while(EECR & (1<<EEWE)) 
	; 
	/* Set up address and data registers */ 
	EEAR = uiAddress; 
	EEDR = ucData; 
	/* Write logical one to EEMWE */ 
	EECR |= (1<<EEMWE); 
	/* Start eeprom write by setting EEWE */ 
	EECR |= (1<<EEWE); 
}

uint16_t eeprom_read(void * dst, uint16_t src, uint8_t len)
{
	uint16_t count = 0;

	while (len--) {
		*((uint8_t*)(dst++)) = eeprom_read_byte(src++);
		 count++;
	}

	return count;
}

uint16_t eeprom_read_str(char * dst, uint16_t src, uint8_t len)
{
	uint16_t count = 0;

	while ((*(dst++) = eeprom_read_byte(src++)) && len--) count++;
	*dst = 0;

	return count;
}

uint16_t eeprom_write_str_P(PGM_P src, uint16_t ee_addr)
{
	uint16_t count = 0;
	uint8_t ch;
	while ((ch = pgm_read_byte(src))  > 0) {
		eeprom_write(ee_addr, ch);

		ee_addr++;
		src++;
		count++;
	}

	return count;
}

