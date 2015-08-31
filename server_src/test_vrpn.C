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
//	Before performing the above tests, VRPN checks its thread library
// to make sure the calls work as expected.  If that fails, the program
// returns an error message.  Note that there is no multi-threading happening
// in the actual message or other test code, just a testing of the thread
// library itself.
//
// Tested device types so far include:
//	vrpn_Analog_Server --> vrpn_Analog_Remote
//	vrpn_Button_Example_Server --> vrpn_Button_Remote
//	vrpn_Dial_Example_Server --> vrpn_Dial_Remote
//	vrpn_Text_Sender --> vrpn_Text_Receiver
//	vrpn_Tracker_NULL --> vrpn_Tracker_Remote

#include <stdio.h>                      // for printf, NULL, fprintf, etc

#include "vrpn_Analog.h"                // for vrpn_Analog_Remote, etc
#include "vrpn_Analog_Output.h"
#include "vrpn_Button.h"                // for vrpn_Button_Remote, etc
#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_Dial.h"                  // for vrpn_Dial_Remote, etc
#include "vrpn_Poser.h"                 // for vrpn_POSERCB, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday, etc
#include "vrpn_Text.h"                  // for vrpn_Text_Receiver, etc
#include "vrpn_Tracker.h"               // for vrpn_Tracker_Remote, etc
#include "vrpn_Types.h"                 // for vrpn_float64
#include "vrpn_Tracker_Filter.h"        // for vrpn_Tracker_DeadReckoning_Rotation

const char	*DIAL_NAME = "Dial0@localhost";
const char	*TRACKER_NAME = "Tracker0@localhost";
const char	*TEXT_NAME = "Text0@localhost";
const char	*ANALOG_NAME = "Analog0@localhost";
const char	*ANALOG_OUTPUT_NAME = "AnalogOutput0@localhost";
const char	*BUTTON_NAME = "Button0@localhost";
const char	*POSER_NAME = "Poser0@localhost";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on
int MAX_CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO + 10;

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

// The analog output server and remote
vrpn_Analog_Output_Callback_Server	*sanaout;
vrpn_Analog_Output_Remote		*ranaout;

// the poser server and remote
vrpn_Poser_Server* sposer;
vrpn_Poser_Remote* rposer;

// Counters incremented by callback handlers.
unsigned pcount = 0, vcount = 0, acount = 0;
unsigned dcount = 0;
unsigned tcount = 0;
unsigned ancount = 0;
unsigned aocount = 0;
unsigned bcount = 0;
unsigned pocount = 0, prcount = 0;

unsigned p1count = 0, p2count = 0;
unsigned b1count = 0, b2count = 0;


/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker pos, sensor %d\n", t.sensor);
        pcount++;
}

void	VRPN_CALLBACK handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	printf(" + vel, sensor %d\n", t.sensor);
        vcount++;
}

void	VRPN_CALLBACK handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	printf(" + acc, sensor %d\n", t.sensor);
        acount++;
}

void	VRPN_CALLBACK handle_dial (void *, const vrpn_DIALCB d)
{
	printf("Dial %d spun by %lf\n", d.dial, d.change);
        dcount++;
}

void	VRPN_CALLBACK handle_text (void *, const vrpn_TEXTCB t)
{
	printf("Received text \"%s\"\n", t.message);
        tcount++;
}

void	VRPN_CALLBACK handle_analog (void *, const vrpn_ANALOGCB a)
{
	printf("Received %d analog channels\n", a.num_channel);
        ancount++;
}

void	VRPN_CALLBACK handle_analog_output (void *, const vrpn_ANALOGOUTPUTCB a)
{
	printf("Received %d analog output channels (first is %lf)\n", a.num_channel, a.channel[0]);
        aocount++;
}

void	VRPN_CALLBACK handle_button (void *, const vrpn_BUTTONCB b)
{
	printf("Button %d is now in state %d\n", b.button, b.state);
        bcount++;
}


