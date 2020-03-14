#ifndef FLASH_H
#define FLASH_H

#define DF_SIZE_BYTES	135168lu
#define DF_PAGE_COUNT   512

#define WRITE_FUNC_NOSRCINC  0x01
#define WRITE_FUNC_NOERASE   0x02

#include <inttypes.h>
#include "spi.h"

typedef uint32_t FLASH_P;

void flash_init();
uint8_t flash_status();
uint16_t flash_read(void * dst, FLASH_P address, uint16_t len);
uint16_t flash_write(FLASH_P address, void * src, uint16_t len, uint8_t func);
uint16_t flash_memset(FLASH_P address, uint8_t c, uint16_t len);

void flash_erase_pages(uint16_t start, uint16_t count);
uint16_t flash_pagenum(FLASH_P address);
#endif

