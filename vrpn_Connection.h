#ifndef	VRPN_CONNECTION_H
#define VRPN_CONNECTION_H

#include <stdio.h>  // for FILE

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
#define	vrpn_CONNECTION_LOG_DESCRIPTION		(-4)

// Classes of service for messages, specify multiple by ORing them together
// Priority of satisfying these should go from the top down (RELIABLE will
// override all others).
// These flags may be ignored, but RELIABLE is guaranteed to be available.
#define	vrpn_CONNECTION_RELIABLE		(1<<0)
#define	vrpn_CONNECTION_FIXED_LATENCY		(1<<1)
#define	vrpn_CONNECTION_LOW_LATENCY		(1<<2)
#define	vrpn_CONNECTION_FIXED_THROUGHPUT	(1<<3)
#define	vrpn_CONNECTION_HIGH_THROUGHPUT		(1<<4)

// What to log
#define vrpn_LOG_NONE				(0)
#define vrpn_LOG_INCOMING			(1<<0)
#define vrpn_LOG_OUTGOING			(1<<1)


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

typedef char cName [100];


// Description of a callback entry for a user type.
struct vrpnMsgCallbackEntry {
  vrpn_MESSAGEHANDLER	handler;	// Routine to call
  void			* userdata;	// Passed along
  long			sender;		// Only if from sender
  vrpnMsgCallbackEntry	* next;		// Next handler
};

class vrpn_Connection
{
  public:
  // No public constructors because ...
  // Users should not create vrpn_Connection directly -- use 
  // vrpn_Synchronized_Connection (for servers) or 
  // vrpn_get_connection_by_name (for clients)

	//XXX Destructor should delete all entries from callback lists
	virtual ~vrpn_Connection (void);

	// Returns 1 if the connection is okay, 0 if not
	inline int doing_okay (void) { return (status >= 0); }
	virtual	int	connected (void) const;

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
				 long type, long sender, const char * buffer,
				 unsigned long class_of_service);

	// Returns the time since the connection opened.
	// Some subclasses may redefine time.
	virtual int time_since_connection_open
	                        (struct timeval * elapsed_time);

	// Returns the name of the specified sender/type, or NULL
	// if the parameter is invalid.  Only works for user
	// messages (type >= 0).
	virtual const char * sender_name (long sender);
	virtual const char * message_type_name (long type);

  protected:
  // Users should not create vrpn_Connection directly -- use 
  // vrpn_Synchronized_Connection (for servers) or 
  // vrpn_get_connection_by_name (for clients)

	// Create a connection to listen for incoming connections on a port
	vrpn_Connection (unsigned short listen_port_no =
		         vrpn_DEFAULT_LISTEN_PORT_NO);

	//   Create a connection -  if server_name is not a file: name,
	// makes an SDI-like connection to the named remote server
	// (otherwise functions as a non-networked messaging hub).
	// Port less than zero forces default.
	vrpn_Connection (const char * server_name,
                         int port = vrpn_DEFAULT_LISTEN_PORT_NO,
                         const char * local_logfile_name = NULL,
                         long local_log_mode = vrpn_LOG_NONE,
                         const char * remote_logfile_name = NULL,
                         long remote_log_mode = vrpn_LOG_NONE);

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

	// The senders we know about and the message types we know about
	// that have been declared by the local version.

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
	static int handle_log_message (void * userdata, vrpn_HANDLERPARAM p);

	// Pointers to the handlers for system messages
	vrpn_MESSAGEHANDLER	system_messages [vrpn_CONNECTION_MAX_TYPES];

	virtual	void	init (void);	// Called by all constructors

	virtual	int	send_pending_reports (void);
	virtual	void	check_connection (void);
	virtual	int	setup_for_new_connection (void);
		// set up data
	virtual	int	setup_new_connection (long logmode = 0L,
	                                      const char * logfile = NULL);
		// set up network
	virtual	void	drop_connection (void);
	virtual	int	pack_sender_description (int which);
	virtual	int	pack_type_description (int which);
	virtual	int	pack_udp_description (int portno);
	virtual int	pack_log_description (long mode,
                                              const char * filename);
	inline	int	outbound_udp_open (void) const
				{ return endpoint.udp_sock != -1; }
	virtual	int	marshall_message (char * outbuf, int outbuf_size,
				int initial_out, int len, struct timeval time,
				long type, long sender, const char * buffer);

	virtual	int	connect_tcp_to (const char * msg);

	virtual	int	do_callbacks_for (long type, long sender,
				struct timeval time, int len,
	                        const char * buffer);

	// Returns message type ID, or -1 if unregistered
	int		message_type_is_registered (const char *) const;

	// offset of clocks on connected machines -- local - remote
	// (this should really not be here, it should be in adjusted time
	// connection, but this makes it a lot easier
        struct timeval tvClockOffset;

	// Thread safety modifications - TCH 19 May 98

	// Input buffers
	int d_TCPbuflen;
	char * d_TCPbuf;
	double d_UDPinbufToAlignRight
			[vrpn_CONNECTION_UDP_BUFLEN/sizeof(double)+1];
	char *d_UDPinbuf;

	// Logging - TCH 11 June 98

	struct vrpn_LOGLIST {
	  vrpn_HANDLERPARAM data;
	  vrpn_LOGLIST * next;
	  vrpn_LOGLIST * prev;
	};

	vrpn_LOGLIST * d_logbuffer;  // last entry in log
	vrpn_LOGLIST * d_first_log_entry;  // first entry in log
	char * d_logname;            // name of file to write log to
	long d_logmode;              // logging incoming, outgoing, or both
	int d_logfile_handle;
	FILE * d_logfile;

	virtual int log_message (int payload_len, struct timeval time,
	                         long type, long sender, const char * buffer);
	virtual int close_log (void);
	virtual int open_log (void);

	// Timekeeping - TCH 30 June 98
	struct timeval start_time;

};

