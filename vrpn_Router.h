/**************************************************************************************/
// vrpn_Router.h
// This implements a Router class, for controlling a video (or audio) router
// using VRPN (Virtual Reality Peripheral Network).  
// A router has a set of inputs and a set of outputs, and can link
// particular inputs to particular outputs when commanded to do so.  
// Clients send messages over VRPN connections to set the state of a server
// which deals directly with the video router hardware.  
// The server sends change messages to the clients when the state of the
// video router changes.
//
// July 2000, Initial version, by Warren Robinett, UNC Computer Science Dept.
// Derived from Dial.h and Dial.C example in the VRPN source.
// See http://www.cs.unc.edu/research/vrpn for documentation on VRPN.
/**************************************************************************************/

#ifndef VRPN_ROUTER
#define VRPN_ROUTER

#include "vrpn_Connection.h"
#include "vrpn_BaseClass.h"

#define vrpn_INPUT_CHANNEL_MIN	 1	// allow for 32 channels, numbered 1 to 32
#define vrpn_INPUT_CHANNEL_MAX	33	// allow for 32 channels, numbered 1 to 32
#define vrpn_OUTPUT_CHANNEL_MIN	 1	// allow for 32 channels, numbered 1 to 32
#define vrpn_OUTPUT_CHANNEL_MAX	33	// allow for 32 channels, numbered 1 to 32
#define vrpn_LEVEL_MIN			 1	// allow for 5 levels, numbered 1 to 5
#define vrpn_LEVEL_MAX			 6	// allow for 5 levels, numbered 1 to 5
#define vrpn_NAME_MAX		   256	// max name length (including zero terminator)
#define vrpn_CHANNEL_TYPE_OUTPUT 0
#define vrpn_CHANNEL_TYPE_INPUT  1
#define vrpn_UNKNOWN_CHANNEL	 0



/**************************************************************************************/
//************** Users deal with the following *************

// User routine to handle a change in router state (ie which inputs
// ar linked to which outputs). The same callback data structure
// vrpn_ROUTERCB = (input_channel, output_channel, timestamp)
// is used for both commands from the client to the server
// to set the router state (set_link message) and change update
// messages from the server to its clients.  When announcing a new
// router state, one change message is sent by the server for each router
// output channel.  

typedef	struct _vrpn_ROUTERCB {
	struct timeval	msg_time;			// Timestamp when change happened
	vrpn_int32	input_channel;			// Input channel #
	vrpn_int32	output_channel;			// Output channel #
	vrpn_int32	level;					// Level of router
} vrpn_ROUTERCB;
typedef void (*vrpn_ROUTERCHANGEHANDLER) (void * userdata, const vrpn_ROUTERCB info);


typedef	struct _vrpn_ROUTERNAMECB {
	struct timeval	msg_time;			// Timestamp when change happened
	vrpn_int32	channelType;
	vrpn_int32	channelNum;
	vrpn_int32	levelNum;
	vrpn_int32	nameLength;
	vrpn_int8	name[vrpn_NAME_MAX];
} vrpn_ROUTERNAMECB;
typedef void (*vrpn_ROUTERNAMEHANDLER) (void * userdata, const vrpn_ROUTERNAMECB info);



/**************************************************************************************/
class vrpn_Router : public vrpn_BaseClass {
public:
	vrpn_Router( const char * name, vrpn_Connection * c = NULL);
        ~vrpn_Router();
	virtual void set_router( const char* channelName1, 
		                     const char* channelName2 );//client sends names of 2 channels to link
	virtual void set_link( vrpn_int32 output_ch, 
			vrpn_int32 input_ch, vrpn_int32 level ); // send cmd to server to link input to output channel
	virtual void query_complete_state( void );  // request full state be sent to client
    virtual void report_channel_status( vrpn_int32 input_ch, vrpn_int32 output_ch, 
				vrpn_int32 level);		 // send report to client re single output channel
	virtual void report (void);			 // send report to client
	virtual void send_name_to_client( vrpn_int32 channelType, 
				vrpn_int32 channelNum, vrpn_int32 levelNum, char* name );
	virtual void send_name_to_server( vrpn_int32 channelType, 
				vrpn_int32 channelNum, vrpn_int32 levelNum, char* name );
	virtual void send_all_names_to_client( void );
	virtual void setInputChannelName(  int levelNum, int channelNum, const char* channelNameCStr);
	virtual void setOutputChannelName( int levelNum, int channelNum, const char* channelNameCStr);


