/**************************************************************************************/
// vrpn_Router_Sierra_ServerMain.C
/**************************************************************************************/
// VRPN Server for controlling the Sierra video router
// in the UNC-Chapel Hill Computer Science Department's Graphics Lab.
//
// The purpose of this server is to let programs send commands over
// the LAN to set the video routing -- eg, to send video from
// a particular graphics machine (like Evans Channel 1) to
// a particular display device (like the G-Lab HMD's right eye).
//
// This server is intended to run on the PC "DC-1-CS" which is installed
// next to the Sierra video switch hardware in Sitterson Hall Room 272.
// This PC's COM1 port is hooked up to the serial input of the Sierra switch,
// so client apps can talk over the LAN to the server running on DC-1-CS,
// which in turn translates the commands it receives into the 
// command strings required by the video switch.
// To run the server on DC-1-CS, you first need to see the screen for
// DC-1-CS: hit Ctl-Alt-1 on the keyboard (the monitor is shared with other displays).
// Launch the server by double-clicking the "video switch server" icon on the desktop.
//
// See http://www.sierravideo.com/generic.html for command format
// for Sierra video routers.
//
// See David Harrison or Herman Towles for questions regarding the 
// video switch hardware and current cabling connections.  
//
// Written by Warren Robinett, initial version Jan 2000. 
// July 2000 -- integrated with VRPN 5.0, allowing multiple simultaneous clients.
/**************************************************************************************/
// INCLUDE FILES
#include "vrpn_Connection.h"
#include "vrpn_Router.h"
#include "vrpn_Router_Sierra_Server.h"

extern void readAndParseRouterConfigFile( void );



/**************************************************************************************/
// GLOBAL VARIABLES
vrpn_Synchronized_Connection	c;
vrpn_Synchronized_Connection *	connection = &c;	// ptr to VRPN connection
vrpn_Router *					router;				// ptr to video router server

const int minInputChannel  = vrpn_INPUT_CHANNEL_MIN;	// lowest channel #
const int maxInputChannel  = vrpn_INPUT_CHANNEL_MAX;	// one past highest channel #
const int minOutputChannel = vrpn_OUTPUT_CHANNEL_MIN;	// lowest channel #
const int maxOutputChannel = vrpn_OUTPUT_CHANNEL_MAX;	// one past highest channel #
const int minLevel         = vrpn_LEVEL_MIN;			// lowest level #
const int maxLevel         = vrpn_LEVEL_MAX;			// one past highest level #


/**************************************************************************************/
// Main program for the VRPN server for controlling the Sierra video router.  
int
main (unsigned argc, char *argv[])
{
	// Call constructor for the router server.
    if ( (router = new vrpn_Router_Sierra_Server( "Router0", connection )) == NULL ) {
		fprintf(stderr,"Can't create new vrpn_Router_Sierra_Server\n");
		return -1;
    }
	fprintf(stderr,"Created new vrpn_Router_Sierra_Server\n");
	

	// Read and parse the configuration file "router.cfg".
	// This file contains the names of the input and output channels
	// which are used by clients to refer to the channels.  
	// These names are stored in the router server name tables
	// input_channel_name and output_channel_name.
	readAndParseRouterConfigFile();

	// Send the input and output channel names to all clients.  
	// This is necessary in the case that a client was launched
	// prior to the server being launched.  
	router->send_all_names_to_client();


	// Server main loop.
    // Loop forever calling the mainloop()s for all devices
    while (1) {

			// Every 10 seconds, query the state of the router hardware.
			// Handle chars sent back from the router hardware in response
			// to commands from the server.  
            router->mainloop();
            
            // Send all messages which have been queued up across the connection.  
			// Receives messages which have come over the connection and 
			// calls the callback routines (associated with each message type)
			// to get the message data to the client or server (server, in this case).
            connection->mainloop();

			// sleep during loop so as not to eat CPU.  
			vrpn_SleepMsecs( 1 );
    }
	return 0;
}


