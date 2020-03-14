
#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <inttypes.h>

typedef enum __output {
	Output1 = 0,
	Output2,
	OutputLED1,
	OutputLED2
} Output;

void outputs_init();
void outputs_set(Output nr, uint8_t val);
uint8_t outputs_get(Output nr);

#endif
