#ifndef PARAM_H
#define PARAM_H

#include <avr/pgmspace.h>

void param_init();

uint8_t param_get_inode_P(PGM_P name);
uint8_t param_get_inode(const char * name);

uint16_t param_get(uint8_t inode, uint8_t * type);
uint8_t param_get_str(char * dst, uint16_t src, uint8_t len);
uint16_t param_get_u16(uint16_t src);

#endif