/**************************************************************************************/
// Constructor for Sierra Router Server.
// Register the two types of message the server can receive:
//     vrpn_handle_set_link_message -------- handles VRPN messages from a client
//         requesting a link to be set using physical channel numbers:
//         (input, output, level).
//     vrpn_handle_set_named_link_message -- handles VRPN messages from a client
//         requesting a link to be set using channel name strings:
//         (name_string_1, name_string_2). 
// Then open the serial port to the router hardware and
// send a message down to the hardware asking it to report back
// the status of all the router channels 
// (ie, what input is linked to each output channel).  
vrpn_Router_Sierra_Server::vrpn_Router_Sierra_Server(
					const char * name, vrpn_Connection * c ) :
	vrpn_Router(name, c)	// Construct the base class, which registers message/sender
{
	// Register_autodeleted_handler() for handler on this end (server end)
	// to receive messages for changing the video router state 
	// (by setting a new link from an input channel to an output channel).

	// Register a handler for the set_link callback from this device,
	// if we got a connection.
	if (d_connection) {
		if (register_autodeleted_handler(set_link_m_id, vrpn_handle_set_link_message,
										this, d_sender_id)) {
			fprintf(stderr,"vrpn_Router_Sierra_Server: can't register handler\n");
			d_connection = NULL;
		}
	} 
	else {
		fprintf(stderr,"vrpn_Router_Sierra_Server: Can't get connection!\n");
	}

	// Register the set_named_link handler.
	if (d_connection) {
		if (register_autodeleted_handler(set_named_link_m_id, vrpn_handle_set_named_link_message,
										this, d_sender_id)) {
			fprintf(stderr,"vrpn_Router_Sierra_Server: can't register handler\n");
			d_connection = NULL;
		}
	} 
	else {
		fprintf(stderr,"vrpn_Router_Sierra_Server: Can't get connection!\n");
	}

		
	// Open the serial port that will be used to communicate with
	// the Sierra video router hardware.
	openSierraHardwareSerialPort();

	// The status of the router channels is initially unknown, so
	// send a command to the router hardware asking it to report
	// its complete status to the server.  
	queryStatusOfSierraHardwareViaSerialPort();
		// When the server receives the reply to this command from
		// the router hardware, 
		// the server will send VRPN change messages for all
		// channels to the client(s).  
		// The way this works is: the server's model of the router state
		// is initialized to "unknown", and as the hardware reports its
		// status to the server, all changes of channel status are 
		// sent by the server to the clients.  
}


/**************************************************************************************/
// This routine is called from the main loop of the server.
// It issues a periodic query of the router hardware to keep
// track of its state, and it handles the reply characters
// coming back from the router hardware in response to these
// queries and to other commands to the hardware (issued elsewhere).  
void vrpn_Router_Sierra_Server::mainloop( void )
{
	// Call the generic server mainloop function 
	// (defined in vrpn_BaseClass)
	// since we are a server.
	server_mainloop();	// handles timeouts


	// Receive characters on the COM1 serial line
	// (in reply to commands sent from the server)
	// and process them.  
	receiveStatusOfSierraHardwareViaSerialPort();


	// Keep track of elapsed real time, so that certain actions
	// can be taken at regular intervals.
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);

	vrpn_float64 update_rate = 0.1;    // 0.1 Hz = once every 10 seconds
	if ( duration(current_time,timestamp) >= 1000000.0 / update_rate) {

		// Update the time
		timestamp.tv_sec = current_time.tv_sec;
		timestamp.tv_usec = current_time.tv_usec;

		// Check the state of the router hardware every so often 
		// (once every 10 seconds)
		// in case someone manually changed the Sierra router state.
		// The reply from the Sierra router is handled by routines
		// router_Sierra_ReceiveChar and router_Sierra_ParseReply.
		queryStatusOfSierraHardwareViaSerialPort();
			// Why the router hardware is queried once every 10 seconds:
			// The response to the queryStatusOfSierraHardwareViaSerialPort 
			// ("**S!!") command 
			// by the Sierra router hardware will be
			// to send back a character string describing the status of every 
			// output channel of the router on all levels.
			// There are currently (July 2000) 5 levels and 32 channels per level.
			// A 10-char phrase is sent for each channel (eg "X01,01,01 " ).  
			// The 9600 baud serial data rate = approx 1 char per millisecond.
			// This works out to be 1600 characters, and takes 1.6 seconds to transmit
			// across the 9600 baud serial line.  
			// Thererfore, querying the router hardware more often than
			// every 1.6 seconds would be a bad idea.  
	}
}


/**************************************************************************************/
static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}




