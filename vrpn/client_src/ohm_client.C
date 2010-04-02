/* sample client for vrpn_Ohmmeter device; Right now the interface is kind
   of specific to a particular ohmmeter from France called the ORPX2 made by
   ABB Barras Provence ("The French Ohmmeter")
*/

#include <stdlib.h>
#include <stdio.h>
#include <vrpn_Ohmmeter.h>
#include "orpx.h" // for parameter values

#define OHMMETER_SERVER "Ohmmeter0@argon"
#define NUM_SELECTION_VALUES (NUM_ORPX_SETTINGS)
#define AUTORANGE_WAIT (2)
#define NUM_OHMMETER_CHANNELS (NUM_ORPX_CHANNELS)

const char *VOLTAGES_STR[NUM_SELECTION_VALUES] ={"3uV","10uV","30uV","100uV",
        "300uV","1mV"};
const char *RANGES_STR[NUM_SELECTION_VALUES] = {">0.5",">5",">50",">500",">5k",
        ">50k"};
const char *FILTER_TIMES_STR[NUM_SELECTION_VALUES] ={"0.1", "0.3", "1.0", "3",
        "10", "30"};
const char *CHANNELS_STR[NUM_OHMMETER_CHANNELS] ={"ch 0", "ch 1", "ch 2",
        "ch 3"};

// this is something specific to the orpx. Since it has discrete levels
// for all its settings while the reports give actual floating point
// values for settings, we use this to keep track of the index
// of the value for each setting - see orpx.h
class OhmmeterChannelParams {
  public:
    OhmmeterChannelParams() {enable = 0; voltage = 0; range = 0; filter = 0;
                autorange = 0; avrgtime = 0.1;}
    int enable, voltage, range, filter, autorange;
    float avrgtime;
};
static OhmmeterChannelParams c_params[NUM_OHMMETER_CHANNELS];

// the ohmmeter device
static vrpn_Ohmmeter_Remote *theOhmmeter;


/*****************************************************************************
 *
   Callback handler with description of how to implement autoranging
 *
 *****************************************************************************/

static void    handle_measurement_change(void *userdata,
	const vrpn_OHMMEASUREMENTCB &info)
{
    static int num_over = 0;
    static int num_under = 0;
    int ch = info.channel_num;
    float fResist = (float)info.resistance;
    OhmmeterChannelParams *cparam = &(c_params[ch]);
    switch(info.status) {
	case MEASURING:
	    printf("measuring: %f ohms\n", fResist);
	    break;
	case ITF_ERROR:
	    printf("hardware interface error\n");
	    break;
	case M_OVERFLO:
	    printf("resistance is too large for range setting\n");
	    num_over++;
	    num_under = 0;
	    break;
	case M_UNDERFLO:
	    printf("resistance is too low for range setting\n");
	    num_over = 0;
	    num_under++;
	    break;
	case R_OVERFLO:
	    printf("resolution may be improved by increasing range setting\n");
	    break;
	default:
	    printf("unrecognized ohmmeter status\n");
    }

    // WARNING: possible bug in the following - note that we are 
    // assuming completely reliable transmission and execution of the
    // set_channel_parameters command - to fix this we would need
    // to compare the floating point values returned in the confirmation
    // message (e.g. in handle_settings_change()) with the values in the 
    // finite selection specific to this ohmmeter
    // (this is an inconvenience caused by the generality of the interface)
    if (num_over > AUTORANGE_WAIT){
	num_over = 0;
	// increase range for this channel if its not already at the maximum
	if (cparam->range < NUM_SELECTION_VALUES-1){
	    cparam->range++;
	    theOhmmeter->set_channel_parameters(ch, cparam->enable,
		orpx_voltages[cparam->voltage], orpx_ranges[cparam->range],
		orpx_filters[cparam->filter]);
	}
    }
    else if (num_under > AUTORANGE_WAIT) {
	num_under = 0;
	// decrease range for this channel if its not already at the minimum
	if (cparam->range > 0)
	    cparam->range--;
	    theOhmmeter->set_channel_parameters(ch, cparam->enable,
		orpx_voltages[cparam->voltage], orpx_ranges[cparam->range],
		orpx_filters[cparam->filter]);
    }

}

void    handle_settings_change(void *userdata, const vrpn_OHMSETCB &info)
{
    printf("Settings change for channel %d:\n", info.channel_num);
    printf("  enabled?: %d\n", info.enabled);
    printf("  (voltage, range, filter) = (%f, %f, %f)\n", 
		info.voltage, info.range_min, info.filter);
}

int main(int argc, char *argv[])
{
	int i;
	theOhmmeter = new vrpn_Ohmmeter_Remote(OHMMETER_SERVER);
	for (i = 0; i < NUM_OHMMETER_CHANNELS; i++){
	    c_params[i].enable = 0;
            c_params[i].voltage = 0;
            c_params[i].range = 0;
            c_params[i].filter = 0;
            theOhmmeter->set_channel_parameters(i, 0,
                orpx_voltages[0], orpx_ranges[0], orpx_filters[0]);
	}
	theOhmmeter->register_measurement_handler(NULL,
				handle_measurement_change);
	theOhmmeter->register_ohmset_handler(NULL, handle_settings_change);

        // main loop
        while (1){
                // Let the ohmmeter report status and measurements
                theOhmmeter->mainloop();
        }
}   /* main */
