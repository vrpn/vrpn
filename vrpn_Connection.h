#ifndef VRPN_CONNECTION_H
#define VRPN_CONNECTION_H
#include <stdio.h>  // for FILE

#ifndef VRPN_SHARED_H
#include "vrpn_Shared.h"
#endif

class vrpn_File_Connection;  // for get_File_Connection()

// NOTE: most users will want to use the vrpn_Synchronized_Connection
// class rather than the default connection class (either will work,
// a regular connection just has 0 as the client-server time offset)

struct vrpn_HANDLERPARAM {
	vrpn_int32	type;
	vrpn_int32	sender;
	struct timeval	msg_time;
	vrpn_int32	payload_len;
	const char	*buffer;
};
typedef	int  (*vrpn_MESSAGEHANDLER)(void *userdata, vrpn_HANDLERPARAM p);

typedef int (* vrpn_LOGFILTER) (void * userdata, vrpn_HANDLERPARAM p);

// bufs are aligned on 8 byte boundaries
#define vrpn_ALIGN                       (8)

// Types now have their storage dynamically allocated, so we can afford
// to have large tables.  We need at least 150-200 for the microscope
// project as of Jan 98, and will eventually need two to three times that
// number.
#define	vrpn_CONNECTION_MAX_SENDERS	(2000)
#define	vrpn_CONNECTION_MAX_TYPES	(2000)

// vrpn_ANY_SENDER can be used to register callbacks on a given message
// type from any sender.

#define	vrpn_ANY_SENDER	(-1)

// vrpn_ANY_TYPE can be used to register callbacks for any USER type of
// message from a given sender.  System messages are handled separately.

#define vrpn_ANY_TYPE (-1)

// Buffer lengths for TCP and UDP.
// TCP is an arbitrary number that can be changed by the user
// using vrpn_Connection::set_tcp_outbuf_size().
// UDP is set based on Ethernet maximum transmission size;  trying
// to send a message via UDP which is longer than the MTU of any
// intervening physical network may cause untraceable failures,
// so for now we do not expose any way to change the UDP output
// buffer size.  (MTU = 1500 bytes, - 28 bytes of IP+UDP header)

#define	vrpn_CONNECTION_TCP_BUFLEN	(64000)
#define	vrpn_CONNECTION_UDP_BUFLEN	(1472)

// System message types

#define	vrpn_CONNECTION_SENDER_DESCRIPTION	(-1)
#define	vrpn_CONNECTION_TYPE_DESCRIPTION	(-2)
#define	vrpn_CONNECTION_UDP_DESCRIPTION		(-3)
#define	vrpn_CONNECTION_LOG_DESCRIPTION		(-4)
#define	vrpn_CONNECTION_DISCONNECT_MESSAGE	(-5)

// Classes of service for messages, specify multiple by ORing them together
// Priority of satisfying these should go from the top down (RELIABLE will
// override all others).
// Most of these flags may be ignored, but RELIABLE is guaranteed
// to be available.

#define	vrpn_CONNECTION_RELIABLE		(1<<0)
#define	vrpn_CONNECTION_FIXED_LATENCY		(1<<1)
#define	vrpn_CONNECTION_LOW_LATENCY		(1<<2)
#define	vrpn_CONNECTION_FIXED_THROUGHPUT	(1<<3)
#define	vrpn_CONNECTION_HIGH_THROUGHPUT		(1<<4)

// What to log
#define vrpn_LOG_NONE				(0)
#define vrpn_LOG_INCOMING			(1<<0)
#define vrpn_LOG_OUTGOING			(1<<1)


#if !defined(_WIN32) || defined (__CYGWIN__)
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
#define vrpn_dropped_connection "VRPN Connection Dropped Connection"
#define vrpn_dropped_last_connection "VRPN Connection Dropped Last Connection"

// vrpn_CONTROL is used for notification messages sent to the user
// from the local VRPN implementation (got_first_connection, etc.)
// and for control messages sent by auxiliary services.  (Such as
// class vrpn_Controller, which will be introduced in a future revision.)

#define vrpn_CONTROL "VRPN Control"

typedef char cName [100];

// Placed here so vrpn_FileConnection can use it too.
struct vrpn_LOGLIST {
  vrpn_HANDLERPARAM data;
  vrpn_LOGLIST * next;
  vrpn_LOGLIST * prev;
};

// HACK
// These structs must be declared outside of vrpn_Connection
// (although we'd like to make them protected/private members)
// because aCC on PixelFlow doesn't handle nested classes correctly.