	vrpn_int32  **router_state;
		// The indices to the array is an output channel # and level #.
		// The value of the array is the associated input channel.
	char	***input_channel_name;
	char	***output_channel_name;
		// Each input and output channel has a text name.
		// These names are the primary means of referring to channels.
		// They are defined in the configuration file "router.config"
		// stored on the server machine with the server executable.

  protected:
	struct timeval	timestamp;
	vrpn_int32		change_m_id;	// change message id (announcing router state change to clients)
	vrpn_int32		set_link_m_id;	// set link message id (cmd to server from client)
	vrpn_int32		name_m_id;		// name message id (server sends channel name to client)
	vrpn_int32		set_named_link_m_id; // client sends pair of channel names to be linked

	virtual int register_types(void);  // Called by BaseClass init()
	vrpn_int32	encode_channel_status(char *buf, vrpn_int32 buflen, 
					vrpn_int32 output_ch, vrpn_int32 input_ch, vrpn_int32 level );
	vrpn_int32	encode_name(char *buf, vrpn_int32 buflen, 
					vrpn_int32 channelType, vrpn_int32 channelNum, 
					vrpn_int32 levelNum, char *name );
};


/**************************************************************************************/
// Server for Sierra router.
class vrpn_Router_Sierra_Server: public vrpn_Router {
public:
	vrpn_Router_Sierra_Server(const char * name, vrpn_Connection * c );
	virtual void mainloop();

protected:
	// The timestamp field within the parent structure is used for timing

	static int vrpn_handle_set_link_message(void *userdata, vrpn_HANDLERPARAM p);
	static int vrpn_handle_set_named_link_message(void *userdata, vrpn_HANDLERPARAM p);
};


/**************************************************************************************/
// Open a router device that is on the client end of a connection
// and handle messages (ie set_link commands) from it.  
// This is the class that user code will instantiate.  
class vrpn_Router_Remote: public vrpn_Router {
  public:
	vrpn_Router_Remote (const char * name, vrpn_Connection * c = NULL);
		// name: name of the server device to connect to -- eg "Router0@DC-1-CS"
        // Optional argument to be used when the Remote MUST listen on
        // a connection that is already open.
	~vrpn_Router_Remote();
	// This routine calls the mainloop of the connection it's on
	virtual void mainloop();

	// (un)Register a callback handler function to handle router state change messages
	// going from server to client.  The client must register a handler in 
	// order to get any information back from the router server about the router state.
	virtual int register_change_handler(  void *userdata, vrpn_ROUTERCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata, vrpn_ROUTERCHANGEHANDLER handler);
	virtual int register_name_handler(  void *userdata, vrpn_ROUTERNAMEHANDLER handler);
	virtual int unregister_name_handler(void *userdata, vrpn_ROUTERNAMEHANDLER handler);

  protected:

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
	typedef	struct vrpn_RCH {
		void						*userdata;
		vrpn_ROUTERCHANGEHANDLER	handler;
		struct vrpn_RCH				*next;
	} vrpn_ROUTERCHANGELIST;
	vrpn_ROUTERCHANGELIST	change_block;
	vrpn_ROUTERCHANGELIST *	change_list;


	static int handle_name_message(void *userdata, vrpn_HANDLERPARAM p);
	typedef	struct vrpn_RNH {
		void					*userdata;
		vrpn_ROUTERNAMEHANDLER	handler;
		struct vrpn_RNH				*next;
	} vrpn_ROUTERNAMELIST;
	vrpn_ROUTERNAMELIST	  name_block;
	vrpn_ROUTERNAMELIST	* name_list;
};



	vrpn_int32	decode_name_message(   vrpn_HANDLERPARAM p, vrpn_ROUTERNAMECB & cp );
	vrpn_int32	decode_change_message( vrpn_HANDLERPARAM p, vrpn_ROUTERCB & cp );


#endif