// forward decls
class vrpn_Clock_Server;
class vrpn_Clock_Remote;

// NOTE: the clock offset is calculated only if the freq is >= 0
// if it is -1, then the offset is only calced and used if the
// client specifically calls fullSync on the vrpn_Clock_Remote.
// if the freq is -2, then the offset is calced immediately using
// full sync.
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
    vrpn_Synchronized_Connection
	 (const char * server_name,
          int port = vrpn_DEFAULT_LISTEN_PORT_NO,
          const char * local_logfile_name = NULL,
          long local_log_mode = vrpn_LOG_NONE,
          const char * remote_logfile_name = NULL,
          long remote_log_mode = vrpn_LOG_NONE,
	  double dFreq = 4.0, 
	  int cOffsetWindow = 2);
    // fullSync will perform an accurate sync on the connection for the
    // user and return the current offset
    struct timeval fullSync();
    vrpn_Clock_Remote * pClockRemote;
    virtual int mainloop (void);
};

// 1hz sync connection by default, windowed over last three bounces 
// WARNING:  vrpn_get_connection_by_name() may not be thread safe.
vrpn_Connection * vrpn_get_connection_by_name
         (const char * cname,
          const char * local_logfile_name = NULL,
          long local_log_mode = vrpn_LOG_NONE,
          const char * remote_logfile_name = NULL,
          long remote_log_mode = vrpn_LOG_NONE,
	  double dFreq = 1.0, int cSyncWindow = 3);


// Utility routines to parse names (<service>@<location specifier>)
// Both return new char [], and it is the caller's responsibility
// to delete this memory!
char * vrpn_copy_service_name (const char * fullname);
char * vrpn_copy_service_location (const char * fullname);

// Utility routines to parse file specifiers
//   file:<filename>
//   file://<hostname>/<filename>
//   file:///<filename>
char * vrpn_copy_file_name (const char * filespecifier);

// Utility routines to parse host specifiers
//   <hostname>
//   <hostname>:<port number>
//   x-vrpn://<hostname>
//   x-vrpn://<hostname>:<port number>
char * vrpn_copy_machine_name (const char * hostspecifier);
int vrpn_get_port_number (const char * hostspecifier);

#endif // VRPN_CONNECTION_H