// Description of a callback entry for a user type.
struct vrpnMsgCallbackEntry {
  vrpn_MESSAGEHANDLER	handler;	// Routine to call
  void			* userdata;	// Passed along
  vrpn_int32		sender;		// Only if from sender
  vrpnMsgCallbackEntry	* next;		// Next handler
};

struct vrpnLogFilterEntry {
  vrpn_LOGFILTER filter;   // routine to call
  void * userdata;         // passed along
  vrpnLogFilterEntry * next;
};

class vrpn_Log;
class vrpn_TranslationTable;

// Encapsulation of the data and methods for a single connection
// to take care of one part of many clients talking to a single server.
// This will only be used from within the vrpn_Connection class, it should
// not be instantiated by users or devices.
// Should not be visible!
class vrpn_Endpoint
{
    public:
	vrpn_Endpoint (void);
	vrpn_Endpoint (const SOCKET, const SOCKET, const SOCKET,
		int, int, vrpn_LOGLIST *_d_logbuffer,
		vrpn_LOGLIST *_d_firstlogentry, char *_d_logname,
		long _d_logmode, int _d_logfilehandle,
		FILE *_d_logfile, vrpnLogFilterEntry *_d_logfilters );
	void	init(void);
	virtual ~vrpn_Endpoint (void);

	// Clear out the remote mapping list. This is done when a
	// connection is dropped and we want to try and re-establish
	// it.
	void	clear_other_senders_and_types(void);

	// A new local sender or type has been established; set
	// the local type for it if the other side has declared it.
	// Return 1 if the other side has one, 0 if not.
	int	newLocalSender(const char *name, vrpn_int32 which);
	int	newLocalType(const char *name, vrpn_int32 which);

	// Give the local mapping for the remote type or sender.
	// Returns -1 if there isn't one.
	int	local_type_id(vrpn_int32 remote_type);
	int	local_sender_id(vrpn_int32 remote_sender);

	// Adds a new remote type/sender and returns its index.
	// Returns -1 on error.
	int	newRemoteType(cName type_name, vrpn_int32 local_id);
	int	newRemoteSender(cName sender_name, vrpn_int32 local_id);

	inline	int	outbound_udp_open (void) const
				{ return udp_outbound != -1; }
	virtual	int	connect_tcp_to (const char * msg,
                                        const char * NIC_IPaddress);

        // Has a clock sync occured yet (slight prob of false negative, but 
        // only for a brief period)
        int clockSynced (void) { return tvClockOffset.tv_usec != 0;}

//XXX These should be protected; making them so will lead to making
//    the code split the functions between Endpoint and Connection
//    protected:
	SOCKET tcp_sock;	// Socket to send reliable data on
	SOCKET udp_outbound;	// Outbound unreliable messages go here
	SOCKET udp_inbound;	// Inbound unreliable messages come here
				// Need one for each due to different
				// clock synchronization for each; we
				// need to know which server each message is from
	// This section deals with when a client connection is trying to
	// establish (or re-establish) a connection with its server. It
	// keeps track of what we need to know to make this happen.
	SOCKET	tcp_client_listen_sock;	// Socket that the client listens on
				// when lobbing datagrams at the server and
				// waiting for it to call back.
	int	tcp_client_listen_port;	// The port number of this socket
	char	*remote_machine_name;	// Machine to call
	int	remote_UDP_port;	// UDP port on remote machine
	struct timeval last_UDP_lob;	// When the last lob occured
	long	remote_log_mode;	// Mode to put the remote logging in
	char	*remote_log_name;	// Name of the remote log file

	// offset of clocks on connected machines -- local - remote
	// (this should really not be here, it should be in adjusted time
	// connection, but this makes it a lot easier
	struct timeval tvClockOffset;

	// Name of the remote host we are connected to.  This is kept for
	// informational purposes.  It is printed by the ceiling server,
	// for example.
	char rhostname [150];

	// Logging - TCH 11 June 98

    vrpn_Log * d_log;

  protected:

    // The senders and types we know about that have been described by
    // the other end of the connection.  Also, record the local mapping
    // for ones that have been described with the same name locally.
    // The arrays are indexed by the ID from the other side, and store
    // the name and local ID that corresponds to each.

    vrpn_TranslationTable * d_senders;
    vrpn_TranslationTable * d_types;

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
	vrpn_bool doing_okay (void) const;
	virtual	vrpn_bool connected (void) const;

