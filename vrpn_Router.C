/**************************************************************************************/
// vrpn_Router.C
/**************************************************************************************/

#include "vrpn_Router.h"
#include <string.h>


/**************************************************************************************/
// Constructor for basic Router class (data and methods which are shared
// by both the server and client).  
// (eg, router state and channel name arrays.)
vrpn_Router::vrpn_Router (const char * name, vrpn_Connection * c) :
    vrpn_BaseClass(name, c)
{
	vrpn_BaseClass::init();		// calls vrpn_Router::register_types

	// Set the time to 0 just to have something there.
	timestamp.tv_usec = timestamp.tv_sec = 0;
        
        int i, j;
	router_state = new vrpn_int32*[vrpn_OUTPUT_CHANNEL_MAX];
        for (i = 0; i < vrpn_OUTPUT_CHANNEL_MAX; i++) {
            router_state[i] = new vrpn_int32[vrpn_LEVEL_MAX];
        }
        input_channel_name = new char**[vrpn_LEVEL_MAX];
        for (i = 0; i < vrpn_LEVEL_MAX; i++){
            input_channel_name[i] = new char*[vrpn_INPUT_CHANNEL_MAX];
	    for (j = 0; j < vrpn_INPUT_CHANNEL_MAX; j++){
                input_channel_name[i][j] = new char[vrpn_NAME_MAX];
            }
        }
	output_channel_name = new char**[vrpn_LEVEL_MAX];
	for (i = 0; i < vrpn_LEVEL_MAX; i++){
            output_channel_name[i] = new char*[vrpn_OUTPUT_CHANNEL_MAX];
            for (j = 0; j < vrpn_OUTPUT_CHANNEL_MAX; j++){
                output_channel_name[i][j] = new char[vrpn_NAME_MAX];
            }
        }



	// Clear the router state array.
	// Initialize all channels to vrpn_UNKNOWN_CHANNEL so that we know
	// the status is unknown until we get a message from some source
	// telling us the channel status.  
	// (The server gets messages from the router hardware;
	// the client gets messages from the server.)
	for (i = vrpn_OUTPUT_CHANNEL_MIN; i < vrpn_OUTPUT_CHANNEL_MAX; i++) {
		for( int level=vrpn_LEVEL_MIN; level<vrpn_LEVEL_MAX; level++ ) {
			router_state[i][level] = vrpn_UNKNOWN_CHANNEL;
		}
	}

	// Init input and output channel names to "???", indicating we don't know
	// any names yet for the channels.
	int level;
	for( level=vrpn_LEVEL_MIN; level<vrpn_LEVEL_MAX; level++ ) {
		for ( int output = vrpn_OUTPUT_CHANNEL_MIN; output < vrpn_OUTPUT_CHANNEL_MAX; output++) {
			strcpy( output_channel_name[ level ][ output ], "???" );	// was "???"
		}
	}
	for( level=vrpn_LEVEL_MIN; level<vrpn_LEVEL_MAX; level++ ) {
		for ( int input = vrpn_INPUT_CHANNEL_MIN; input < vrpn_INPUT_CHANNEL_MAX; input++) {
			strcpy( input_channel_name[ level ][ input ], "???" );	// was "???"
		}
	}
}

vrpn_Router::~vrpn_Router(void)
{
        int i, j;
        for (i = 0; i < vrpn_OUTPUT_CHANNEL_MAX; i++) {
            delete [] router_state[i];
        }
        delete [] router_state;

        for (i = 0; i < vrpn_LEVEL_MAX; i++){
            for (j = 0; j < vrpn_INPUT_CHANNEL_MAX; j++){
                delete [] input_channel_name[i][j];
            }
	    delete [] input_channel_name[i];
        }
        delete [] input_channel_name;

        for (i = 0; i < vrpn_LEVEL_MAX; i++){
            for (j = 0; j < vrpn_OUTPUT_CHANNEL_MAX; j++){
                delete [] output_channel_name[i][j];
            }
	    delete [] output_channel_name[i];
        }
	delete [] output_channel_name;

}

