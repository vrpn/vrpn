// test_vrpn.C
//	This is a VRPN test program that has both clients and servers
// running within the same thread. It is intended to test the sending
// and receiving of messages for the various types of devices that VRPN
// supports. It also tests the deletion and creation of "Remote" objects
// to make sure that it doesn't cause problems.
//	The basic idea is to instantiate both "server" devices (the example
// drivers are used for this) and "Remote" devices using the same connection
// for each. Then, the local call handlers on the connection will send the
// information from the server to the client callbacks.
//
// Tested device types so far include:
//	vrpn_Analog_Server --> vrpn_Analog_Remote
//	vrpn_Button_Example_Server --> vrpn_Button_Remote
//	vrpn_Dial_Example_Server --> vrpn_Dial_Remote
//	vrpn_Text_Sender --> vrpn_Text_Receiver
//	vrpn_Tracker_NULL --> vrpn_Tracker_Remote

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Dial.h"
#include "vrpn_Text.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

char	*DIAL_NAME = "Dial0";
char	*TRACKER_NAME = "Tracker0";
char	*TEXT_NAME = "Text0";
char	*ANALOG_NAME = "Analog0";
char	*BUTTON_NAME = "Button0";
int	CONNECTION_PORT = 4500;	// Port for connection to listen on

// The connection that is used by all of the servers and remotes
vrpn_Connection		*connection;

// The tracker server and remote
vrpn_Tracker_NULL	*stkr;
vrpn_Tracker_Remote	*rtkr;

// The dial server and remote
vrpn_Dial_Example_Server	*sdial;
vrpn_Dial_Remote		*rdial;

// The button server and remote
vrpn_Button_Example_Server	*sbtn;
vrpn_Button_Remote		*rbtn;

// The text sender and receiver
vrpn_Text_Sender		*stext;
vrpn_Text_Receiver		*rtext;

// The analog server and remote
vrpn_Analog_Server		*sana;
vrpn_Analog_Remote		*rana;


/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	handle_pos (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker pos, sensor %ld", t.sensor);
}

void	handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	printf(" + vel, sensor %ld", t.sensor);
}

void	handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	printf(" + acc, sensor %ld\n", t.sensor);
}

void	handle_dial (void *, const vrpn_DIALCB d)
{
	printf("Dial %ld spun by %lf\n", d.dial, d.change);
}

void	handle_text (void *, const vrpn_TEXTCB t)
{
	printf("Received text \"%s\"\n", t.message);
}

void	handle_analog (void *, const vrpn_ANALOGCB a)
{
	printf("Received %d analog channels\n", a.num_channel);
}

void	handle_button (void *, const vrpn_BUTTONCB b)
{
	printf("Button %d is now in state %d\n", b.button, b.state);
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

void	create_and_link_dial_remote(void)
{
	// Open the dial remote using this connection
	rdial = new vrpn_Dial_Remote (DIAL_NAME, connection);

	// Set up the dial callback handlers
	rdial->register_change_handler(NULL, handle_dial);
}

void	create_and_link_button_remote(void)
{
	// Open the button remote using this connection
	rbtn = new vrpn_Button_Remote (BUTTON_NAME, connection);

	// Set up the button callback handlers
	rbtn->register_change_handler(NULL, handle_button);
}

void	create_and_link_text_remote(void)
{
	// Open the text remote using this connection
	rtext = new vrpn_Text_Receiver (TEXT_NAME, connection);

	// Set up the dial callback handlers
	rtext->register_message_handler(NULL, handle_text);
}

void	create_and_link_analog_remote(void)
{
	// Open the analog remote using this connection
	rana = new vrpn_Analog_Remote (ANALOG_NAME, connection);

	// Set up the analog callback handlers
	rana->register_change_handler(NULL, handle_analog);
}

/*****************************************************************************
 *
   Server routines for those devices that rely on application intervention
 *
 *****************************************************************************/

void	send_text_once_in_a_while(void)
{
	static	long	secs = 0;
	struct	timeval	now;

	gettimeofday(&now, NULL);
	if (secs == 0) {	// First time through
		secs = now.tv_sec;
	}
	if ( now.tv_sec - secs >= 1 ) {
		secs = now.tv_sec;
		stext->send_message("Text message");
	}
}

void	send_analog_once_in_a_while(void)
{
	static	long	secs = 0;
	struct	timeval	now;

	gettimeofday(&now, NULL);
	if (secs == 0) {	// First time through
		secs = now.tv_sec;
	}
	if ( now.tv_sec - secs >= 1 ) {
		secs = now.tv_sec;
		sana->report();
	}
}


void main (int argc, char * argv [])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		exit(-1);
	}

	//---------------------------------------------------------------------
	// explicitly open the connection
	connection = new vrpn_Synchronized_Connection(CONNECTION_PORT);

	//---------------------------------------------------------------------
	// Open the tracker server, using this connection, 2 sensors, update 1 times/sec
	stkr = new vrpn_Tracker_NULL(TRACKER_NAME, connection, 2, 1.0);
	printf("Tracker's name is %s.\n", TRACKER_NAME);
	create_and_link_tracker_remote();

	//---------------------------------------------------------------------
	// Open the dial server, using this connection, 2 dials, spin 0.5 rev/sec,
	// update 1 times/sec
	sdial = new vrpn_Dial_Example_Server(DIAL_NAME, connection, 2, 0.5, 1.0);
	printf("Dial's name is %s.\n", DIAL_NAME);
	create_and_link_dial_remote();

	//---------------------------------------------------------------------
	// Open the button server, using this connection, defaults
	sbtn = new vrpn_Button_Example_Server(BUTTON_NAME, connection);
	printf("Button's name is %s.\n", BUTTON_NAME);
	create_and_link_button_remote();

	//---------------------------------------------------------------------
	// Open the text sender and receiver.
	stext = new vrpn_Text_Sender(TEXT_NAME, connection);
	printf("Text's name is %s.\n", TEXT_NAME);
	create_and_link_text_remote();

	//---------------------------------------------------------------------
	// Open the analog server, using this connection.
	sana = new vrpn_Analog_Server(ANALOG_NAME, connection);
	sana->setNumChannels(8);
	printf("Analog's name is %s.\n", ANALOG_NAME);
	create_and_link_analog_remote();

	/* 
	 * main interactive loop
	 */
	while ( 1 ) {
		static	long	secs = 0;
		struct	timeval	now;

		// Let the servers, clients and connection do their things
		send_analog_once_in_a_while();
		rana->mainloop();
		send_text_once_in_a_while();
		rtext->mainloop();
		sdial->mainloop();
		rdial->mainloop();
		sbtn->mainloop();
		rbtn->mainloop();
		stkr->mainloop();
		rtkr->mainloop();
		connection->mainloop();

		// Every 2 seconds, delete the old remotes and create new ones
		gettimeofday(&now, NULL);
		if (secs == 0) {	// First time through
			secs = now.tv_sec;
		}
		if ( now.tv_sec - secs >= 2 ) {
			secs = now.tv_sec;
			printf("\nDeleting and restarting _Remote objects\n");
			delete rtkr;
			delete rdial;
			delete rbtn;
			delete rtext;
			delete rana;
			create_and_link_tracker_remote();
			create_and_link_dial_remote();
			create_and_link_button_remote();
			create_and_link_text_remote();
			create_and_link_analog_remote();
		}

		// Sleep for 1ms each iteration so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}

}   /* main */