	// This is similar to check connection except that it can be
	// used to receive requests from before a server starts up
	virtual int connect_to_client(const char *machine, int port);

	// Returns the name of the service that the connection was first
	// constructed to talk to, or NULL if it was built as a server.
	//inline const char * name (void) const { return my_name; }

	// Call each time through program main loop to handle receiving any
	// incoming messages and sending any packed messages.
	// Returns -1 when connection dropped due to error, 0 otherwise.
	// (only returns -1 once per connection drop).
        // Optional argument is TOTAL time to block on select() calls;
        // there may be multiple calls to select() per call to mainloop(),
        // and this timeout will be divided evenly between them.
	virtual int mainloop (const struct timeval * timeout = NULL);

	// Get a token to use for the string name of the sender or type.
	// Remember to check for -1 meaning failure.
	virtual vrpn_int32 register_sender (const char * name);
	virtual vrpn_int32 register_message_type (const char * name);

	// Set up (or remove) a handler for a message of a given type.
	// Optionally, specify which sender to handle messages from.
	// Handlers will be called during mainloop().
	// Your handler should return 0 or a communication error is assumed
	// and the connection will be shut down.
	virtual int register_handler(vrpn_int32 type,
		vrpn_MESSAGEHANDLER handler, void *userdata,
		vrpn_int32 sender = vrpn_ANY_SENDER);
	virtual	int unregister_handler(vrpn_int32 type,
		vrpn_MESSAGEHANDLER handler, void *userdata,
		vrpn_int32 sender = vrpn_ANY_SENDER);

	// Pack a message that will be sent the next time mainloop() is called.
	// Turn off the RELIABLE flag if you want low-latency (UDP) send.
	virtual int pack_message(vrpn_uint32 len, struct timeval time,
		vrpn_int32 type, vrpn_int32 sender, const char * buffer,
		vrpn_uint32 class_of_service);

	// send pending report, clear the buffer.
	// This function was protected, now is public, so we can use it
	// to send out intermediate results without calling mainloop
	virtual int     send_pending_reports (void);

	// Returns the time since the connection opened.
	// Some subclasses may redefine time.
	virtual int time_since_connection_open
	                        (struct timeval * elapsed_time);

	// Returns the name of the specified sender/type, or NULL
	// if the parameter is invalid.  Only works for user
	// messages (type >= 0).
	virtual const char * sender_name (vrpn_int32 sender);
	virtual const char * message_type_name (vrpn_int32 type);

        // Sets up a filter function for logging.
        // Any user message to be logged is first passed to this function,
        // and will only be logged if the function returns zero (XXX).
        // NOTE:  this only affects local logging - remote logging
        // is unfiltered!  Only user messages are filtered;  all system
        // messages are logged.
        // Returns nonzero on failure.
        virtual int register_log_filter (vrpn_LOGFILTER filter,
                                         void * userdata);

  // Applications that need to send very large messages may want to
  // know the buffer size used or to change its size.  Buffer size
  // is returned in bytes.
  vrpn_int32 tcp_outbuf_size (void) const { return d_tcp_buflen; }
  vrpn_int32 udp_outbuf_size (void) const { return d_udp_buflen; }

  // Allows the application to attempt to reallocate the buffer.
  // If allocation fails (error or out-of-memory), -1 will be returned.
  // On successful allocation or illegal input (negative size), the
  // size in bytes will be returned.  These routines are NOT guaranteed
  // to shrink the buffer, only to increase it or leave it the same.
  vrpn_int32 set_tcp_outbuf_size (vrpn_int32 bytecount);

  vrpn_Endpoint * endpointPtr (void) { return &endpoint; }

  // vrpn_File_Connection implements this as "return this" so it
  // can be used to detect a File_Connection and get the pointer for it
  virtual vrpn_File_Connection * get_File_Connection (void);

  protected:
  // Users should not create vrpn_Connection directly -- use 
  // vrpn_Synchronized_Connection (for servers) or 
  // vrpn_get_connection_by_name (for clients)

	// Create a connection to listen for incoming connections on a port
	vrpn_Connection (unsigned short listen_port_no =
		         vrpn_DEFAULT_LISTEN_PORT_NO,
                         const char * local_logfile_name = NULL,
                         long local_log_mode = vrpn_LOG_NONE,
                         const char * NIC_IPaddress = NULL);

