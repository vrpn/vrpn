// test_radamec_spi.C
//	This is a VRPN test program that has both clients and servers
// running within the same thread. It is intended to test whether a
// Radamec Serial Positioning Interface that is connected to a local
// serial port is working.
//

#include <stdio.h>                      // for printf, NULL, fprintf, etc

#include "vrpn_Analog.h"                // for vrpn_Analog_Remote, etc
#include "vrpn_Analog_Radamec_SPI.h"    // for vrpn_Radamec_SPI
#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs
#include "vrpn_Tracker.h"               // for vrpn_Tracker_Remote, etc

const char	*TRACKER_NAME = "Tracker0";
const char	*ANALOG_NAME = "Analog0";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

// The connection that is used by all of the servers and remotes
vrpn_Connection		*connection;

// The tracker remote
vrpn_Tracker_Remote	*rtkr;

// The analog remote
vrpn_Analog_Remote		*rana;

// The Serial unit
vrpn_Radamec_SPI		*sana;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker pos, sensor %d", t.sensor);
}

void	VRPN_CALLBACK handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	printf(" + vel, sensor %d", t.sensor);
}

void	VRPN_CALLBACK handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	printf(" + acc, sensor %d\n", t.sensor);
}

void	VRPN_CALLBACK handle_analog (void *, const vrpn_ANALOGCB a)
{
	printf("Received %d analog channels\n", a.num_channel);
}


/*****************************************************************************
 *
   Routines to create remotes and link their callback handlers.
 *
 *****************************************************************************/

void	create_and_link_tracker_remote(void)
{
	// Open the tracker remote using this connection
	rtkr = new vrpn_Tracker_Remote (TRACKER_NAME, connection);

	// Set up the tracker callback handlers
	rtkr->register_change_handler(NULL, handle_pos);
	rtkr->register_change_handler(NULL, handle_vel);
	rtkr->register_change_handler(NULL, handle_acc);
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
	if (argc != 2) {
		fprintf(stderr, "Usage: %s serial_port_name\n", argv[0]);
		return -1;
	}

	//---------------------------------------------------------------------
	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	//---------------------------------------------------------------------
	// Open the tracker server, using this connection, 2 sensors, update 1 times/sec
//	printf("Tracker's name is %s.\n", TRACKER_NAME);
//	create_and_link_tracker_remote();

	//---------------------------------------------------------------------
	// Open the analog server, using this connection.
	sana = new vrpn_Radamec_SPI(ANALOG_NAME, connection, argv[1], 38400);
	printf("Analog's name is %s.\n", ANALOG_NAME);
	create_and_link_analog_remote();

	/* 
	 * main interactive loop
	 */
	while ( 1 ) {

		// Let the servers, clients and connection do their things
		sana->mainloop();
		rana->mainloop();
//		rtkr->mainloop();
		connection->mainloop();

		// Sleep for 1ms each iteration so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}

	return 0;

}   /* main */