void	VRPN_CALLBACK handle_poser( void*, const vrpn_POSERCB p )
{
	printf( "Poser position/orientation:  (%lf, %lf, %lf) \n\t (%lf, %lf, %lf, %lf)\n", 
			p.pos[0], p.pos[1], p.pos[2], p.quat[0], p.quat[1], p.quat[2], p.quat[3] );
        pocount++;
}


void	VRPN_CALLBACK handle_poser_relative( void*, const vrpn_POSERCB p )
{
	printf( "Poser position/orientation relative:  (%lf, %lf, %lf) \n\t (%lf, %lf, %lf, %lf)\n", 
			p.pos[0], p.pos[1], p.pos[2], p.quat[0], p.quat[1], p.quat[2], p.quat[3] );
        prcount++;
}

void	VRPN_CALLBACK handle_pos1 (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker1 pos, sensor %d\n", t.sensor);
        p1count++;
}

void	VRPN_CALLBACK handle_pos2 (void *, const vrpn_TRACKERCB t)
{
	printf("Got tracker2 pos, sensor %d\n", t.sensor);
        p2count++;
}

void	VRPN_CALLBACK handle_button1 (void *, const vrpn_BUTTONCB b)
{
	printf("Button1 %d is now in state %d\n", b.button, b.state);
        b1count++;
}

