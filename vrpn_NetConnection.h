////////////////////////////////////////////////////////
//
// class vrpn_NetConnection
//
// This class provides a single communication channel over the network.
// It encompasses a tcp connection, a udp sonnection, and a udp connection
// for receiving multicast messages. In addition to the various types of
// network connections, the NetConnection also provides logging 
// cpapbilities.
//
// NetConnection replaces the OneConnection class.


#ifndef VRPN_NETCONNECTION_INCLUDED
#define VRPN_NETCONNECTION_INCLUDED

#include "vrpn_ConnectionOldCommonStuff.h"
#include "vrpn_BaseConnection.h"

class vrpn_NetConnection:: public vrpn_BaseConnection {

public:  // c'tors and d'tors
    vrpn_NetConnection();	
    virtual ~vrpn_NetConnection();

public:  // sending and receiving

    // functions for sending messages and receiving messages
    // the ConnectionController will call these functions

    vrpn_int32 handle_outgoing_messages( /*...XXX...*/ );

    vrpn_int32 handle_incoming_messages( /*...XXX...*/ );


	// Pack a message that will be sent the next time mainloop() is called.
	// Turn off the RELIABLE flag if you want low-latency (UDP) send.
	virtual vrpn_int32 pack_message(vrpn_uint32 len, struct timeval time,
		vrpn_int32 type, vrpn_int32 sender, const char * buffer,
		vrpn_uint32 class_of_service);

	// send pending report, clear the buffer.
	// This function was protected, now is public, so we can use it
	// to send out intermediate results without calling mainloop
	virtual vrpn_int32 send_pending_reports (void);

protected:	// sending and receiving

	// Routines to handle incoming messages on file descriptors
	// called by handle_incoming_message

	vrpn_int32 handle_udp_messages (vrpn_int32 fd, const struct timeval * timeout);
	vrpn_int32 handle_tcp_messages (vrpn_int32 fd, const struct timeval * timeout);
	vrpn_int32 handle_mcast_messages(/* XXX */);

public: // status

	// Routines that report status of connections
	inline vrpn_int32 outbound_udp_open (void) const
	  { return udp_outbound != -1; }
	inline vrpn_int32 outbound_mcast_open (void) const
	  { return mcast_recv ? 1 : 0; } // if multicast recvr ptr non-null

public: // setting up connections

	//----------------------------
	// server side

	// This is similar to check connection except that it can be
	// used to receive requests from before a server starts up
	// Returns the name of the service that the connection was first
	// constructed to talk to, or NULL if it was built as a server.
	//inline const char * name (void) const { return my_name; }
	virtual vrpn_int32 connect_to_client(const char* machine, vrpn_int16 port);

	//-----------------------------
	// client side

	// doesn't exst yet, but seems to be a natural counterpart
	// for connect_to_client
	virtual vrpn_int32 connect_to_server(const char* machine, vrpn_int16 port);

protected: // setting up connections

	//----------------------------
	// server side

	vrpn_int32 connect_udp_to(const char* machine, vrpn_int16 port);
	// i think the arg list on this should be (machine,port)
	virtual vrpn_int32 connect_tcp_to(const char* msg);

	//-----------------------------
	// client side

	// formerly vrpn_udp_request_call
	vrpn_int32 udp_request_connection_call(const char* machine, vrpn_int16 port);
	vrpn_int32 open_udp_socket(vrpn_int16 port);


	//-----------------------------
	// common functions

	// i think that these are common functions. not sure yet.
	virtual void	handle_connection(void);
	// set up data
	virtual	vrpn_int32	setup_new_connection (vrpn_int32 logmode = 0L,
											  const char * logfile = NULL);
	// set up network
	virtual	void	drop_connection (void);
	virtual	vrpn_int32	pack_sender_description (vrpn_int32 which);
	virtual	vrpn_int32	pack_type_description (vrpn_int32 which);
	virtual	vrpn_int32	pack_udp_description (vrpn_int16 portno);
	virtual vrpn_int32	pack_log_description (vrpn_int32 mode, const char * filename);
 
private: // data members

	vrpn_int32	status;			// Status of the connection

	/*
	// perhaps we should move to enums
	enum ConnectionStatus = { LISTEN, CONNECTED, CONNECTION_FAIL, BROKEN, DROPPED };
	ConnectionStatus status;
	*/

	// Output buffers storing messages to be sent
	// Convention:  use d_ to denote a data member of an object so
	// that inside a method you can recognize data members at a glance
	// and differentiate them from parameters or local variables.

	char *	d_tcp_outbuf;
	char *	d_udp_outbuf;
	vrpn_int32	d_tcp_buflen;  // total buffer size, in bytes
	vrpn_int32	d_udp_buflen;
	vrpn_int32	d_tcp_num_out;  // number of bytes currently used
	vrpn_int32	d_udp_num_out;


	// Input buffers
	// HACK:  d_TCPbuf uses malloc()/realloc()/free() instead of
	//   new/delete.
	vrpn_uint32 d_TCPbuflen;
	char * d_TCPbuf;
	char * d_UDPinbuf;
	// used to know how many bytes to read from the udp socket
	vrpn_float64 d_UDPinbufToAlignRight
        [vrpn_CONNECTION_UDP_BUFLEN / sizeof(vrpn_float64) + 1];


	// Socket to send reliable data on
	SOCKET tcp_sock;
	// Outbound unreliable messages go here
	SOCKET udp_outbound;
	// Inbound unreliable messages come here	
	SOCKET udp_inbound;	
	// Need one for each due to different
	// clock synchronization for each; we
	// need to know which server each message is from


	// offset of clocks on connected machines -- local - remote
	// (this should really not be here, it should be in adjusted time
	// connection, but this makes it a lot easier
	struct timeval tvClockOffset;

	// Timekeeping - TCH 30 June 98
	struct timeval start_time;

	// Name of the remote host we are connected to.  This is kept for
	// informational purposes.  It is printed by the ceiling server,
	// for example.
	char rhostname [150];


	// Holds one entry for a mapping of remote strings to local IDs.
	struct cRemoteMapping {
		char		* name;
		vrpn_int32	local_id;
	};

	// The senders and types we know about that have been described by
	// the other end of the connection.  Also, record the local mapping
	// for ones that have been described with the same name locally.
	// The arrays are indexed by the ID from the other side, and store
	// the name and local ID that corresponds to each.
	cRemoteMapping	other_senders [vrpn_CONNECTION_MAX_SENDERS];
	vrpn_int32	num_other_senders;
	cRemoteMapping	other_types [vrpn_CONNECTION_MAX_TYPES];
	vrpn_int32	num_other_types;

};




