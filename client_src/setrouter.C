/**************************************************************************************/
/**************************************************************************************/
// File setrouter.C
//
// Command-line Client for Sierra Video Router Server  
// (uses text channel names as command line args)
// Warren Robinett
// Computer Science Dept.
// University of North Carolina at Chapel Hill
// Aug 2000

/**************************************************************************************/
/**************************************************************************************/
// INCLUDE FILES
#include <string>
#include "vrpn_Router.h"


void	setNamedLink( char* channelName1, char* channelName2 );



/**************************************************************************************/
/**************************************************************************************/
// GLOBAL VARIABLES
vrpn_Router_Remote *router;


/**************************************************************************************/
// Main routine.
int
main( int argc, char *argv[])
{
	// Open the named router.
	char routerName[100] = "Router0@DC-1-CS";  // This is the PC in SN272
	printf( "Attempting to open vrpn_Router_Remote for \"%s\"\n", routerName );
	router = new vrpn_Router_Remote( routerName );


	// Check that the command line has the right number of arguments.
	// If not, print usage message.
	if( argc == 1 ) {
		printf( "Usage: setrouter <channel_name>:<channel_name>\n" );
		return 1;
	}


	// For each arg in command line, try to interpret it
	// as a router set-link command.
	for( int i=1; i<argc; i++ ) {
		printf( "Arg%d...\n", i );

		// Copy next command line arg into buffer.
		char buf[vrpn_NAME_MAX];
		strncpy( buf, argv[i], vrpn_NAME_MAX );
		buf[vrpn_NAME_MAX-1] = 0;	// enforce zero-termination

		// Search through string for colon.
		// If not found, skip this cmd line arg.
		// If found, replace colon with zero (null) char
		// and set pointers to the starts of the two strings.
		char* name1 = 0;
		char* name2 = 0;
		for( int k=0; buf[k] != 0; k++ ) {
			if( buf[k] == ':' ) {
				// First string goes from start of command string to colon (now zero terminator).
				name1 = &buf[0];
				buf[k] = 0;

				// Second string goes from right after colon to end of original command string.
				// (If colon was last char, this works: string 2 is empty string.)
				name2 = &buf[k+1];
				goto foundColon;
			}
		}
		// Failed to find colon in command string.
		// Print error message and go on to next command line arg.  
		printf( "setrouter: failed to find colon in command line arg \"%s\".\n", argv[i] );
		continue;

		foundColon:
				//printf( "name1:%s, name2:%s\n", name1, name2 );
		// Successfully broke the command string into two name strings (name1, name2).
		char* channelName1 = name1;
		char* channelName2 = name2;

		printf( "Sending set_router command to server to link %s and %s.\n", 
				channelName1, channelName2 );


			
		// Send Set_Link command to video router server using VRPN.
		setNamedLink( channelName1, channelName2 );
	}

	return 0;               /* ANSI C requires main to return int. */
}


/**************************************************************************************/
void
setNamedLink( char* channelName1, char* channelName2 )
{
	int j;
	for( j=0; j<1; j++ ) {
		if( router)   router->mainloop();
		vrpn_SleepMsecs( 10 );
	}
	for( j=0; j<1; j++ ) {
		if( router)   router->mainloop();
		vrpn_SleepMsecs( 10 );

		// Send a command across the LAN to a VRPN server to set the Sierra
		// video router to a new setting.  
		router->set_router( channelName1, channelName2 );
	}
	for( j=0; j<1; j++ ) {
		if( router)   router->mainloop();
		vrpn_SleepMsecs( 10 );
	}
}
