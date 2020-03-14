#include <avr/io.h>

#include "io.h"


void io_init()
{
	DDRC &= ~( _BV(PIN6) | _BV(PIN7)); // buttons

	DDRD |= _BV(DDD4); // power relay
	PORTD &= ~_BV(PIN4); 

}

void io_powerup()
{
	PORTD |= _BV(PIN4);
}

void io_powerdown()
{
	PORTD &= ~_BV(PIN4);
}

/*! ADC value */
uint8_t io_get(uint8_t nr)
{
	switch(nr) {
	case 0:

	break;
	case 1:

	break;
	}

	return 0;
}

void io_wait_user()
{
	while ((PINC & _BV(PIN6)) == 0) ;
	while ((PINC & _BV(PIN6)) > 0) ;
}

void io_set(uint8_t nr, uint8_t state)
{
	
}