void	VRPN_CALLBACK handle_button2 (void *, const vrpn_BUTTONCB b)
{
	printf("Button2 %d is now in state %d\n", b.button, b.state);
        b2count++;
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

void	create_and_link_analog_output_server(void)
{
	// Open the analog output remote using this connection
	sanaout = new vrpn_Analog_Output_Callback_Server (ANALOG_OUTPUT_NAME, connection, 12);

	// Set up the analog output callback handlers
	sanaout->register_change_handler(NULL, handle_analog_output);
}


void	create_and_link_poser_server( void )
{
	sposer = new vrpn_Poser_Server( POSER_NAME, connection );
	sposer->register_change_handler( NULL, handle_poser );
	sposer->register_relative_change_handler( NULL, handle_poser_relative );
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

	vrpn_gettimeofday(&now, NULL);
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

	vrpn_gettimeofday(&now, NULL);
	if (secs == 0) {	// First time through
		secs = now.tv_sec;
	}
	if ( now.tv_sec - secs >= 1 ) {
		secs = now.tv_sec;
		sana->report();
	}
}

void	send_analog_output_once_in_a_while(void)
{
	static	long	secs = 0;
	struct	timeval	now;

	vrpn_gettimeofday(&now, NULL);
	if (secs == 0) {	// First time through
		secs = now.tv_sec;
	}
	if ( now.tv_sec - secs >= 1 ) {
                static vrpn_float64 val = 1.0;
		secs = now.tv_sec;
                val++;
		ranaout->request_change_channel_value(0, val);
	}
}


void	send_poser_once_in_a_while( void )
{
	static long secs = 0;
	struct timeval	now;
	static vrpn_float64 p[3] = { 0, 0, 0 };
	static vrpn_float64 dp[3] = {0, 0, 0 };
	static vrpn_float64 q[4] = { 1, 1, 1, 1 };
	static int count = 0;
	static bool doRelative = true;

	vrpn_gettimeofday( &now, NULL );
	if( secs == 0 )
	{	// First time through
		secs = now.tv_sec;
	}
	if( now.tv_sec - secs >= 1 ) 
	{
		if( !doRelative )
		{
			// do a pose request
			p[count%3] += 1;
			if( p[count%3] > 1 ) p[count%3] = -1;
			rposer->request_pose( now, p, q );
			count++;
			doRelative = true;
		}
		else
		{	// do a relative pose request
			dp[count%3] = 0.25;
			dp[(count+1)%3] = 0;
			dp[(count+2)%3] = 0;
			rposer->request_pose_relative( now, dp, q );
			doRelative = false;
		}
		secs = now.tv_sec;
	}
}


int main (int argc, char * argv [])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return -1;
	}

    //---------------------------------------------------------------------
    // test the packing and unpacking routines
    if (!vrpn_test_pack_unpack()) {
        fprintf(stderr, "vrpn_test_pack_unpack() failed!\n");
        return -1;
    }

    //---------------------------------------------------------------------
	// test the thread library
	if (!vrpn_test_threads_and_semaphores()) {
		fprintf(stderr, "vrpn_test_threads_and_semaphores() failed!\n");
                fprintf(stderr, "  (This is not often used within VRPN, so it should not be fatal\n");
	} else {
		printf("Thread code passes tests (not using for the following though)\n");
	}

    //---------------------------------------------------------------------
	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	while (!connection->doing_okay() && CONNECTION_PORT < MAX_CONNECTION_PORT) {
		CONNECTION_PORT++;
		fprintf(stderr, "Could not open port - assuming parallel test. Increasing port number to %d and trying again\n", CONNECTION_PORT);
		connection = vrpn_create_server_connection(CONNECTION_PORT);
	}

	if (!connection->doing_okay()) {
        fprintf(stderr, "Hit port number limit - assuming something in port opening is broken!\n");
        return -1;
    }

    //---------------------------------------------------------------------
    // This test must be done while the connection is connected, or else it
    // will just drop the messages on the floor.
    // Try defining a sender and type and packing a message into the connection.
    // Try packing two messages that should not both fit to make sure that
    // it properly purges the first one and can pack the second.
    // Then try packing a message that is too large to fit and make sure it
    // does not succeed.

    // Set up the type and sender.
    vrpn_int32 my_sender = connection->register_sender("test");
    if (my_sender < 0) {
        fprintf(stderr, "Could not register sender\n");
        return -1;
    }
    vrpn_int32 my_type = connection->register_message_type("test");
    if (my_type < 0) {
        fprintf(stderr, "Could not register type\n");
        return -1;
    }

    // Get the connection connected.  We use a bogus receiver type.
    vrpn_Tracker_Remote test_remote("test@localhost");
    do {
        connection->mainloop();
        test_remote.mainloop();
    } while (!connection->connected());

    // Try packing the smaller message.
    char test_buffer[vrpn_CONNECTION_TCP_BUFLEN + 2];
    struct timeval test_now;
    vrpn_gettimeofday(&test_now, NULL);
    if (0 != connection->pack_message(vrpn_CONNECTION_TCP_BUFLEN / 2, test_now, my_type,
            my_sender, test_buffer, vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "Could not pack initial half-sized buffer\n");
        return -1;
    }

    // Try packing the second message that should fit by itself, but
    // not along with the first message.
    if (0 != connection->pack_message(vrpn_CONNECTION_TCP_BUFLEN / 2 + 2, test_now, my_type,
        my_sender, test_buffer, vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "Could not pack second half-sized++ buffer\n");
        return -1;
    }

    // Try packing the message that should not fit.  It should not work.
    if (0 == connection->pack_message(vrpn_CONNECTION_TCP_BUFLEN + 2, test_now, my_type,
        my_sender, test_buffer, vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "Could pack too-large message.\n");
        return -1;
    }

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

	//---------------------------------------------------------------------
	// Open the analog output remote , using this connection.
	ranaout = new vrpn_Analog_Output_Remote(ANALOG_OUTPUT_NAME, connection);
	printf("Analog's name is %s.\n", ANALOG_OUTPUT_NAME);
	create_and_link_analog_output_server();

	//---------------------------------------------------------------------
	// open the poser remote
	rposer = new vrpn_Poser_Remote( POSER_NAME, connection );
	printf( "Poser's name is %s.\n", POSER_NAME );
	create_and_link_poser_server();

	/* 
	 * main interactive loop
	 */
        int repeat_tests = 4;
	while ( repeat_tests > 0 ) {
		static	long	secs = 0;
		struct	timeval	now;

		// Let the servers, clients and connection do their things
		send_analog_output_once_in_a_while();
                ranaout->mainloop(); // The remote is on the server for AnalogOutput
                sanaout->mainloop(); // The server is on the client for AnalogOutput
		send_analog_once_in_a_while();
		sana->mainloop();
		rana->mainloop();
		send_text_once_in_a_while();
		rtext->mainloop();
		sdial->mainloop();
		rdial->mainloop();
		sbtn->mainloop();
		rbtn->mainloop();
		stkr->mainloop();
		rtkr->mainloop();
		send_poser_once_in_a_while();
		rposer->mainloop();
		sposer->mainloop();
		connection->mainloop();

		// Every 2 seconds, delete the old remotes and create new ones
		vrpn_gettimeofday(&now, NULL);
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
			delete sanaout;
			delete sposer;
			create_and_link_tracker_remote();
			create_and_link_dial_remote();
			create_and_link_button_remote();
			create_and_link_text_remote();
			create_and_link_analog_remote();
			create_and_link_analog_output_server();
			create_and_link_poser_server();
                        --repeat_tests;
		}

		// Sleep for 1ms each iteration so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}
        if ( (pcount == 0) || (vcount == 0) || (acount == 0) ||
             (dcount == 0) || (tcount == 0) || (ancount == 0) ||
             (aocount == 0) ||
             (bcount == 0) || (pocount == 0) || (prcount == 0) ) {
               fprintf(stderr,"Did not get callbacks from one or more device\n");
               return -1;
        }

        printf("\nDeleting _Remote objects\n");
        delete rtkr;
        delete rdial;
        delete rbtn;
        delete rtext;
        delete rana;
        delete ranaout;
        delete rposer;

        printf("Testing whether two connections to a tracker and to a button each get their own messages.\n");
        vrpn_Tracker_Remote *t1 = new vrpn_Tracker_Remote(TRACKER_NAME);
        t1->register_change_handler(NULL, handle_pos1);
        vrpn_Tracker_Remote *t2 = new vrpn_Tracker_Remote(TRACKER_NAME);
        t2->register_change_handler(NULL, handle_pos2);
        vrpn_Button_Remote *b1 = new vrpn_Button_Remote(BUTTON_NAME);
        b1->register_change_handler(NULL, handle_button1);
        vrpn_Button_Remote *b2 = new vrpn_Button_Remote(BUTTON_NAME);
        b2->register_change_handler(NULL, handle_button2);
        unsigned long secs;
        struct timeval start, now;
        vrpn_gettimeofday(&start, NULL);
        do {
          stkr->mainloop();
          sbtn->mainloop();
          t1->mainloop();
          t2->mainloop();
          b1->mainloop();
          b2->mainloop();
          connection->mainloop();

          vrpn_gettimeofday(&now, NULL);
          secs = now.tv_sec - start.tv_sec;
        } while (secs <= 2);
        if ( (p1count == 0) || (p2count == 0) ) {
               fprintf(stderr,"Did not get callbacks from trackers\n");
               return -1;
        }
        if ( (b1count == 0) || (b2count == 0) ) {
               fprintf(stderr,"Did not get callbacks from buttons\n");
               return -1;
        }

	printf("Deleting extra remote objects\n");
	delete t1;
	delete t2;
	delete b1;
	delete b2;

    printf("Deleting servers and connection\n");
    delete stkr;
    delete sbtn;
    delete sdial;
    delete stext;
    delete sana;
    delete sanaout;
    delete sposer;
    connection->removeReference();

    //---------------------------------------------------------------------
    // test various system objects that have defined test methods.
    if (vrpn_Tracker_DeadReckoning_Rotation::test() != 0) {
        fprintf(stderr, "Test of vrpn_Tracker_DeadReckoning_Rotation failed!\n");
        return -1;
    } else {
        printf("vrpn_Tracker_DeadReckoning_Rotation::test() passes\n");
    }

    printf("Success!\n");
	return 0;

}   /* main */


