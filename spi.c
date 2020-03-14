#include <avr/io.h>

#include "spi.h"

#define SPI_SS		PIN4
#define SPI_MOSI	PIN5
#define SPI_MISO	PIN6
#define SPI_SCK		PIN7


void spi_init()
{
	// mosi  | sck | ss autputs
	DDRB |= _BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_SS);
	//  SS  |   MISO
	DDRB &= ~( _BV(SPI_MISO));

	// setup SPI mode 0
	// SPI enable MSB, Master, sck/4  
	SPCR = _BV(SPE) | _BV(MSTR);
}

inline void spi_ss_lo()
{
	asm volatile("nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		::); 

	PORTB &= ~_BV(SPI_SS);
	asm volatile("nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		::); 

}

inline void spi_ss_hi()
{
	asm volatile("nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		::); 
	PORTB |= _BV(SPI_SS);
	asm volatile("nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		"nop\n\t" 
		::); 
}

inline void spi_tx(uint8_t c) 
{ 
	SPDR = c; 

	while(!(SPSR & (1<<SPIF))) ; 
	c = SPDR;
} 

inline uint8_t spi_rx() 
{
	SPDR = 0;
	while(!(SPSR & (1<<SPIF))) ; 

	return SPDR; 
}

