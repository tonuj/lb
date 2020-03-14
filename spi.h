
#ifndef SPI_H
#define SPI_H

#include <inttypes.h>

void spi_init();
inline void spi_ss_lo();
inline void spi_ss_hi();
inline void spi_tx(uint8_t c);
inline uint8_t spi_rx();

#endif

