// client_and_server.C
//	This is a VRPN example program that has a client and server for a
// tracker both within the same thread.
//	The basic idea is to instantiate both a vrpn_Tracker_NULL and
// a vrpn_Tracker_Remote using the same connection for each. Then, the
// local call handlers on the connection will send the information from
// the server to the client callbacks.

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <iostream>

#if defined(VRPN_USE_WIIUSE)
#include "vrpn_Tracker.h"
#include "vrpn_WiiMote.h"
#include "vrpn_Tracker_WiimoteHead.h"


const char* TRACKER_NAME = "Tracker0";
const char* WIIMOTE_NAME = "WiimoteForHead";
const char* WIIMOTE_REMOTE_NAME = "*WiimoteForHead";
const int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;  // Port for connection to listen on
const int	TRACKER_FREQUENCY = 60;
const int	DEBUG_DISPLAY_INTERVAL = 3; // # of seconds between status displays

vrpn_Tracker_WiimoteHead* wmtkr;
vrpn_WiiMote* wiimote;
vrpn_Tracker_Remote* tkr;
vrpn_Connection* connection;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos(void*, const vrpn_TRACKERCB t) {
	static int count = 0;
	fprintf(stderr, ".");
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("\nXl:(%5f, %5f, %5f) Rt:((%5f, %5f, %5f) %5f)\n",
			       t.pos[0], t.pos[1], t.pos[2], t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
			count = 0;
		}
	}
}

void	VRPN_CALLBACK handle_vel(void*, const vrpn_TRACKERVELCB t) {
	//static	int	count = 0;

	fprintf(stderr, "/");
}

void	VRPN_CALLBACK handle_acc(void*, const vrpn_TRACKERACCCB t) {
	//static	int	count = 0;

	fprintf(stderr, "~");
}


int main(int argc, char* argv []) {
	int wmnum = -1;
	if (argc == 1) {
		fprintf(stderr, "Using wiimote 0 by default... to specify another (0-3):\n");
		fprintf(stderr, "%s WIIMOTENUM\n\n", argv[0]);
		wmnum = 0;
	} else if (argc == 2) {
		wmnum = atoi(argv[1]);
		if (wmnum == -1) {
			// improper # provided - pretend as if we had too many arguments
			// so we error out.
		}
	}

	if (argc > 2) {
		fprintf(stderr, "Please run this with no arguments or optionally a wiimote number (0-3)\n");
		fprintf(stderr, "Usage: %s [WIIMOTENUM]\n", argv[0]);
		return -1;
	}

	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);


	if (!connection) {
		fprintf(stderr, "Could not create connection!\n");
		fprintf(stderr, "Press 'enter' to exit...\n");
		int tmp;
		std::cin >> tmp;
		return -1;
	}

	wiimote = new vrpn_WiiMote(WIIMOTE_NAME, connection, wmnum);
	if (!wiimote) {
		fprintf(stderr, "Could not create Wiimote server named %s!\n", WIIMOTE_NAME);
		fprintf(stderr, "Press 'enter' to exit...\n");
		int tmp;
		std::cin >> tmp;
		return -1;
	}

	wmtkr = new vrpn_Tracker_WiimoteHead(TRACKER_NAME, connection, WIIMOTE_REMOTE_NAME, 60.0);
	if (!wmtkr) {
		fprintf(stderr, "Could not create Wiimote Head Tracker named %s!\n", TRACKER_NAME);
		fprintf(stderr, "Press 'enter' to exit...\n");
		int tmp;
		std::cin >> tmp;
		return -1;
	}

	// Open the tracker remote using this connection
	fprintf(stderr, "\nTracker's name is %s.\n", TRACKER_NAME);
	tkr = new vrpn_Tracker_Remote(TRACKER_NAME, connection);

	// Set up the tracker callback handlers
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	/*
	 * main interactive loop
	 */
	while (1) {
		// Let the tracker server, client and connection do their things
		// The wiimote tracker runs the wiimote's mainloop.
		wiimote->mainloop();
		wmtkr->mainloop();
		tkr->mainloop();
		connection->mainloop();

		// Sleep for 1ms so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}

	return 0;
}   /* main */

#else

// if not defined(VRPN_USE_WIIUSE)
int main(int argc, char* argv []) {
	fprintf(stderr, "Error: This app doesn't do anything if you didn't compile against WiiUse.\n");
	return -1;
}

#endif // defined(VRPN_USE_WIIUSE)
