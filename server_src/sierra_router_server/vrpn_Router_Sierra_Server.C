/**************************************************************************************/
// vrpn_Router_Sierra_Server.C
/**************************************************************************************/
#include "vrpn_Router.h"
#include "vrpn_Serial.h"
#include "vrpn_Router_Sierra_Server.h"

extern void readAndParseRouterConfigFile( void );



/**************************************************************************************/
// Update the server's model of the router state with information
// received from the router hardware.  If the state changed,
// send a message to the client.
void
router_Sierra_SetState( int inputChannel, int outputChannel, int level )
{
	// Test whether channel status received from the router hardware is 
	// already known to the server.
	// If is different than what we expected, the router changed state.
	int oldRouterState = router->router_state[ outputChannel ][ level ];
	if( inputChannel != oldRouterState ) {


		if( oldRouterState == vrpn_UNKNOWN_CHANNEL ) {
			printf( "." );
		}
		else {
			printf( "Level %2d: output %2d changed from %2d to %2d\n", 
						level, outputChannel, oldRouterState, inputChannel );
		}

		// Set the server's model of the router state to reflect this new info.
		router->router_state[ outputChannel ][ level ] = inputChannel;

		// Send update on this channel to client.
		// Note that although the server queries the hardware status every 10 seconds
		// and receives many redundant status updates over the serial line,
		// it only sends a change message to the client when there *is*
		// a change.  (Or when it first receives the status of a channel.)
		router->report_channel_status( inputChannel, outputChannel, level );
	}
	else {
			//printf( "." );
	}
}


/**************************************************************************************/
// ************* SERVER ROUTINES ****************************

/**************************************************************************************/
// Test 2 character strings for equality.  
int
streq( char* str1, char* str2 )
{
	// strcmp does lexicographic comparison.  Result=0 means strings are the same.
	return   (strcmp( str1, str2 ) == 0);
}


/**************************************************************************************/
// Search through the input and output channel names tables for a match
// with the string "name".  If a match is found, return with
// the channel type (input or output), channel number, and level number
// of matching channel name.  
void
searchNameTables( char* name, int* pMatch, 
				 int* pChannelType, int* pChannelNum, int* pLevelNum )
{
	// Search all input channel names in server's name table.
	int levelNum;
	for( levelNum=vrpn_LEVEL_MIN; levelNum<vrpn_LEVEL_MAX; levelNum++ ) {
		for ( int input = vrpn_INPUT_CHANNEL_MIN; input < vrpn_INPUT_CHANNEL_MAX; input++) {
			int channelType = vrpn_CHANNEL_TYPE_INPUT;
			int channelNum = input;
			char* input_ch_name = router->input_channel_name[ levelNum ][ channelNum ];

			if( streq( name, input_ch_name ) ) {
				// Found match.  Set identifying values and return.
				*pMatch = 1;
				*pChannelType = vrpn_CHANNEL_TYPE_INPUT;
				*pChannelNum  = channelNum;
				*pLevelNum    = levelNum;
				return;
			}
		}
	}

	// Search all output channel names in server's name table.
	for( levelNum=vrpn_LEVEL_MIN; levelNum<vrpn_LEVEL_MAX; levelNum++ ) {
		for ( int output = vrpn_OUTPUT_CHANNEL_MIN; output < vrpn_OUTPUT_CHANNEL_MAX; output++) {
			int channelType = vrpn_CHANNEL_TYPE_OUTPUT;
			int channelNum = output;
			char* output_ch_name = router->output_channel_name[ levelNum ][ channelNum ];

			if( streq( name, output_ch_name ) ) {
				// Found match.  Set identifying values and return.
				*pMatch = 1;
				*pChannelType = vrpn_CHANNEL_TYPE_OUTPUT;
				*pChannelNum  = channelNum;
				*pLevelNum    = levelNum;
				return;
			}
		}
	}

	// No match in all name tables.
	*pMatch = 0;
	return;
}


