
#ifndef IO_H
#define IO_H

#include <inttypes.h>

void io_init();
void io_wait_user();

void io_init();
uint8_t io_get(uint8_t nr);
void io_set(uint8_t nr, uint8_t state);


void io_powerup();
void io_powerdown();

#endif
