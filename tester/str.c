#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "str.h"

#define PRINT_BUFSIZE 80

#define FLOAT_PLACES 100  // places after comma. 10^2 -> xx.xx


char * pr_P(PGM_P fmt, ...)
{
	static char buf[PRINT_BUFSIZE];//into stack for print-time

	char cP;
	va_list argptr;
	char * bufPtr = buf;

	uint8_t count = 0;

	va_start(argptr, fmt);
	
	while ((cP = pgm_read_byte(fmt))) {
		if (count > (PRINT_BUFSIZE - 10)) {
			break;
		}
		if (cP == '%') {

			fmt++;
			cP = pgm_read_byte(fmt);
			if (cP == 's') {
				char * str = va_arg(argptr, char * );
				while ((*(bufPtr++) = *(str++))) ; // copy string until 0

				bufPtr--; // no null 

#warning FIXME bufPtr might inc over PRINT_BUFSIZE
			} else if (cP == 'f') {

				// prints value 1234567 as 12345.67 . use this to print pseudo-floats
				int32_t val = va_arg(argptr, int32_t);

				ltoa(val / 100, bufPtr, 10); //integral part
				bufPtr += strlen(bufPtr);

				*(bufPtr++) = '.';  // dot

				if (val < 0) val *= -1;

				ltoa((val/10) % 10, bufPtr, 10); // first place
				bufPtr += strlen(bufPtr);

				ltoa(val % 10, bufPtr, 10); // second place
				bufPtr += strlen(bufPtr);
			
/*			} else if (cP == 'f') {
				double val = va_arg(argptr, double);

				itoa((int16_t)(val), bufPtr, 10); // int part
				bufPtr += strlen(bufPtr);

				*(bufPtr++) = '.'; //dot

				if (val < 0) val *= -1.0;

				itoa((int16_t)(val * FLOAT_PLACES) % FLOAT_PLACES, bufPtr, 10); // int part
				bufPtr += strlen(bufPtr);
*/
			} else if (cP == 'p') {
				PGM_P str = va_arg(argptr, PGM_P );
                                while ((*(bufPtr++) = pgm_read_byte(str++))) ; // copy string until 0
                                bufPtr--;

			} else if (cP == 'l') {
				ltoa(va_arg(argptr, int32_t), bufPtr, 10);
				bufPtr += strlen(bufPtr);

			} else if (cP == 'd') {
				itoa(va_arg(argptr, int16_t), bufPtr, 10);
				bufPtr += strlen(bufPtr);
			} else if (cP == 't') {
				if (va_arg(argptr, int16_t) == 0) {
					*(bufPtr++) = 'f';
				} else {
					*(bufPtr++) = 't';
				}
			} else  if (cP == 'x') {
				*(bufPtr++) = '0';
				*(bufPtr++) = 'x';
				itoa(va_arg(argptr, int16_t), bufPtr, 16);
				bufPtr += strlen(bufPtr);
			} else if (cP == 'b') {
				*(bufPtr++) = '0';
				*(bufPtr++) = 'b';
				itoa(va_arg(argptr, int16_t), bufPtr, 2);
				bufPtr += strlen(bufPtr);
			} else if (cP == 0) {
				break;
			} else {
				*(bufPtr++) = cP;
			}
		} else {
			*(bufPtr++) = cP;
		}
		fmt++;
		count = (bufPtr - buf) ;
	}

	*bufPtr = 0;

	va_end(argptr);

	if ((bufPtr - buf) > PRINT_BUFSIZE) {
		buf[0] = 0;
	}

	return buf;
}

uint8_t touint8(char *p) // in uupper case
{
	uint8_t i;
	uint8_t ret = 0;
	for (i = 0; i < 2; i++) {
		ret <<= 4;
		if (*p >= '0' && *p <= '9')  {
			ret |= 0xf & (*p - 0x30);
		} else if(*p >= 'A' && *p <= 'F') {
			ret |= 0xf & (*p - 0x37);
		} else {
			break;
		}
		p++;
	}
	return ret;
}

uint8_t indexof( const char * string,  char ch)
{
	uint8_t count = 0;
	while (*string != ch && *string != 0) { string++; count++; }

	return count;
}

void wait(uint32_t i) {
	while (i--) ;
}