/**************************************************************************************/
// Extract the two channel names (separated by colon) from the command string. 
// Try to find these names in the channel name tables and convert
// them to physical channel numbers and level number.
int 
parseSetNamedLinkCommand( char* commandString, int* input_ch, int* output_ch, int* level )
{
	// Find the colon in the command string and extract the two substrings
	// (channel names) on either side.
	// First make copy of command string.
	char buf[vrpn_NAME_MAX];
	strncpy( buf, commandString, vrpn_NAME_MAX );
	buf[ vrpn_NAME_MAX - 1 ] = 0;

	// Search through string for colon.
	// If not found, return with error code.
	// If found, replace colon with zero (null) char
	// and set pointers to the starts of the two strings.
	char* name1 = 0;
	char* name2 = 0;
	for( int i=0; buf[i] != 0; i++ ) {
		if( buf[i] == ':' ) {
			// First string goes from start of command string to colon (now zero terminator).
			name1 = &buf[0];
			buf[i] = 0;

			// Second string goes from right after colon to end of original command string.
			// (If colon was last char, this works: string 2 is empty string.)
			name2 = &buf[i+1];
			goto foundColon;
		}
	}
	// failed to find colon in command string.
	printf( "Error: failed to find colon in command string.\n" );
	return 1;

foundColon:
			//printf( "str1:%s, str2:%s\n", name1, name2 );
	// Successfully broke the command string into two name strings (name1, name2).
	// Now try to match these name strings with names in channel name tables.

	// Search for name1 in name tables.
	int match1;
	int channelType1;
	int channelNum1;
	int levelNum1;
	char ioname[2][100] = { "output", "input" };
	searchNameTables( name1, &match1, &channelType1, &channelNum1, &levelNum1 );
	if( match1 ) {
			//printf( "%s is %s_ch%d_level%d\n", name1, ioname[channelType1], channelNum1, levelNum1 );
	}
	else {
		printf( "Error: failed to find \"%s\" in name tables.\n", name1 );
		return 1;
	}

	// Search for name2 in name tables.
	int match2;
	int channelType2;
	int channelNum2;
	int levelNum2;
	searchNameTables( name2, &match2, &channelType2, &channelNum2, &levelNum2 );
	if( match2 ) {
			//printf( "%s is %s_ch%d_level%d\n", name2, ioname[channelType2], channelNum2, levelNum2 );
	}
	else {
		printf( "Error: failed to find \"%s\" in name tables.\n", name2 );
		return 1;
	}

	// Make sure we have one intput and one output.
	if(      channelType1==vrpn_CHANNEL_TYPE_INPUT  && channelType2==vrpn_CHANNEL_TYPE_OUTPUT ) {
			//printf( "Channel types are compatible.\n" );
		*input_ch  = channelNum1;
		*output_ch = channelNum2;

	}
	else if( channelType1==vrpn_CHANNEL_TYPE_OUTPUT && channelType2==vrpn_CHANNEL_TYPE_INPUT  ) {
			//printf( "Channel types are compatible.\n" );
		*input_ch  = channelNum2;
		*output_ch = channelNum1;
	}
	else {
		printf( "Error: need one each of input and output channel names.\n" );
		return 1;
	}


	// Make sure input and output are on the same level.
	if( levelNum1 == levelNum2 ) {
			//printf( "OK: both on same level.\n" );
		*level     = levelNum1;
	}
	else {
		printf( "Error: named input and output channels are on different levels.\n" );
		return 1;
	}
	
	// Everything worked.  Return input channel number, output channel number,
	// and level number corresponding to the two named channels in command string.
	printf( "Linking %s to %s: input #%d:output #%d (level #%d)\n", 
					name1, name2, *input_ch, *output_ch, *level );
	return 0;	// 0 means no error in parsing
}


