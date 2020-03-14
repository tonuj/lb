#include "inputs.h"
#include "outputs.h"
#include "gps.h"
#include "sch.h"
#include "gsm.h"
#include "mkx.h"
#include "timer.h"
#include "log.h"

#include "uart.h"

static void event_trace();
static void event_create(EVENT_RECORD * rec);
static void event_sw_profile();

static uint16_t event_id = 0;


static PROC worker1;
static PROC worker2;

void event_init()
{
	 event_id = log_get_event_id() + 1; // get next

	sch_reg(&worker1, PSTR("ev_ignswitch"), 1000, &(INPUTS.chg[DigitalInputIgn]), 0, &event_sw_profile);// 1 sec min chg time
	sch_reg(&worker2, PSTR("ev_trace_gen"), 10000, 0, 0, &event_trace);

	event_sw_profile();

}

static void event_trace()
{
	event_post();
}

void event_post()
{
	if (event_id != 0 && GPS.initialized == 0 && TIMER.millis < 10000) return; // 10 first seconds after startup don't create event

	EVENT_RECORD rec;

	event_create(&rec);
	log_push(&rec);
}

static void event_sw_profile()
{
	if (inputs_digital_get(DigitalInputIgn)) { // ignition on
		worker2.interval = 30000;
	} else {
		worker2.interval = 3600000;
	}
}

static void event_create(EVENT_RECORD * rec)
{
	rec->eventID = event_id++;
	rec->code = EVENT_CODE_TRACE;

	// uint32_t time = (h*3600 + m*60 + s) & 0x00ffffff  12:50:15 
	MK3_TIME t;
	t.time = (GPS.hours*3600lu + GPS.minutes*60lu + GPS.seconds) & 0x00fffffflu;
	rec->ds.time[0] = t.bytes[0];
	rec->ds.time[1] = t.bytes[1];
	rec->ds.time[2] = t.bytes[2];

	// ((yy)*372 + (mm - 1)*31 + (dd - 1)) 
	rec->ds.date = ((GPS.years) * 372 + (GPS.months - 1)*31 + (GPS.days - 1)) ;

	rec->ds.GPRMC_OK = GPS.valid;

	// bearing / 3
	rec->ds.GPRMC_bearing = GPS.bearing / 300;

	// 5917.8166 -> 59178166. south is negative and west is neg.
	rec->ds.GPRMC_lat = GPS.lat;
	rec->ds.GPRMC_lon = GPS.lon;

	// in knots. (123.40) 12340 -> 1234
	rec->ds.GPRMC_speed = GPS.speed / 10;

	rec->ds.inputs = inputs_digital_get(DigitalInputIgn) | (inputs_digital_get(DigitalInputGPI1) << 1) | (inputs_digital_get(DigitalInputRing) << 2) | (inputs_digital_get(DigitalInputJam) << 3);
/*
        DigitalInputIgn, DigitalInputGPI1, DigitalInputRing, DigitalInputJam, DigitalInputGSM
*/

	rec->ds.outputs = outputs_get(Output1) | outputs_get(Output2) << 1;
	rec->ds.analog_input = inputs_analog_get(AnalogInputA1) >> 2;
	rec->ds.power_input = inputs_analog_get(AnalogInputVMon); 
	rec->ds.status = 0;
	rec->ds.extra = 0;
	rec->ds.fuel = 0;

	rec->event_data_size = 0;
	//rec->event_data[EVENT]
}
