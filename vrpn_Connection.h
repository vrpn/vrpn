#ifndef	VRPN_CONNECTION_H
#define VRPN_CONNECTION_H

#ifndef VRPN_SHARED_H
#include "vrpn_Shared.h"
#endif

// NOTE: most users will want to use the vrpn_Synchronized_Connection
// class rather than the default connection class (either will work,
// a regular connection just has 0 as the client-server time offset)

typedef	struct {
	long		type;
	long		sender;
	struct timeval	msg_time;
	int		payload_len;
	const char	*buffer;
} vrpn_HANDLERPARAM;
typedef	int  (*vrpn_MESSAGEHANDLER)(void *userdata, vrpn_HANDLERPARAM p);

// bufs are aligned on 8 byte boundaries
#define vrpn_ALIGN                       (8)

// Types now have their storage dynamically allocated, so we can afford
// to have large tables.  We need at least 150-200 for the microscope
// project as of Jan 98, and will eventually need two to three times that
// number.
#define	vrpn_CONNECTION_MAX_SENDERS	(10)
#define	vrpn_CONNECTION_MAX_TYPES	(2000)

// vrpn_ANY_SENDER can be used to register callbacks on a given message
// type from any sender.

#define	vrpn_ANY_SENDER	(-1)

// vrpn_ANY_TYPE can be used to register callbacks for any USER type of
// message from a given sender.
#define vrpn_ANY_TYPE (-1)

// Buffer lengths for TCP and UDP.  UDP is set based on ethernet payload length
#define	vrpn_CONNECTION_TCP_BUFLEN	(64000)
#define	vrpn_CONNECTION_UDP_BUFLEN	(1500)

// System message types
#define	vrpn_CONNECTION_SENDER_DESCRIPTION	(-1)
#define	vrpn_CONNECTION_TYPE_DESCRIPTION	(-2)
#define	vrpn_CONNECTION_UDP_DESCRIPTION		(-3)

// Classes of service for messages, specify multiple by ORing them together
// Priority of satisfying these should go from the top down (RELIABLE will
// override all others).
// These flags may be ignored, but RELIABLE is guaranteed to be available.
#define	vrpn_CONNECTION_RELIABLE		(1<<0)
#define	vrpn_CONNECTION_FIXED_LATENCY		(1<<1)
#define	vrpn_CONNECTION_LOW_LATENCY		(1<<2)
#define	vrpn_CONNECTION_FIXED_THROUGHPUT	(1<<3)
#define	vrpn_CONNECTION_HIGH_THROUGHPUT		(1<<4)

#ifndef _WIN32
#define SOCKET int
#endif


// Default port to listen on for a server
#define vrpn_DEFAULT_LISTEN_PORT_NO             (4500)

// If defined, will filter out messages:  if the remote side hasn't
// registered a type, messages of that type won't be sent over the
// link.
//#define vrpn_FILTER_MESSAGES

#define vrpn_got_first_connection "VRPN Connection Got First Connection"
#define vrpn_got_connection "VRPN Connection Got Connection"
#define vrpn_dropped_connection "VRPN Connection Dropped Last Connection"
#define vrpn_dropped_last_connection "VRPN Connection Dropped Connection"

// vrpn_CONTROL is used for notification messages sent to the user
// from the local VRPN implementation (got_first_connection, etc.)
// and for control messages sent by auxiliary services.  (Such as
// class vrpn_Controller, which will be introduced in a future revision.)

#define vrpn_CONTROL "VRPN Control"


class vrpn_Connection
{
  public:
	// Create a connection to listen for incoming connections on a port
	vrpn_Connection (unsigned short listen_port_no =
		         vrpn_DEFAULT_LISTEN_PORT_NO);

	// Create a connection  makes an SDI connection to a remote server
	vrpn_Connection (char * server_name);

	//XXX Destructor should delete all entries from callback lists

	// Returns 1 if the connection is okay, 0 if not
	inline int doing_okay (void) { return (status >= 0); }

	// Returns the name of the service that the connection was first
	// constructed to talk to, or NULL if it was built as a server.
	//inline const char * name (void) const { return my_name; }

	// Call each time through program main loop to handle receiving any
	// incoming messages and sending any packed messages.
	// Returns -1 when connection dropped due to error, 0 otherwise.
	// (only returns -1 once per connection drop).
	virtual int mainloop(void);

	// Get a token to use for the string name of the sender or type.
	// Remember to check for -1 meaning failure.
	virtual long register_sender (const char * name);
	virtual long register_message_type (const char * name);

	// Set up (or remove) a handler for a message of a given type.
	// Optionally, specify which sender to handle messages from.
	// Handlers will be called during mainloop().
	// Your handler should return 0 or a communication error is assumed
	// and the connection will be shut down.
	virtual int register_handler(long type, vrpn_MESSAGEHANDLER handler,
			void *userdata, long sender = vrpn_ANY_SENDER);
	virtual	int unregister_handler(long type, vrpn_MESSAGEHANDLER handler,
			void *userdata, long sender = vrpn_ANY_SENDER);

	// Pack a message that will be sent the next time mainloop() is called.
	// Turn off the RELIABLE flag if you want low-latency (UDP) send.
	virtual int pack_message(int len, struct timeval time,
				 long type, long sender, char *buffer,
				 unsigned long class_of_service);

  protected:
	//char *	my_name;
	int	status;			// Status of the connection

        // Encapsulation of the data for a single connection
        // since we're thinking about having many clients talking to
        // a single server.
        struct vrpn_OneConnection {