/**************************************************************************************/
// Callback routine which is called when the server receives a "set_named_link" command
// from the client over the VRPN connection.  
// This message is sent by the C++ or Unix-script interfaces to the server,
// using text names to set a link between an intput and an output channel.
int vrpn_Router_Sierra_Server::vrpn_handle_set_named_link_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	// Get the "this" pointer passed in userdata.
	// Callback functions must be static member functions, and therefore
	// can carry along no "this" pointer, but it can be carried along through
	// the convention that "this" (which was specified when the handler
	// was registered, eg, in the server's constructor) is saved and
	// then passed as an argument (userdata) to the handler function.  
	vrpn_Router_Sierra_Server *me = (vrpn_Router_Sierra_Server *)userdata;
			//Example use: me->report();
	
	
	// Point to start of buffer of received raw data.
	const char	*bufptr = p.buffer;

	// Unpack data into data structure for this message type.
	vrpn_ROUTERNAMECB	cp;
	decode_name_message( p, cp );
	int channelType = cp.channelType;
	int channelNum  = cp.channelNum;
	int levelNum    = cp.levelNum;
	int nameLength  = cp.nameLength;
	char* commandString = &cp.name[0];	// only using name field
			//	printf( "Received Set Named Link command, name: %s\n", commandString );


	// Extract the two channel names (separated by colon) from the command string. 
	// Try to find these names in the channel name tables and convert
	// them to physical channel numbers and level number.
	int input_ch  = 0;
	int output_ch = 0;
	int level     = 0;
	if( parseSetNamedLinkCommand( commandString, &input_ch, &output_ch, &level ) ) {
		printf( "vrpn_handle_set_named_link_message: parse of command string failed.\n" );
		return 0;
	}

	
	// This is a set_link command: link input channel to output channel.
	// Send command to router hardware based on received command.
	setLinkInSierraHardwareViaSerialPort( output_ch, input_ch, level );
			// After executing an "X" command to set a link, the Sierra router
			// sends back a short status message confirming the link was set.
			// Therefore no query (such as queryStatusOfSierraHardwareViaSerialPort()) need be sent
			// to the router hardware.

	return 0;
}


/**************************************************************************************/
// Callback routine which is called when the server receives a "set link" command
// from the client over the VRPN connection.  
// This handler will
// * be called when a command is sent by the client to change the router state
// * talk to the router hardware over a serial line to make the state change
// * send out a message to all clients with the new router state
//       (so all clients will get updated with new state info).
int vrpn_Router_Sierra_Server::vrpn_handle_set_link_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	// Get the "this" pointer passed in userdata.
	// Callback functions must be static member functions, and therefore
	// can carry along no "this" pointer, but it can be carried along through
	// the convention that "this" (which was specified when the handler
	// was registered, eg, in the server's constructor) is saved and
	// then passed as an argument (userdata) to the handler function.  
	vrpn_Router_Sierra_Server *me = (vrpn_Router_Sierra_Server *)userdata;
			//Example use: me->report();
	
	
	// Unpack data into data structure.
	vrpn_ROUTERCB	cp;
	decode_change_message( p, cp );

	// Decode message from client 
	int input_ch  = cp.input_channel;
	int output_ch = cp.output_channel;
	int level     = cp.level;


	// Which type of message is it?
	if( input_ch==0  &&  output_ch==0  &&  level==0 ) {
		// An all zeros set_link message is a status query from the client.
		// The GUI-based client sends this message to the server when it starts up,
		// because it needs the router state and channel name info.  
		printf( "Received Status Query.  " );
		printf( "Sending complete state and all channel names to clients.\n" );

		// Report the complete state of the router to the client.  
		// The information transmitted is the server's model of the 
		// router hardware state: the array router->router_state[][].   
		// Since the server queries the router hardware periodically
		// to detect state changes, this model should be accurate.  
		router->report();

		// Send names of all input and output channels to the client.  
		router->send_all_names_to_client();
	}
	else {
		// This is a normal set_link command: link input channel to output channel.

		// Send command to router hardware based on received command.
		setLinkInSierraHardwareViaSerialPort( output_ch, input_ch, level );
			// After executing an "X" command to set a link, the Sierra router
			// sends back a short status message confirming the link was set.
			// Therefore no query (such as queryStatusOfSierraHardwareViaSerialPort()) need be sent
			// to the router hardware.
	}

	return 0;
}


/**************************************************************************************/
void
router_setInputChannelName(  int levelNum, int channelNum, const char* channelNameCStr )
{
	router->setInputChannelName( levelNum, channelNum, channelNameCStr );
}


/**************************************************************************************/
void
router_setOutputChannelName( int levelNum, int channelNum, const char* channelNameCStr )
{
	router->setOutputChannelName( levelNum, channelNum, channelNameCStr );
}



