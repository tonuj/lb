#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>

#include "event.h"
#include "uart.h"
#include "inputs.h"
#include "str.h"
#include "sch.h"

#define LB_VREF 303
#define ANALOG_VMON_CALIB 1110 // 11.10
#define ANALOG_IMON_CALIB 4131 // 41.31 //24.21

INPUTS_t INPUTS;

uint8_t digital_input[DIGITAL_INPUT_COUNT];
uint16_t analog_input[ANALOG_INPUT_COUNT];

static PROC update_a, update_d;

static void inputs_set(DigitalInputs inp, uint8_t newval);
static void inputs_sample_digi();
static void inputs_sample_ad();

void inputs_init()
{
	memset(&INPUTS, 0, sizeof(INPUTS_t));
	memset(digital_input, 0, DIGITAL_INPUT_COUNT);
	memset(analog_input, 0, ANALOG_INPUT_COUNT);

	// analog
	DDRA &= ~_BV(DDA0); //  VMON
	DDRA &= ~_BV(DDA1); //  IMON
	DDRA &= ~_BV(DDA6); //  A1 (i2c sc
	DDRA &= ~_BV(DDA7); //  A2 (i2c sd)

	DDRC &= ~_BV(DDC0); //  twi
	DDRC &= ~_BV(DDC1); //  twi
	PORTC &= ~_BV(PIN0);
	PORTC &= ~_BV(PIN1);

	//digital
	DDRB &= ~_BV(DDB0); // RING
	DDRB &= ~_BV(DDB1); // IGN
	DDRB &= ~_BV(DDB2); // INT
	DDRB &= ~_BV(DDB3); // I1

	DDRC &= ~_BV(DDC6); // GSM PWRMON
	DDRC &= ~_BV(DDC7); // JAMMING

	ADMUX = 0;
	ADCSRA = 0;

	ACSR |= _BV(ACD); // analog comparator off

	MCUCSR &= ~_BV(ISC2); // rising edge generates int
	GIFR = _BV(INTF2);
	GICR |= _BV(INT2);

	sch_reg(&update_a, PSTR("inp_ad_samp"), 1000, 0, 0, &inputs_sample_ad);
	sch_reg(&update_d, PSTR("inp_di_samp"), 300, 0, 0, &inputs_sample_digi);
}

static void inputs_sample_ad()
{
	uint16_t value_raw;
	ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0); // ps == 8 
//ANALOG
	uint8_t mux;
	for (mux = 0; mux < 4; mux++) {
		if (mux == 2) ADMUX = 6; // Ain1
		else if (mux == 3) ADMUX = 7; // ain2
		else {
			ADMUX = mux; //| _BV(REFS0); // AVCC with external capacitor at AREF pin 
		}

		ADCSRA |= _BV(ADSC); // start

		while ((ADCSRA & _BV(ADSC))) ;  // stays high until completion

		value_raw = ADCL;
		value_raw |= ADCH<<8;

		analog_input[mux] = value_raw;
	}
	ADCSRA = 0; // disable

	//INPUTS.analog_input[AIN_VMON].value_proc = (uint32_t)INPUTS.analog_input[AIN_VMON].value_proc * 100 / ANALOG_VMON_CALIB;  // volts
	//INPUTS.analog_input[AIN_IMON].value_proc = (uint32_t)INPUTS.analog_input[AIN_IMON].value_proc * 100 / ANALOG_IMON_CALIB;  // milliampres 
}

uint8_t inputs_digital_get(DigitalInputs i)
{
	return (digital_input[i] ? 1:0);
}

uint16_t inputs_analog_get(AnalogInputs i)
{
	uint32_t val = analog_input[i];

	val = (val * LB_VREF) / 1024;

	if ( i == AnalogInputVMon )  return (val * ANALOG_VMON_CALIB) / 100;
	else if ( i == AnalogInputIMon )  return (val * ANALOG_IMON_CALIB) / 100;

	return analog_input[i];
}

static void inputs_sample_digi()
{
	inputs_set(DigitalInputIgn,   PINB & _BV(PIN1));
	inputs_set(DigitalInputGPI1,  PINB & _BV(PIN3));
	inputs_set(DigitalInputRing, !( PINB & _BV(PIN0))); // thois one must be reversed
	inputs_set(DigitalInputJam, PINB & _BV(PIN7));
	inputs_set(DigitalInputGSM,  PINB & _BV(PIN6));
}

static void inputs_set(DigitalInputs inp, uint8_t newval)
{
	if (digital_input[inp] != newval) {
		 INPUTS.chg[inp] = 1; // set signal

		digital_input[inp] = newval;
	}
}

ISR(INT2_vect)
{
	inputs_sample_digi();
}

void inputs_print()
{
	DBG("VMon:%fV IMon:%fmA AIn1:%u AIn2:%u",
		(int32_t)inputs_analog_get(AnalogInputVMon),
		(int32_t)inputs_analog_get(AnalogInputIMon),
		inputs_analog_get(AnalogInputA1),
		inputs_analog_get(AnalogInputA2));

	DBG("ign:%t gpi1:%t ring:%t jam:%t gsm_pwrmon:%t", digital_input[DigitalInputIgn], digital_input[DigitalInputGPI1], digital_input[DigitalInputRing], digital_input[DigitalInputJam], digital_input[DigitalInputGSM]);
}
