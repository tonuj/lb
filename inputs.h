
#ifndef INPUTS_H
#define INPUTS_H

#include "sch.h"

#define DIGITAL_INPUT_COUNT 6
typedef enum __digitalinp {
	DigitalInputIgn,
	DigitalInputGPI1,
	DigitalInputRing,
	DigitalInputJam,
	DigitalInputGSM
} DigitalInputs;

#define ANALOG_INPUT_COUNT 4
typedef enum __analoginp {
	AnalogInputVMon,
	AnalogInputIMon,
	AnalogInputA1,
	AnalogInputA2
} AnalogInputs;

typedef struct __inputs_t {
	SIGNAL chg[DIGITAL_INPUT_COUNT]; // signal is set on state change
} INPUTS_t;

extern INPUTS_t INPUTS;

void inputs_init();
void inputs_print();

uint8_t inputs_digital_get(DigitalInputs i);
uint16_t inputs_analog_get(AnalogInputs i);


#endif