/**************************************************************************************/
// Define all message types used by router server or client.
// Called automagically by vrpn_BaseClass::init().
int vrpn_Router::register_types(void)
{
    if (d_connection == NULL) {return 0;}

	// Define message type: the router server sends a message to its clients
	// announcing that the router state has changed.
    change_m_id = d_connection->register_message_type("Router state update");
    if (change_m_id == -1) {
		fprintf(stderr,"vrpn_Router: Can't register type IDs\n");
		d_connection = NULL;
    }

	// Define message type: a client sends a command to the router server
	// to route a certain input channel to a certain output channel.
    set_link_m_id = d_connection->register_message_type("Set link command from client");
    if (set_link_m_id == -1) {
		fprintf(stderr,"vrpn_Router: Can't register type IDs\n");
		d_connection = NULL;
    }

    name_m_id = d_connection->register_message_type("Send channel name to client");
    if (name_m_id == -1) {
		fprintf(stderr,"vrpn_Router: Can't register type IDs\n");
		d_connection = NULL;
    }

    set_named_link_m_id = d_connection->register_message_type("Client sends pair of names to server");
    if (set_named_link_m_id == -1) {
		fprintf(stderr,"vrpn_Router: Can't register type IDs\n");
		d_connection = NULL;
    }

    return 0;
}


/**************************************************************************************/
// ENCODING AND DECODING MESSAGES
// FOR TRANSMISSION OVER VRPN CONNECTION
/**************************************************************************************/
// Encode (serialize for network transmission) a message of this form:
// (input_channel, output_channel, level).
vrpn_int32	vrpn_Router::encode_channel_status(char *buf, vrpn_int32 buflen, 
				vrpn_int32 output_ch, vrpn_int32 input_ch, vrpn_int32 level )
{
	vrpn_int32	buflensofar = buflen;

	// Message includes: vrpn_int32 routerNum, vrpn_float64 delta
	// We pack them with the delta first, so that everything is aligned
	// on the proper boundary.

	if (vrpn_buffer( &buf, &buflensofar, input_ch)) {
		fprintf(stderr,"vrpn_Router::encode_channel_status: Can't buffer input_ch\n");
		return -1;
	}
	if (vrpn_buffer( &buf, &buflensofar, output_ch)) {
		fprintf(stderr,"vrpn_Router::encode_channel_status: Can't buffer output_ch\n");
		return -1;
	}
	if (vrpn_buffer( &buf, &buflensofar, level)) {
		fprintf(stderr,"vrpn_Router::encode_channel_status: Can't buffer level\n");
		return -1;
	}

	return sizeof(vrpn_int32) + sizeof(vrpn_int32) + sizeof(vrpn_int32);
}


/**************************************************************************************/
// Decode (reconstruct from sequence of bytes received over network) 
// a message of this form: (input_channel, output_channel, level).
vrpn_int32
decode_change_message( vrpn_HANDLERPARAM p, vrpn_ROUTERCB & cp )
{
	// Get ptr to the payload chars received by VRPN for this message.  
	const char	*bufptr = p.buffer;

	// Unpack the message from the series of chars into the
	// data structure for this message type.  
	cp.msg_time = p.msg_time;
	vrpn_unbuffer(&bufptr, &cp.input_channel);
	vrpn_unbuffer(&bufptr, &cp.output_channel);
	vrpn_unbuffer(&bufptr, &cp.level);

	return 0;
}