	//   Create a connection -  if server_name is not a file: name,
	// makes an SDI-like connection to the named remote server
	// (otherwise functions as a non-networked messaging hub).
	// Port less than zero forces default.
	//   Currently, server_name is an extended URL that defaults
	// to VRPN connections at the port, but can be file:: to read
	// from a file.  Other extensions should maintain this, so
	// that VRPN uses URLs to name things that are to be connected
	// to.
	vrpn_Connection (const char * server_name,
                         int port = vrpn_DEFAULT_LISTEN_PORT_NO,
                         const char * local_logfile_name = NULL,
                         long local_log_mode = vrpn_LOG_NONE,
                         const char * remote_logfile_name = NULL,
                         long remote_log_mode = vrpn_LOG_NONE,
                         const char * NIC_IPaddress = NULL);

	//char *	my_name;
	int	status;			// Status of the connection

	// Sockets used to talk to remote Connection(s)
	// and other information needed on a per-connection basis
	vrpn_Endpoint	endpoint;
	vrpn_int32	num_live_connections;

	// Only used for a vrpn_Connection that awaits incoming connections
	int	listen_udp_sock;	// Connect requests come here

	// The senders we know about and the message types we know about
	// that have been declared by the local version.
        // cCares:  has the other side of this connection registered
        //   a type by this name?  Only used by filtering.

	struct vrpnLocalMapping {
		char			* name;		// Name of type
		vrpnMsgCallbackEntry	* who_cares;	// Callbacks
		vrpn_int32		cCares;		// TCH 28 Oct 97
	};
	char			* my_senders [vrpn_CONNECTION_MAX_SENDERS];
	vrpn_int32		num_my_senders;
	vrpnLocalMapping	my_types [vrpn_CONNECTION_MAX_TYPES];
	vrpn_int32		num_my_types;

	// Callbacks on vrpn_ANY_TYPE
	vrpnMsgCallbackEntry	* generic_callbacks;	

	// Output buffers storing messages to be sent
	// Convention:  use d_ to denote a data member of an object so
	// that inside a method you can recognize data members at a glance
	// and differentiate them from parameters or local variables.

	char *	d_tcp_outbuf;
	char *	d_udp_outbuf;
        int	d_tcp_buflen;  // total buffer size, in bytes
	int	d_udp_buflen;
	int	d_tcp_num_out;  // number of bytes currently used
	int	d_udp_num_out;

	// Routines to handle incoming messages on file descriptors
	int	handle_udp_messages (int fd, const struct timeval * timeout);
	int	handle_tcp_messages (int fd, const struct timeval * timeout);

	// Routines that handle system messages
	static int handle_sender_message (void * userdata, vrpn_HANDLERPARAM p);
	static int handle_type_message (void * userdata, vrpn_HANDLERPARAM p);
	static int handle_UDP_message (void * userdata, vrpn_HANDLERPARAM p);
	static int handle_log_message (void * userdata, vrpn_HANDLERPARAM p);
	static int handle_disconnect_message (void * userdata,
		vrpn_HANDLERPARAM p);

	// Pointers to the handlers for system messages
	vrpn_MESSAGEHANDLER	system_messages [vrpn_CONNECTION_MAX_TYPES];

	virtual	void	init (void);	// Called by all constructors

	// This is called by a server-side process to see if there have
	// been any UDP packets come in asking for a connection. If there
	// are, it connects the TCP port and then calls handle_connection().
	virtual	void	check_connection(const struct timeval *timeout = NULL);

	// This routine is called by a server-side connection when a
	// new connection has just been established, and the tcp port
	// has been connected to it.
	virtual void	handle_connection(void);

	// This sends the magic cookie and other information to its
	// peer.  It is called by both the client and server setup routines.
	virtual	int	setup_new_connection (long logmode = 0L,
	                                      const char * logfile = NULL);

        // This receives the peer's cookie. It is called by both
	// the client and server setup routines.
	virtual	int	finish_new_connection_setup (void);
        void poll_for_cookie (const timeval * pTimeout = NULL);

        long d_pendingLogmode;
        char * d_pendingLogname;

	// set up network
	virtual	void	drop_connection (void);
	virtual	int	pack_sender_description (vrpn_int32 which);
	virtual	int	pack_type_description (vrpn_int32 which);
	virtual	int	pack_udp_description (int portno);
	virtual int	pack_log_description (long mode, const char * filename);
	virtual	int	marshall_message (char * outbuf,vrpn_uint32 outbuf_size,
				vrpn_uint32 initial_out,
				vrpn_uint32 len, struct timeval time,
				vrpn_int32 type, vrpn_int32 sender,
				const char * buffer,
                                vrpn_uint32 sequenceNumber);

