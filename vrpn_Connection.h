#ifndef	VRPN_CONNECTION_H

#include <sys/time.h>

class	vrpn_Connection;

typedef	struct {
	long		type;
	long		sender;
	struct timeval	msg_time;
	int		payload_len;
	const char	*buffer;
} vrpn_HANDLERPARAM;
typedef	int  (*vrpn_MESSAGEHANDLER)(void *userdata, vrpn_HANDLERPARAM p);

#define	vrpn_CONNECTION_MAX_SENDERS	(10)
#define	vrpn_CONNECTION_MAX_TYPES	(50)

#define	vrpn_ANY_SENDER	(-1)

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

// Default port to listen on for a server
#define	vrpn_DEFAULT_LISTEN_PORT_NO		(4500)

class vrpn_Connection
{
  public:
	// Create a connection to listen for incoming connections on a port
	vrpn_Connection(unsigned short listen_port_no =
		vrpn_DEFAULT_LISTEN_PORT_NO);

	// Create a connection that makes an SDI connection to a remote server
	vrpn_Connection(char *server_name);

	//XXX Destructor should delete all entries from callback lists

	// Returns 1 if the connection is okay, 0 if not
	inline int doing_okay(void) { return (status >= 0); };

	// Call each time through program main loop to handle receiving any
	// incoming messages and sending any packed messages.
	// Returns -1 when connection dropped due to error, 0 otherwise.
	// (only returns -1 once per connection drop).
	virtual int mainloop(void);

	// Get a token to use for the string name of the sender or type.
	// Remember to check for -1 meaning failure.
	virtual long register_sender(char *name);
	virtual long register_message_type(char *name);

//XXX	Still need pc_station to wrap the tracker and button callbacks and
//XXX	tell the connection about its handlers.
//XXX	Eventually, make button and tracker work on both sides, so that
//XXX	packing and unpacking is done by the same object, (open it by name).

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
	int	status;			// Status of the connection

	// Sockets used to talk to a remote Connection
	int	tcp_sock;		// TCP socket to other machine
	int	udp_outbound;		// UDP socket to other machine
	int	udp_inbound;		// UDP socket from other machine

	// Only used for a vrpn_Connection that awaits incoming connections
	int	listen_udp_sock;	// Connect requests come here
	char	rmachine_name[150];	// Name of remote machine to connect to

	// Description of a callback entry for a user type.
	typedef struct VRPNHANDLER {
		vrpn_MESSAGEHANDLER	handler;	// Routine to call
		void			*userdata;	// Passed along
		long			sender;		// Only if from sender
		struct VRPNHANDLER	*next;		// Next handler
	} vrpnMsgCallbackEntry;

	// The senders we know about and the message types we know about
	// that have been declared by the local version.
	typedef char cName[100];
	typedef struct {
		cName			name;		// Name of type
		vrpnMsgCallbackEntry	*who_cares;	// Callbacks
	} vrpnLocalMapping;
	cName			my_senders[vrpn_CONNECTION_MAX_SENDERS];
	int			num_my_senders;
	vrpnLocalMapping	my_types[vrpn_CONNECTION_MAX_TYPES];
	int			num_my_types;

	// The senders and types we know about that have been described by
	// the other side.  Also, record the local mapping for ones that have
	// been described with the same name locally.
	typedef	struct {
		cName	name;
		int	local_id;
	} cRemoteMapping;
	cRemoteMapping	other_senders[vrpn_CONNECTION_MAX_SENDERS];
	int		num_other_senders;
	cRemoteMapping	other_types[vrpn_CONNECTION_MAX_TYPES];
	int		num_other_types;

	// Output buffers storing messages to be sent
	char	tcp_outbuf[vrpn_CONNECTION_TCP_BUFLEN];
	char	udp_outbuf[vrpn_CONNECTION_UDP_BUFLEN];
	int	tcp_num_out;
	int	udp_num_out;

	// Routines to handle incoming messages on file descriptors
	int	handle_udp_messages(int fd);
	int	handle_tcp_messages(int fd);

	// Routines that handle system messages
	static int handle_sender_message(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_type_message(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_UDP_message(void *userdata, vrpn_HANDLERPARAM p);

	// Pointers to the handlers for system messages
	vrpn_MESSAGEHANDLER	system_messages[vrpn_CONNECTION_MAX_TYPES];

	virtual	void	init(void);	// Called by all constructors

	virtual	int	send_pending_reports(void);
	virtual	void	check_connection(void);
	virtual	int	setup_for_new_connection(void);
	virtual	void	drop_connection(void);
	virtual	int	pack_sender_description(int which);
	virtual	int	pack_type_description(int which);
	virtual	int	pack_udp_description(int portno);
	virtual	int	connected(void);
	inline	int	outbound_udp_open(void) {return udp_outbound != -1; };
	virtual	int	marshall_message(char *outbuf, int outbuf_size,
				int initial_out, int len, struct timeval time,
				long type, long sender, char *buffer);

	virtual	int	connect_tcp_to(char *msg);

	virtual	int	do_callbacks_for(long type, long sender,
				struct timeval time, int len, char *buffer);
};

extern	vrpn_Connection *vrpn_get_connection_by_name(char *cname);

#define VRPN_CONNECTION_H
#endif