/**************************************************************************************/
// Encode (serialize for network transmission) a message of this form:
// (channel_type, channel_number, level_number, name_string).
vrpn_int32	vrpn_Router::encode_name(char *buf, vrpn_int32 buflen, 
			vrpn_int32 channelType, vrpn_int32 channelNum, vrpn_int32 levelNum, char *name )
{
	vrpn_int32	buflensofar = buflen;

	if (vrpn_buffer( &buf, &buflensofar, channelType)) {
		fprintf(stderr,"vrpn_Router::encode_name: Can't buffer channelType\n");
		return -1;
	}
	if (vrpn_buffer( &buf, &buflensofar, channelNum)) {
		fprintf(stderr,"vrpn_Router::encode_name: Can't buffer channelNum\n");
		return -1;
	}
	if (vrpn_buffer( &buf, &buflensofar, levelNum)) {
		fprintf(stderr,"vrpn_Router::encode_name: Can't buffer levelNum\n");
		return -1;
	}

	// Count length of name string.
	int nameLength = strlen( name );

	// Send length 
	if (vrpn_buffer( &buf, &buflensofar, nameLength)) {
		fprintf(stderr,"vrpn_Router::encode_name: Can't buffer nameLength\n");
		return -1;
	}

	// Send out "nameLength" chars out connection.
	for( int i=0; i<nameLength; i++ ) {
		char c = name[i];
		vrpn_int8 nameChar = c;
		if (vrpn_buffer( &buf, &buflensofar, nameChar)) {
			fprintf(stderr,"vrpn_Router::encode_name: Can't buffer nameChar\n");
			return -1;
		}
	}

	// We sent 4 ints + nameLength chars.
	return 4 * sizeof(vrpn_int32) + nameLength * sizeof(vrpn_int8);
}


/**************************************************************************************/
// Decode (reconstruct from sequence of bytes received over network) 
// a message of this form: 
// (channel_type, channel_number, level_number, name_string).
vrpn_int32
decode_name_message( vrpn_HANDLERPARAM p, vrpn_ROUTERNAMECB & cp )
{
	// Get ptr to the payload chars received by VRPN for this message.  
	const char	*bufptr = p.buffer;

	// Unpack the message from the series of chars into the
	// data structure for this message type.  
		//	struct timeval	msg_time;			// Timestamp when change happened
		//	vrpn_int32	channelType;
		//	vrpn_int32	channelNum;
		//	vrpn_int32	levelNum;
		//	vrpn_int32	nameLength;
		//	vrpn_int8	name[vrpn_NAME_MAX];

	cp.msg_time = p.msg_time;
	vrpn_unbuffer(&bufptr, &cp.channelType);
	vrpn_unbuffer(&bufptr, &cp.channelNum);
	vrpn_unbuffer(&bufptr, &cp.levelNum);
	vrpn_unbuffer(&bufptr, &cp.nameLength);

	if( cp.nameLength >= vrpn_NAME_MAX ) {
		fprintf( stderr, "vrpn_Router_Remote::handle_name_message: name too long.\n" );
		return 1;
	}

	// Clear whole name buffer.
	for( int i=0; i<vrpn_NAME_MAX; i++ )   cp.name[i] = 0;

	// Copy in name string.
	for( int j=0; j<cp.nameLength; j++ ) {
		vrpn_unbuffer(&bufptr, &cp.name[j] );
	}

	return 0;
}


/**************************************************************************************/
// SENDING MESSAGES
// MESSAGES SENT BY CLIENT
/**************************************************************************************/

