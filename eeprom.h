#ifndef EEPROM_H
#define EEPROM_H

#include <inttypes.h>
#include <avr/pgmspace.h>

unsigned char eeprom_read_byte(uint16_t uiAddress) ;
void eeprom_write(unsigned int uiAddress, unsigned char ucData);
uint16_t eeprom_read_str(char * dst, uint16_t src, uint8_t len);
uint16_t eeprom_write_str_P(PGM_P src, uint16_t ee_addr);
uint16_t eeprom_read(void * dst, uint16_t src, uint8_t len);


#endif