	virtual	int	do_callbacks_for (vrpn_int32 type, vrpn_int32 sender,
				struct timeval time, vrpn_uint32 len,
	                        const char * buffer);

	// Returns message type ID, or -1 if unregistered
	int		message_type_is_registered (const char *) const;

	// Thread safety modifications - TCH 19 May 98
	// Convention:  use d_ to denote a data member of an object so
	// that inside a method you can recognize data members at a glance
	// and differentiate them from parameters or local variables.

	// Input buffers
	vrpn_float64 d_TCPinbufToAlignRight
		[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64) + 1];
	char * d_TCPbuf;
	vrpn_float64 d_UDPinbufToAlignRight
		[vrpn_CONNECTION_UDP_BUFLEN / sizeof(vrpn_float64) + 1];
	char * d_UDPinbuf;

	// Timekeeping - TCH 30 June 98
	struct timeval start_time;

    vrpn_uint32 d_sequenceNumberUDP;
      /**<
       * All packets are sent out with a sequence number;  this uses
       * the sixth ("padding") word in the VRPN header.  Currently
       * two sequences are maintained:  one for TCP, one for UDP.
       */

    vrpn_uint32 d_sequenceNumberTCP;
      /**<
       * All packets are sent out with a sequence number;  this uses
       * the sixth ("padding") word in the VRPN header.  Currently
       * two sequences are maintained:  one for TCP, one for UDP.
       */

    const char * d_NIC_IP;
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
    // server call.  If no IP address for the NIC to use is specified,
    // uses the default NIC.
    vrpn_Synchronized_Connection (unsigned short listen_port_no =
		         vrpn_DEFAULT_LISTEN_PORT_NO,
                         const char * local_logfile_name = NULL,
                         long local_log_mode = vrpn_LOG_NONE,
                         const char * NIC_IPaddress = NULL);
    vrpn_Clock_Server * pClockServer;

    // Create a connection makes aconnection to a remote server
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
	  int cOffsetWindow = 2,
          const char * NIC_IPaddress = NULL);
    // fullSync will perform an accurate sync on the connection for the
    // user and return the current offset
	~vrpn_Synchronized_Connection();
	struct timeval fullSync();
    vrpn_Clock_Remote * pClockRemote;
    virtual int mainloop (const struct timeval * timeout = NULL );
};

// 1hz sync connection by default, windowed over last three bounces 
// WARNING:  vrpn_get_connection_by_name() may not be thread safe.
// If no IP address for the NIC to use is specified, uses the default
// NIC.
vrpn_Connection * vrpn_get_connection_by_name
         (const char * cname,
          const char * local_logfile_name = NULL,
          long local_log_mode = vrpn_LOG_NONE,
          const char * remote_logfile_name = NULL,
          long remote_log_mode = vrpn_LOG_NONE,
	  double dFreq = 1.0, int cSyncWindow = 3,
          const char * NIC_IPaddress = NULL);


// Utility routines to parse names (<service>@<location specifier>)
// Both return new char [], and it is the caller's responsibility
// to delete this memory!
char * vrpn_copy_service_name (const char * fullname);
char * vrpn_copy_service_location (const char * fullname);

// Utility routines to parse file specifiers FROM service locations
//   file:<filename>
//   file://<hostname>/<filename>
//   file:///<filename>
char * vrpn_copy_file_name (const char * filespecifier);

// Utility routines to parse host specifiers FROM service locations
//   <hostname>
//   <hostname>:<port number>
//   x-vrpn://<hostname>
//   x-vrpn://<hostname>:<port number>
//   x-vrsh://<hostname>/<server program>,<comma-separated server arguments>
char * vrpn_copy_machine_name (const char * hostspecifier);
int vrpn_get_port_number (const char * hostspecifier);
char * vrpn_copy_rsh_program (const char * hostspecifier);
char * vrpn_copy_rsh_arguments (const char * hostspecifier);

// Checks the buffer to see if it is a valid VRPN header cookie.
// Returns -1 on total mismatch,
// 1 on minor version mismatch or other acceptable difference,
// and 0 on exact match.
int check_vrpn_cookie (const char * buffer);

// Returns the size of the magic cookie buffer, plus any alignment overhead.
int vrpn_cookie_size (void);

int write_vrpn_cookie (char * buffer, int length, long remote_log_mode);

#endif // VRPN_CONNECTION_H

