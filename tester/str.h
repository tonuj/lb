
#ifndef STR_H
#define STR_H

#include <avr/pgmspace.h>

#define TONEXTCOMMA(ptr) while (*ptr != ',' && *ptr != 0) ptr++;


char * pr_P(PGM_P fmt, ...);
uint8_t touint8(char *p);
uint8_t indexof( const char * string,  char ch);
void wait(uint32_t i);

#endif