/**************************************************************************************/
// Send a message from the client side to the server:
// a command to link the given input channel to the given output channel.
void vrpn_Router::set_link( vrpn_int32 output_ch, vrpn_int32 input_ch, vrpn_int32 level ) 
{
	char	msgbuf[1000];
	vrpn_int32 len;

	if (d_connection) {
		len = encode_channel_status(msgbuf, sizeof(msgbuf), output_ch, input_ch, level );
		if (d_connection->pack_message(len, timestamp,
				set_link_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      		fprintf(stderr,"vrpn_Router: can't write message: tossing\n");
      	}
	}
}


/**************************************************************************************/
// Query the complete state of the router.  
void vrpn_Router::query_complete_state( void ) 
{
	// An all-zeroes set_link command sent from the client to the server
	// is interpreted as a status query command.  
	set_link( 0, 0, 0 );
}


/**************************************************************************************/
void vrpn_Router::send_name_to_server ( 
				vrpn_int32 channelType, vrpn_int32 channelNum, vrpn_int32 levelNum, char* name )
{
	char	msgbuf[1000];

	// Encode the message VRPN style.
	vrpn_int32 len = encode_name( msgbuf, sizeof(msgbuf), 
								  channelType, channelNum, levelNum, name );

	// Send the message out the VRPN connection.
	if (d_connection->pack_message(len, timestamp,
				set_named_link_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      	fprintf(stderr,"vrpn_Router: can't write message: tossing\n");
    }
}


/**************************************************************************************/
// Send a command from the router client to router server
// containing the names of the two router channels
// (one an input and one an output) to be linked.  
// The names are defined in the configuration file "router.config"
// that lives on the server.  
void
vrpn_Router::set_router(  const char* channelName1, const char* channelName2 )
{
	int leng1 = strlen( channelName1 );
	int leng2 = strlen( channelName2 );
	int cmdStringLeng = leng1 + leng2 + 1;
	if( cmdStringLeng >= vrpn_NAME_MAX ) {
		printf( "set_router: channelNames too long.\n" );
		return;
	}
	
	// Catenate the names together with a colon in the middle.
	char buf[vrpn_NAME_MAX];
	strcpy( &buf[0],         channelName1 );
	buf[ leng1 ] = ':';
	strcpy( &buf[leng1 + 1], channelName2 );
	
	char* commandString = &buf[0];
		//char* commandString = "coons_ch1:hmdlab_monitor_left";

	// These args are not used on the other end, but give them definite values.
	int n1 = 0;
	int n2 = 0;
	int n3 = 0;

	// Send command string containing input and output channel names
	// (eg "coons_ch1:hmdlab_monitor_left") to server.
	send_name_to_server( n1, n2, n3, commandString );
}


/**************************************************************************************/
// SENDING MESSAGES
// MESSAGES SENT BY SERVER
/**************************************************************************************/
// Send a report on the status of a single channel to the client.
void vrpn_Router::report_channel_status ( 
				vrpn_int32 input_ch, vrpn_int32 output_ch, vrpn_int32 level )
{
	char	msgbuf[1000];

	// Encode the message VRPN style.
	vrpn_int32 len = encode_channel_status(msgbuf, sizeof(msgbuf), output_ch, input_ch, level );

	// Send the message out the VRPN connection.
	if (d_connection->pack_message(len, timestamp,
				change_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      	fprintf(stderr,"vrpn_Router: can't write message: tossing\n");
    }
}


/**************************************************************************************/
// Report the complete state of the router to the client.
// Send a series of report_channel_status() messages to the client.
// Each messages describes the state of one output channel in the router.
void vrpn_Router::report(void)
{
	vrpn_int32 output;

	if (d_connection) {
		for( int level=1; level<vrpn_LEVEL_MAX; level++ ) {
			// loop through all output channels.
			for (output = 0; output < vrpn_OUTPUT_CHANNEL_MAX; output++) {
				// arg order: input, output, level
				report_channel_status( router_state[output][level], output, level );
			}
		}
	}
}


/**************************************************************************************/
void vrpn_Router::send_name_to_client ( 
				vrpn_int32 channelType, vrpn_int32 channelNum, vrpn_int32 levelNum, char* name )
{
	char	msgbuf[1000];

	// Encode the message VRPN style.
	vrpn_int32 len = encode_name( msgbuf, sizeof(msgbuf), 
								  channelType, channelNum, levelNum, name );

	// Send the message out the VRPN connection.
	if (d_connection->pack_message(len, timestamp,
				name_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      	fprintf(stderr,"vrpn_Router: can't write message: tossing\n");
    }
}


/**************************************************************************************/
void vrpn_Router::send_all_names_to_client( void )
{
	// Send all input channel names from server to client(s).
	int levelNum;
	for( levelNum=vrpn_LEVEL_MIN; levelNum<vrpn_LEVEL_MAX; levelNum++ ) {
		for ( int input = vrpn_INPUT_CHANNEL_MIN; input < vrpn_INPUT_CHANNEL_MAX; input++) {
			int channelType = vrpn_CHANNEL_TYPE_INPUT;
			int channelNum = input;
//			char buf[256];
//			sprintf( buf, "In%d-%d", channelNum, levelNum );  // eg "In32-1"
//			char* name = &buf[0];

			char* name = input_channel_name[ levelNum ][ channelNum ];

			send_name_to_client( channelType, channelNum, levelNum, name );
		}
	}

	// Send all output channel names from server to client(s).
	for( levelNum=vrpn_LEVEL_MIN; levelNum<vrpn_LEVEL_MAX; levelNum++ ) {
		for ( int output = vrpn_OUTPUT_CHANNEL_MIN; output < vrpn_OUTPUT_CHANNEL_MAX; output++) {
			int channelType = vrpn_CHANNEL_TYPE_OUTPUT;
			int channelNum = output;
//			char buf[256];
//			sprintf( buf, "Out%d-%d", channelNum, levelNum );  // eg "Out32-1"
//			char* name = &buf[0];


			char* name = output_channel_name[ levelNum ][ channelNum ];

			send_name_to_client( channelType, channelNum, levelNum, name );
		}
	}
}


/**************************************************************************************/
// ROUTINES TO SET CHANNEL NAMES
/**************************************************************************************/
void
vrpn_Router::setInputChannelName( int levelNum, int channelNum, const char* channelNameCStr )
{
	const int minInputChannel  = vrpn_INPUT_CHANNEL_MIN;	// lowest channel #
	const int maxInputChannel  = vrpn_INPUT_CHANNEL_MAX;	// one past highest channel #
//	const int minOutputChannel = vrpn_OUTPUT_CHANNEL_MIN;	// lowest channel #
//	const int maxOutputChannel = vrpn_OUTPUT_CHANNEL_MAX;	// one past highest channel #
	const int minLevel         = vrpn_LEVEL_MIN;			// lowest level #
	const int maxLevel         = vrpn_LEVEL_MAX;			// one past highest level #

	// Check the ranges of arguments.
	int input_ch = channelNum;
	if( input_ch < minInputChannel  ||  input_ch >= maxInputChannel ) {
		printf(	"\nError: input channel (%d) out of range [%d,%d]\n", 
				input_ch, minInputChannel, maxInputChannel-1 );
		return;
	}

	int level = levelNum;
	if( level < minLevel  ||  level >= maxLevel ) {
		printf(	"\nError: level (%d) out of range [%d,%d]\n", 
				level, minLevel, maxLevel-1 );
		return;
	}
	
	
	// Get ptr into channel name table for this entry.
	char * inputChannelName = 
			        input_channel_name[ levelNum ][ channelNum ];

	// Copy the channel name from the config file into the name table.
	strncpy( inputChannelName, channelNameCStr, vrpn_NAME_MAX );
	inputChannelName[ vrpn_NAME_MAX - 1 ] = 0;   // make sure of zero termination
}


/**************************************************************************************/
void
vrpn_Router::setOutputChannelName( int levelNum, int channelNum, const char* channelNameCStr )
{
//	const int minInputChannel  = vrpn_INPUT_CHANNEL_MIN;	// lowest channel #
//	const int maxInputChannel  = vrpn_INPUT_CHANNEL_MAX;	// one past highest channel #
	const int minOutputChannel = vrpn_OUTPUT_CHANNEL_MIN;	// lowest channel #
	const int maxOutputChannel = vrpn_OUTPUT_CHANNEL_MAX;	// one past highest channel #
	const int minLevel         = vrpn_LEVEL_MIN;			// lowest level #
	const int maxLevel         = vrpn_LEVEL_MAX;			// one past highest level #
	
	// Check the ranges of arguments.
	int output_ch = channelNum;
	if( output_ch < minOutputChannel  ||  output_ch >= maxOutputChannel ) {
		printf(	"\nError: output channel (%d) out of range [%d,%d]\n", 
				output_ch, minOutputChannel, maxOutputChannel-1 );
		return;
	}
	int level = levelNum;
	if( level < minLevel  ||  level >= maxLevel ) {
		printf(	"\nError: level (%d) out of range [%d,%d]\n", 
				level, minLevel, maxLevel-1 );
		return;
	}
	
	
	// Get ptr into channel name table for this entry.
	char * outputChannelName = 
			        output_channel_name[ levelNum ][ channelNum ];

	// Copy the channel name from the config file into the name table.
	strncpy( outputChannelName, channelNameCStr, vrpn_NAME_MAX );
	outputChannelName[ vrpn_NAME_MAX - 1 ] = 0;   // make sure of zero termination
}


/**************************************************************************************/
// ************* CLIENT ROUTINES ****************************

/**************************************************************************************/
// Router Client constructor.
// Initialize router data structures (via call to vrpn_Router)
// and register callbacks for messages client understands:
//     change_message -- a report from the server that an output channel
//         has changed its status (ie a new input is now linked to that channel).
//     name_message -- a message from the server giving a channel name string
//         to associate with a physical channel number.   
vrpn_Router_Remote::vrpn_Router_Remote (const char * name,
                                        vrpn_Connection * c) : 
	vrpn_Router( name, c )
	,change_list( NULL )
	,name_list( NULL )
{
	// Register a handler for the change callback from this device,
	// if we got a connection.
	if (d_connection) {
	  if (register_autodeleted_handler(change_m_id, handle_change_message,
	    this, d_sender_id)) {
		fprintf(stderr,"vrpn_Router_Remote: can't register handler\n");
		d_connection = NULL;
	  }
	} else {
		fprintf(stderr,"vrpn_Router_Remote: Can't get connection!\n");
	}

	// Register a handler for the name callback from this device,
	// if we got a connection.
	if (d_connection) {
	  if (register_autodeleted_handler(name_m_id, handle_name_message,
	    this, d_sender_id)) {
		fprintf(stderr,"vrpn_Router_Remote: can't register handler\n");
		d_connection = NULL;
	  }
	} else {
		fprintf(stderr,"vrpn_Router_Remote: Can't get connection!\n");
	}


	gettimeofday(&timestamp, NULL);
}


/**************************************************************************************/
vrpn_Router_Remote::~vrpn_Router_Remote()
{
}


/**************************************************************************************/
// This routine gets called every interation of the client's main loop.  
// It enabled VRPN messages to be received and sent.  
void	vrpn_Router_Remote::mainloop()
{
  if (d_connection) { 
    d_connection->mainloop(); 
  }
}


/**************************************************************************************/
// REGISTER, UNREGISTER, AND HANDLE CALLBACKS
// FOR MESSAGES RECEIVED BY CLIENT.
// namely:
//     change_messages
//     name_messages
/**************************************************************************************/



#if 0
// SINGLE CALLBACK CHANGE MESSAGE CODE


/**************************************************************************************/
int vrpn_Router_Remote::register_change_handler(void *userdata,
			vrpn_ROUTERCHANGEHANDLER handler)
{
	// Store a pointer to the desired handler function for this 
	// message type.  Also save a word of data (userdata) to be
	// passed on when the handler is invoked.  
	change_block.handler  = handler;
	change_block.userdata = userdata;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::unregister_change_handler(void *userdata,
			vrpn_ROUTERCHANGEHANDLER handler)
{
	// Force the ptr to the callback function for this message type
	// to be null.
	change_block.handler  = NULL;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	// Only static member functions can be passed as pointers,
	// so the "this" pointer is passed in "userdata".  
	// We can't call it "this" (reserved keyword), so we call it "me".
	// Get ptr to the data block holding the callback function and its data.
	vrpn_Router_Remote *me = (vrpn_Router_Remote *)userdata;
	vrpn_ROUTERCHANGELIST *handlerBlockPtr = &(me->change_block);


	// Unpack the message from the series of chars into the
	// data structure for this message type.  
	vrpn_ROUTERCB	cp;
	decode_change_message( p, cp );

	// Call callback, if one was registered.
	// Fill in the parameters and call it.
	if( handlerBlockPtr->handler ) {
		handlerBlockPtr->handler( handlerBlockPtr->userdata, cp );
	}

	return 0;
}



#else
// OLD (SAVED) CHANGE MESSAGE CALLBACK CODE


/**************************************************************************************/
int vrpn_Router_Remote::register_change_handler(void *userdata,
			vrpn_ROUTERCHANGEHANDLER handler)
{
	vrpn_ROUTERCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_ROUTER_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_ROUTERCHANGELIST) == NULL) {
		fprintf(stderr,
			"vrpn_Router_Remote::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = change_list;
	change_list = new_entry;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::unregister_change_handler(void *userdata,
			vrpn_ROUTERCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_ROUTERCHANGELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &change_list;
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
		snitch = &( (*snitch)->next );
		victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		   "vrpn_Router_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	// Only static member functions can be passed as pointers,
	// so the "this" pointer is passed in "userdata".  
	// We can't call it "this" (reserved keyword), so we call it "me".
	// Get ptr to the data block holding the callback function and its data.
	vrpn_Router_Remote* me = (vrpn_Router_Remote *)userdata;
	vrpn_ROUTERCHANGELIST* handlerBlockPtr = me->change_list;


	// Unpack the message from the series of chars into the
	// data structure for this message type.  
	vrpn_ROUTERCB	cp;
	decode_change_message( p, cp );
		//	const char	*bufptr = p.buffer;
		//	cp.msg_time = p.msg_time;
		//	vrpn_unbuffer(&bufptr, &cp.input_channel);
		//	vrpn_unbuffer(&bufptr, &cp.output_channel);


	// Go down the list of callbacks that have been registered.
	// Fill in the parameters and call each.
	while (handlerBlockPtr != NULL) {
		handlerBlockPtr->handler(handlerBlockPtr->userdata, cp);
		handlerBlockPtr = handlerBlockPtr->next;
	}

	return 0;
}

#endif





/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/



#if 0
// SINGLE CALLBACK NAME MESSAGE CODE


/**************************************************************************************/
int vrpn_Router_Remote::register_name_handler(void *userdata,
			vrpn_ROUTERNAMEHANDLER handler)
{
	// Store a pointer to the desired handler function for this 
	// message type.  Also save a word of data (userdata) to be
	// passed on when the handler is invoked.  
	name_block.handler  = handler;
	name_block.userdata = userdata;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::unregister_name_handler(void *userdata,
			vrpn_ROUTERNAMEHANDLER handler)
{
	// Force the ptr to the callback function for this message type
	// to be null.
	name_block.handler  = NULL;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::handle_name_message(void *userdata, vrpn_HANDLERPARAM p)
{
	// Get data for this message type into a struct.
	vrpn_ROUTERNAMECB	cp;
	decode_name_message( p, cp );

	
	// Only static member functions can be passed as pointers,
	// so the "this" pointer is passed in "userdata".  
	// We can't call it "this" (reserved keyword), so we call it "me".
	// Get ptr to the data block holding the callback function and its data.
	vrpn_Router_Remote *me = (vrpn_Router_Remote *)userdata;
	vrpn_ROUTERNAMELIST *handlerBlockPtr = &(me->name_block);

	// Call callback, if one was registered.
	// Fill in the parameters and call it.
	if( handlerBlockPtr->handler ) {
		handlerBlockPtr->handler( handlerBlockPtr->userdata, cp );
	}

	return 0;
}




#else
// NEW NAME MESSAGE CALLBACK CODE



/**************************************************************************************/
int vrpn_Router_Remote::register_name_handler(void *userdata,
			vrpn_ROUTERNAMEHANDLER handler)
{
	vrpn_ROUTERNAMELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_ROUTER_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_ROUTERNAMELIST) == NULL) {
		fprintf(stderr,
			"vrpn_Router_Remote::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = name_list;
	name_list = new_entry;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::unregister_name_handler(void *userdata,
			vrpn_ROUTERNAMEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_ROUTERNAMELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &name_list;
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
		snitch = &( (*snitch)->next );
		victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		   "vrpn_Router_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


/**************************************************************************************/
int vrpn_Router_Remote::handle_name_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	// Only static member functions can be passed as pointers,
	// so the "this" pointer is passed in "userdata".  
	// We can't call it "this" (reserved keyword), so we call it "me".
	// Get ptr to the data block holding the callback function and its data.
	vrpn_Router_Remote* me = (vrpn_Router_Remote *)userdata;
	vrpn_ROUTERNAMELIST* handlerBlockPtr = me->name_list;


	// Unpack the message from the series of chars into the
	// data structure for this message type.  
	vrpn_ROUTERNAMECB	cp;
	decode_name_message( p, cp );


	// Go down the list of callbacks that have been registered.
	// Fill in the parameters and call each.
	while (handlerBlockPtr != NULL) {
		handlerBlockPtr->handler(handlerBlockPtr->userdata, cp);
		handlerBlockPtr = handlerBlockPtr->next;
	}

	return 0;
}

#endif




