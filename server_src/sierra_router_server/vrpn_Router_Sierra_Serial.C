/**************************************************************************************/
// vrpn_Router_Sierra_Serial.C
/**************************************************************************************/
#include "vrpn_Router.h"
#include "vrpn_Serial.h"

extern void	router_Sierra_ReceiveChar( char c );


/**************************************************************************************/
// GLOBAL VARIABLES
int		serialfd;		// serial port file descriptor


/**************************************************************************************/
// Open the serial port through which the router server talks to
// the router hardware.  
void
openSierraHardwareSerialPort( void )
{
    // Open the serial port that will be used to communicate with
    // the Sierra video router.
	char serialPortName[256] = "COM1";
	long int baudRate = 9600;		// 9600 baud = approx 1 char per millisecond
	char message[1024];
	serialfd = -1;
    serialfd = vrpn_open_commport(serialPortName, baudRate);
    if (serialfd < 0) {
		sprintf(message,"Video router server: error opening serial port: %s\n",
			serialPortName);
		fprintf( stderr, message );
		exit( 1 );
		return;
    }
	else {
		sprintf(message,"Video router server: opened serial port: %s\n",
			serialPortName);
		fprintf( stderr, message );
	}
}


/**************************************************************************************/
// Construct a command string to set output video channel "outputIndex"
// to input video channel "inputIndex" on level "level"..  
// Then send this ASCII command
// to the Sierra video router hardware via the serial port.  
// This will cause the video router hardware to change state.  
// Example call: setLinkInSierraHardwareViaSerialPort( output, input, level );
void
setLinkInSierraHardwareViaSerialPort( int outputIndex, int inputIndex, int level )
{
	// Construct command string to set a link in the video router.  
	// This command will send video input channel "inputIndex" to 
	// video output channel "outputIndex" on level "level".
		// See http://www.sierravideo.com/generic.html for command format
		// for Sierra video routers.
	char commandString[256] = "";
	sprintf( commandString, "**X%d,%d,%d!!", outputIndex, inputIndex, level );

	// Write command string to Sierra video router via serial port.  
	// vrpn_write_characters returns # of chars written, or -1 on failure.
	int leng = strlen(commandString);
	if( vrpn_write_characters( serialfd,(unsigned char *)commandString,leng ) != leng ) {
		fprintf(stderr, "setLinkInSierraHardwareViaSerialPort: write to serial port FAILED.\n" );
	}
	else {
			//fprintf(stderr, "setLinkInSierraHardwareViaSerialPort: write to serial port SUCCEEDED.\n" );
	}
			//	printf( "Command string: %s\n", commandString );
}


/**************************************************************************************/
// Construct a command string to query the state of the Sierra video router.
// Then send this ASCII command to the Sierra video router via the serial port.  
// This will cause the video router to send back a block of characters
// describing its complete state.  
// This reply is received and parsed by receiveStatusOfSierraHardwareViaSerialPort().
void
queryStatusOfSierraHardwareViaSerialPort( void )
{
	// Construct command string.  
	char commandString[256];
	sprintf( commandString, "**S!!" );
		// See http://www.sierravideo.com/generic.html for command format
		// for Sierra video routers.
		// See discussion in vrpn_Router_Sierra_Server::mainloop() regarding
		// how many chars come back from this command (1600) and how long
		// it takes (1.6 sec).

	// Write command string to Sierra video router via serial port.  
	// vrpn_write_characters returns # of chars written, or -1 on failure.
	int leng = strlen(commandString);
	if( vrpn_write_characters( serialfd,(unsigned char *)commandString,leng ) != leng ) {
		fprintf(stderr, "queryStatusOfSierraHardwareViaSerialPort: write to serial port FAILED.\n" );
	}
	else {
			//fprintf(stderr, "queryStatusOfSierraHardwareViaSerialPort: write to serial port SUCCEEDED.\n" );
	}

	// Write one char (.) to the server screen for each time (every 10 sec)
	// the server queries the router hardware status.  
	printf( "." );
			//printf( "Command string: %s\n", commandString );
}


/**************************************************************************************/
// Process characters that come back to the server from the router hardware
// over the serial line.  These chars are passed one at a time to a routine
// who job is to find a complete message and then parse it.
void
receiveStatusOfSierraHardwareViaSerialPort( void )
{
	// Get characters sent from the Sierra video router on the serial port.
	unsigned char buffer[2];
	unsigned char * pBuffer = &buffer[0];
	while (vrpn_read_available_characters( serialfd, pBuffer, 1) != 0) {
		// Get the chars one at a time.
		char c = *pBuffer;
				//printf( "%c", c );

		// Pass the char to a routine who job is to find a 
		// complete message and then parse it.  
		// The messages are replies to commands sent to the router hardware 
		// by the server, and convey the state of the router hardware to the server.
		// The server's model of the state of the router hardware is stored 
		// in array router->router_state[][].
		router_Sierra_ReceiveChar( c );
	}
}


