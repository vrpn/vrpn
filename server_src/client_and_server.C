// client_and_server.C
//	This is a VRPN example program that has a client and server for a
// tracker both within the same thread.
//	The basic idea is to instantiate both a vrpn_Tracker_NULL and
// a vrpn_Tracker_Remote using the same connection for each. Then, the
// local call handlers on the connection will send the information from
// the server to the client callbacks.

#include <stdio.h>                      // for fprintf, stderr, printf, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs
#include "vrpn_Tracker.h"               // for vrpn_TRACKERCB, etc
#include "vrpn_Types.h"                 // for vrpn_float64

const char	*TRACKER_NAME = "Tracker0";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

vrpn_Tracker_NULL	*ntkr;
vrpn_Tracker_Remote	*tkr;
vrpn_Connection		*connection;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos (void *, const vrpn_TRACKERCB t)
{
	static	int	count = 0;

	fprintf(stderr, "%d.", t.sensor);
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("Pos, sensor %d = %f, %f, %f\n", t.sensor,
				t.pos[0], t.pos[1], t.pos[2]);
			count = 0;
		}
	}
}

void	VRPN_CALLBACK handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	//static	int	count = 0;

	fprintf(stderr, "%d/", t.sensor);
}

void	VRPN_CALLBACK handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	//static	int	count = 0;

	fprintf(stderr, "%d~", t.sensor);
}

int main (int argc, char * argv [])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return -1;
	}

	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	// Open the tracker server, using this connection, 2 sensors, update 60 times/sec
	ntkr = new vrpn_Tracker_NULL(TRACKER_NAME, connection, 2, 60.0);

	// Open the tracker remote using this connection
	fprintf(stderr, "Tracker's name is %s.\n", TRACKER_NAME);
	tkr = new vrpn_Tracker_Remote (TRACKER_NAME, connection);

	// Set up the tracker callback handlers
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	/* 
	 * main interactive loop
	 */
	while ( 1 ) {
		// Let the tracker server, client and connection do their things
		ntkr->mainloop();
		tkr->mainloop();
		connection->mainloop();

		// Sleep for 1ms so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}

	return 0;

}   /* main */


