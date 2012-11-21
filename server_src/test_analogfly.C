// test_analogfly.C
//	This is a VRPN test program that has both clients and servers
// running within the same thread. It is intended to test whether the
// vrpn_Tracker_AnalogFly class is working as intended.  It does this
// by creating an analog to drive both an absolute and a differential
// AnalogFly and then seeing what their resulting outputs are.

#include <stdio.h>                      // for printf, NULL, fprintf, etc
#include <stdlib.h>                     // for exit

#include "vrpn_Analog.h"                // for vrpn_Analog_Server, etc
#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKERCB, etc
#include "vrpn_Tracker_AnalogFly.h"     // for vrpn_TAF_axis, etc
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_false, etc

const char	*TRACKER1_NAME = "Tracker1";
const char	*TRACKER2_NAME = "Tracker2";
const char	*ANALOG_NAME = "Analog0";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

int getting_analog_values = 0;	//< Are we getting analog reports?

// The connection that is used by all of the servers and remotes
vrpn_Connection		*connection;

// The tracker remotes
vrpn_Tracker_Remote	*rtkr1;
vrpn_Tracker_Remote	*rtkr2;

// The analog remote
vrpn_Analog_Remote	*rana;

// The analog server to drive all this
vrpn_Analog_Server	*sana;

// The tracker servers
vrpn_Tracker_AnalogFly	*stkr1;
vrpn_Tracker_AnalogFly	*stkr2;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos1 (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker1 pos: sensor %d at (%f, %f, %f)\n", t.sensor,
	    t.pos[0], t.pos[1], t.pos[2]);
}

void	VRPN_CALLBACK handle_pos2 (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker2 pos: sensor %d at (%f, %f, %f)\n", t.sensor,
	    t.pos[0], t.pos[1], t.pos[2]);
}

void	VRPN_CALLBACK handle_analog (void *, const vrpn_ANALOGCB a)
{
    if (a.channel[0] != 1.0) {
	fprintf(stderr, "handle_analog: Expected 1.0, got %f\n",a.channel[0]);
    } else {
	getting_analog_values = 1;
    }
}


/*****************************************************************************
 *
   Routines to create remotes and link their callback handlers.
 *
 *****************************************************************************/

void	create_and_link_tracker_remotes(void)
{
	// Open the tracker remote using this connection
	rtkr1 = new vrpn_Tracker_Remote (TRACKER1_NAME, connection);
	rtkr2 = new vrpn_Tracker_Remote (TRACKER2_NAME, connection);

	// Set up the tracker callback handlers
	rtkr1->register_change_handler(NULL, handle_pos1);
	rtkr2->register_change_handler(NULL, handle_pos2);
}

void	create_and_link_analog_remote(void)
{
	// Open the analog remote using this connection
	rana = new vrpn_Analog_Remote (ANALOG_NAME, connection);

	// Set up the analog callback handlers
	rana->register_change_handler(NULL, handle_analog);
}

int main (int argc, char * argv [])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		exit(-1);
	}

	//---------------------------------------------------------------------
	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	//---------------------------------------------------------------------
	// Open the analog server, using this connection.
	sana = new vrpn_Analog_Server(ANALOG_NAME, connection);
	sana->setNumChannels(1);
	sana->channels()[0] = 1.0;	//< Set the value to 1
	printf("Analog's name is %s.\n", ANALOG_NAME);
	create_and_link_analog_remote();

	//---------------------------------------------------------------------
	// Open the tracker remotes, using this connection.
	printf("Tracker 1's name is %s.\n", TRACKER1_NAME);
	printf("Tracker 2's name is %s.\n", TRACKER2_NAME);
	create_and_link_tracker_remotes();

	//---------------------------------------------------------------------
	// Create the differential and absolute trackers.  Both should send one
	// report every two seconds.  First, create the axis descriptor (used
	// for all axes).  Then pack this into the parameter descriptor and start
	// the relative tracker and absolute tracker.
	
	vrpn_TAF_axis	taf_axis1;  // Axis that returns values from the analog
	char	name[500];
	sprintf(name, "*%s", ANALOG_NAME);
	taf_axis1.name = name;
	taf_axis1.channel = 0;
	taf_axis1.offset = 0.0;
	taf_axis1.thresh = 0.0;
	taf_axis1.scale = 1.0;
	taf_axis1.power = 1.0;

	vrpn_TAF_axis	taf_axisNULL;	// Axis that doesn't return anything
	taf_axisNULL.name = NULL;
	taf_axisNULL.channel = 0;
	taf_axisNULL.offset = 0.0;
	taf_axisNULL.thresh = 0.0;
	taf_axisNULL.scale = 1.0;
	taf_axisNULL.power = 1.0;

	vrpn_Tracker_AnalogFlyParam p;
	p.reset_name = NULL;
	p.reset_which = 0;
	p.x = taf_axis1;
	p.y = taf_axis1;
	p.z = taf_axis1;
	p.sx = taf_axisNULL;	// Don't want any rotation!
	p.sy = taf_axisNULL;
	p.sz = taf_axisNULL;

	stkr1 = new vrpn_Tracker_AnalogFly(TRACKER1_NAME, connection, &p, 0.5, vrpn_false);
	stkr2 = new vrpn_Tracker_AnalogFly(TRACKER2_NAME, connection, &p, 0.5, vrpn_true);

	/* 
	 * main interactive loop
	 */

	printf("You should see tracker1 positions increasing by 2 per 2 seconds\n");
	printf("You should see tracker2 positions remaining at 1\n");

	struct timeval start, now, diff;
	vrpn_gettimeofday(&start, NULL);
	vrpn_gettimeofday(&now, NULL);
	diff = vrpn_TimevalDiff(now, start);
	while ( diff.tv_sec <= 10 ) {

	    // Make sure that we are getting analog values
	    {	static	struct	timeval	last_report;
		static	int	first = 1;
		struct timeval now;

		if (first) {
		    vrpn_gettimeofday(&last_report, NULL);
		    first = 0;
		}
		vrpn_gettimeofday(&now, NULL);
		if (now.tv_sec - last_report.tv_sec > 1) {
		    if (!getting_analog_values) {
			fprintf(stderr, "Error - not getting analog values!\n");
		    }
		    vrpn_gettimeofday(&last_report, NULL);
		    getting_analog_values = 0; // Make sure we get more next time
		}
	    }

	    // Let the servers, clients and connection do their things
	    sana->report(); sana->mainloop();
	    stkr1->mainloop();
	    stkr2->mainloop();
	    rana->mainloop();
	    rtkr1->mainloop();
	    rtkr2->mainloop();
	    connection->mainloop();

	    // Sleep for 1ms each iteration so we don't eat the CPU
	    vrpn_SleepMsecs(1);
	    vrpn_gettimeofday(&now, NULL);
	    diff = vrpn_TimevalDiff(now, start);
	}

	delete sana;
	delete rtkr1;
	delete rtkr2;
	delete stkr1;
	delete stkr2;
	delete connection;

	return 0;
}   /* main */