          vrpn_OneConnection (void);
          vrpn_OneConnection (const SOCKET, const SOCKET);

          SOCKET tcp_sock;
          SOCKET udp_sock;
          char rhostname [150];
        };

	// Sockets used to talk to remote Connection(s)
	vrpn_OneConnection	endpoint;
	SOCKET		udp_inbound;
	unsigned short	udp_portnum;  // need to keep this around now!
		// design expects to only have one udp_inbound
		// for receipt from all remote hosts

	int	num_live_connections;

	// Only used for a vrpn_Connection that awaits incoming connections
	int	listen_udp_sock;	// Connect requests come here

	// Description of a callback entry for a user type.
	struct vrpnMsgCallbackEntry {
		vrpn_MESSAGEHANDLER	handler;	// Routine to call
		void			* userdata;	// Passed along
		long			sender;		// Only if from sender
		vrpnMsgCallbackEntry	* next;		// Next handler
	};

	// The senders we know about and the message types we know about
	// that have been declared by the local version.

	typedef char cName [100];
	struct vrpnLocalMapping {
		cName			* name;		// Name of type
		vrpnMsgCallbackEntry	* who_cares;	// Callbacks
		int			cCares;		// TCH 28 Oct 97
	};
	cName			* my_senders [vrpn_CONNECTION_MAX_SENDERS];
	int			num_my_senders;
	vrpnLocalMapping	my_types [vrpn_CONNECTION_MAX_TYPES];
	int			num_my_types;

	// Callbacks on vrpn_ANY_TYPE
	vrpnMsgCallbackEntry	* generic_callbacks;	

	// The senders and types we know about that have been described by
	// the other side.  Also, record the local mapping for ones that have
	// been described with the same name locally.
	struct cRemoteMapping {
		cName	* name;
		int	local_id;
	};
	cRemoteMapping	other_senders [vrpn_CONNECTION_MAX_SENDERS];
	int		num_other_senders;
	cRemoteMapping	other_types [vrpn_CONNECTION_MAX_TYPES];
	int		num_other_types;

	// Output buffers storing messages to be sent
	char	tcp_outbuf [vrpn_CONNECTION_TCP_BUFLEN];
	char	udp_outbuf [vrpn_CONNECTION_UDP_BUFLEN];
	int	tcp_num_out;
	int	udp_num_out;

	// Routines to handle incoming messages on file descriptors
	int	handle_udp_messages (int fd);
	int	handle_tcp_messages (int fd);

	// Routines that handle system messages
	static int handle_sender_message (void * userdata, vrpn_HANDLERPARAM p);
	static int handle_type_message (void * userdata, vrpn_HANDLERPARAM p);
	static int handle_UDP_message (void * userdata, vrpn_HANDLERPARAM p);

	// Pointers to the handlers for system messages
	vrpn_MESSAGEHANDLER	system_messages [vrpn_CONNECTION_MAX_TYPES];

	virtual	void	init (void);	// Called by all constructors

	virtual	int	send_pending_reports (void);
	virtual	void	check_connection (void);
	virtual	int	setup_for_new_connection (void);
	virtual	void	drop_connection (void);
	virtual	int	pack_sender_description (int which);
	virtual	int	pack_type_description (int which);
	virtual	int	pack_udp_description (int portno);
	virtual	int	connected (void) const;
	inline	int	outbound_udp_open (void) const
				{ return endpoint.udp_sock != -1; }
	virtual	int	marshall_message (char * outbuf, int outbuf_size,
				int initial_out, int len, struct timeval time,
				long type, long sender, char * buffer);

	virtual	int	connect_tcp_to (const char * msg);

	virtual	int	do_callbacks_for (long type, long sender,
				struct timeval time, int len, char * buffer);

	// Returns message type ID, or -1 if unregistered
	int		message_type_is_registered (const char *) const;

	// offset of clocks on connected machines -- local - remote
	// (this should really not be here, it should be in adjusted time
	// connection, but this makes it a lot easier
        struct timeval tvClockOffset;
};

// 1hz sync connection by default, windowed over last three bounces 
extern	vrpn_Connection *vrpn_get_connection_by_name (char * cname,
						      double dFreq = 1.0,
						      int cSyncWindow = 3);

// forward decls
class vrpn_Clock_Server;
class vrpn_Clock_Remote;

// NOTE: the clock offset is calculated only if the freq is >= 0
// if it is < 0, then the offset is only calced and used if the
// client specifically calls fullSync on the vrpn_Clock_Remote.
// Time stamp for messages on the connection server are never
// adjusted (only the clients get adjusted time stamps).

// NOTE: eventually this should just be another system message type
//       rather than a separate class, but right now i don't have time
//       for that.

class vrpn_Synchronized_Connection : public vrpn_Connection
{
  public:
    // Create a connection to listen for incoming connections on a port
    // server call
    vrpn_Synchronized_Connection (unsigned short listen_port_no =
		  vrpn_DEFAULT_LISTEN_PORT_NO);
    vrpn_Clock_Server * pClockServer;

    // Create a connection makes an SDI connection to a remote server
    // client call
    // freq is the frequency of clock syncs (<0 means only on user request)
    // cOffsetWindow is how many syncs to include in search window for min
    // roundtrip.  Higher values are more accurate but result in a sync
    // which accumulates drift error more quickly.
    vrpn_Synchronized_Connection(char * server_name, double dFreq = 4.0, 
			       int cOffsetWindow = 2);
    vrpn_Clock_Remote * pClockRemote;
    virtual int mainloop (void);
};

#endif // VRPN_CONNECTION_H

