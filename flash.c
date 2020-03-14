#include "timer.h"
#include "uart.h"
#include "flash.h"
#include "spi.h"


#define DFC_B1_WRITE		0x84
#define DFC_PAGE_ERASE		0x81
#define DFC_MEM_WRITE_B1	0x83
#define DFC_MEM_WRITE_NOERASE_B1    0x88
#define DFC_CA_READ			0xe8
#define DFC_STATUS_READ		0xd7
#define DFC_MEM_PAGE_TO_B1	0x53

static void flash_wait_ready();

union flash_addr {
	uint32_t a32;
	struct { 
		uint8_t third;
		uint8_t second;
		uint8_t first;
	}portion ;
} addr;


void flash_init()
{
	if (flash_status() == 0x8c) {
		DBG("1Mbit DataFlash found.");
	} else {
		DBG("Unknown flash status.");
	}
} 

uint8_t flash_status()
{
	uint8_t status;

	spi_ss_lo();
	spi_tx(DFC_STATUS_READ);
	status = spi_rx();
	spi_ss_hi();

	return status;
}

static void flash_wait_ready()
{
	uint8_t count = 0;
	uint16_t delay;

	while (flash_status() != 0x8c && count < 128) {
		count++;
#ifdef BOOTLOADER
		for (delay=0; delay<500; delay++) ;
#else
		timer_busy_wait(1);
#endif
        }

	if (count >= 128) {
		DBG("flash not ready");
	}
}

uint16_t flash_memset(FLASH_P address, uint8_t c, uint16_t len)
{
	uint8_t buf[1];

	buf[0] = c;
	return flash_write(address, buf, len, WRITE_FUNC_NOSRCINC | WRITE_FUNC_NOERASE);
}

uint16_t flash_write(FLASH_P address, void * src, uint16_t len, uint8_t func)
{
        if (address + len >= DF_SIZE_BYTES) {
                return 0;
        }

	uint16_t byte;
	uint16_t page;
	char next_page = 0;

	byte = (address % 264) & 0x000001ff;
	page = (address / 264) & 0x000001ff;

	while (len > 0) {

		flash_wait_ready();

		if (next_page) page++;

		// read flash to flash buf
		addr.a32 = (((uint32_t)page << 9));

		spi_ss_lo();
		spi_tx(DFC_MEM_PAGE_TO_B1);
		spi_tx(addr.portion.first);
		spi_tx(addr.portion.second);
		spi_tx(addr.portion.third);
		spi_ss_hi();

		flash_wait_ready();

		// write page to flash buf
		addr.a32 = byte;

		spi_ss_lo();
		spi_tx(DFC_B1_WRITE);
		spi_tx(addr.portion.first);
		spi_tx(addr.portion.second);
		spi_tx(addr.portion.third);

		// send data
		while (len > 0) {

			spi_tx(*(uint8_t*)src);

			len--;
			if (!(func & WRITE_FUNC_NOSRCINC)) src++;
			byte++;

			if (byte == 264) { // current page filled
				next_page = 1;
				byte = 0;
				break;
			}
		}
		spi_ss_hi();

		flash_wait_ready();

		// flush from flash buf to flash
		addr.a32 = (((uint32_t)page << 9));

		spi_ss_lo();
		if (!(func & WRITE_FUNC_NOERASE))
			spi_tx(DFC_MEM_WRITE_B1);
		else
			spi_tx(DFC_MEM_WRITE_NOERASE_B1);
		spi_tx(addr.portion.first);
		spi_tx(addr.portion.second);
		spi_tx(addr.portion.third);
		spi_ss_hi();

	}

	return len;
}

void flash_erase_pages(uint16_t start, uint16_t count)
{
	uint16_t i;
	for (i = 0; i< count; i++) {
		flash_wait_ready();

		addr.a32 = (((uint32_t)(start + i) << 9));

		spi_ss_lo();
		spi_tx(DFC_PAGE_ERASE);
		spi_tx(addr.portion.first);
		spi_tx(addr.portion.second);
		spi_tx(addr.portion.third);
		spi_ss_hi();
	}
}

// returns chars read
uint16_t flash_read(void * dst, FLASH_P address, uint16_t len)
{
	if (address + len >= DF_SIZE_BYTES) {
		return 0;
	}

	flash_wait_ready();

	uint16_t byte;
	uint16_t page;

	byte = (address % 264) & 0x000001ff;
	page = (address / 264) & 0x000001ff;

	addr.a32 = (((uint32_t)page << 9) | byte);

	spi_ss_lo();
	spi_tx(DFC_CA_READ);
	spi_tx(addr.portion.first);
	spi_tx(addr.portion.second);
	spi_tx(addr.portion.third);

	// "Dont care" bits
	spi_tx(0);
	spi_tx(0);
	spi_tx(0);
	spi_tx(0);

	for (;len > 0; len--) {
		*((uint8_t*)dst) = spi_rx();
		dst++;
	}
	spi_ss_hi();

	return len;
}

uint16_t flash_pagenum(FLASH_P address)
{
	return (address / 264) & 0x000001ff;
}
