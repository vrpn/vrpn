// vrpn_ConnectionCommonStuff.h
// 
// this is a place to put stuff that is shared between the different
// connection classes like BaseConnectionController and BaseConnection
// and their subclasses

#ifndef VRPN_CONNECTIONCOMMONSTUFF_INCLUDED
#define VRPN_CONNECTIONCOMMONSTUFF_INCLUDED

#include "vrpn_Shared.h"

// used in clock synchronization
#define CLOCK_VERSION 0x10

// bufs are aligned on 8 byte boundaries
#define vrpn_ALIGN                       (8)


// Syntax of MAGIC:
//   The minor version number must follow the last period ('.') and be
// the only significant thing following the last period in the MAGIC
// string.  Since minor versions should interoperate, MAGIC is only
// checked through the last period;  characters after that are ignored.

const char * vrpn_MAGIC = (const char *) "vrpn: ver. 04.11";
const int vrpn_MAGICLEN = 16;  // Must be a multiple of vrpn_ALIGN bytes!


// Types now have their storage dynamically allocated, so we can afford
// to have large tables.  We need at least 150-200 for the microscope
// project as of Jan 98, and will eventually need two to three times that
// number.
#define	vrpn_CONNECTION_MAX_SENDERS   (2000) /*XXX*/
#define	vrpn_CONNECTION_MAX_SERVICES  (2000)
#define	vrpn_CONNECTION_MAX_TYPES     (2000)


// vrpn_ANY_SERVICE can be used to register callbacks on a given message
// type from any service.

#define	vrpn_ANY_SENDER  (-1) /*XXX*/
#define	vrpn_ANY_SERVICE (-1)

// vrpn_ANY_TYPE can be used to register callbacks for any USER type of
// message from a given sender.  System messages are handled separately.

#define vrpn_ANY_TYPE (-1)


// Classes of service for messages, specify multiple by ORing them together
// Priority of satisfying these should go from the top down (RELIABLE will
// override all others).
// Most of these flags may be ignored, but RELIABLE is guaranteed
// to be available.

#define vrpn_CONNECTION_RELIABLE        (1<<0)
#define vrpn_CONNECTION_FIXED_LATENCY       (1<<1)
#define vrpn_CONNECTION_LOW_LATENCY     (1<<2)
#define vrpn_CONNECTION_FIXED_THROUGHPUT    (1<<3)
#define vrpn_CONNECTION_HIGH_THROUGHPUT     (1<<4)

#define vrpn_got_first_connection "VRPN Connection Got First Connection"
#define vrpn_got_connection "VRPN Connection Got Connection"
#define vrpn_dropped_connection "VRPN Connection Dropped Connection"
#define vrpn_dropped_last_connection "VRPN Connection Dropped Last Connection"


// vrpn_CONTROL is used for notification messages sent to the user
// from the local VRPN implementation (got_first_connection, etc.)
// and for control messages sent by auxiliary services.  (Such as
// class vrpn_Controller, which will be introduced in a future revision.)

#define vrpn_CONTROL "VRPN Control"


struct vrpn_HANDLERPARAM {
    vrpn_int32  type;
    //vrpn_int32  sender;
    vrpn_int32  service;
    struct timeval  msg_time;
    vrpn_int32  payload_len;
    const char  *buffer;
};
typedef vrpn_int32 (*vrpn_MESSAGEHANDLER)(void *userdata, vrpn_HANDLERPARAM p);

typedef char cName [100];


// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#ifndef	_WIN32
#define	INVALID_SOCKET	-1
#endif


////////////////////////////////////////////////////////
//
// first, a bunch of stuff that needs to be defined here
// if nested classes worked right on pixelflow aCC, then
// this could probably be cleaned up some
//


// some forward declarations so we can avoid including
// a bunch of stuff at this point

// from vrpn_BaseConnectionController.h
class vrpn_ClientConnectionController;

// NOTE: most users will want to use the vrpn_Synchronized_Connection
// class rather than the default connection class (either will work,
// a regular connection just has 0 as the client-server time offset)

typedef vrpn_int32 (* vrpn_LOGFILTER) (void * userdata, vrpn_HANDLERPARAM p);

// Buffer lengths for TCP and UDP.
// TCP is an arbitrary number that can be changed by the user
// using vrpn_Connection::set_tcp_outbuf_size().
// UDP is set based on Ethernet maximum transmission size;  trying
// to send a message via UDP which is longer than the MTU of any
// intervening physical network may cause untraceable failures,
// so for now we do not expose any way to change the UDP output
// buffer size.  (MTU = 1500 bytes, - 28 bytes of IP+UDP header)

#define vrpn_CONNECTION_TCP_BUFLEN  (64000)
#define vrpn_CONNECTION_UDP_BUFLEN  (1472)

// This is the list of states that a connection can be in
// (possible values for status).
#define vrpn_CONNECTION_LISTEN			(1)
#define vrpn_CONNECTION_CONNECTED		(0)
#define vrpn_CONNECTION_TRYING_TO_CONNECT	(-1)
#define vrpn_CONNECTION_BROKEN			(-2)
#define vrpn_CONNECTION_DROPPED			(-3)

// System message types
#define vrpn_CONNECTION_SERVICE_DESCRIPTION  (-1)
#define vrpn_CONNECTION_TYPE_DESCRIPTION    (-2)
#define vrpn_CONNECTION_UDP_DESCRIPTION     (-3)
#define vrpn_CONNECTION_LOG_DESCRIPTION     (-4)
#define vrpn_CONNECTION_MCAST_DESCRIPTION   (-5)  
#define vrpn_CLIENT_MCAST_CAPABLE_REPLY     (-6)
#define vrpn_CLOCK_QUERY                    (-7)
#define vrpn_CLOCK_REPLY                    (-8)
#define	vrpn_CONNECTION_DISCONNECT_MESSAGE	(-9)

// What to log
#define vrpn_LOG_NONE               (0)
#define vrpn_LOG_INCOMING           (1<<0)
#define vrpn_LOG_OUTGOING           (1<<1)


#if !defined(_WIN32) || defined (__CYGWIN__)
#define SOCKET int
#endif


// Default port to listen on for a server
#define vrpn_DEFAULT_LISTEN_PORT_NO             (4500)

// If defined, will filter out messages:  if the remote side hasn't
// registered a type, messages of that type won't be sent over the
// link.
//#define vrpn_FILTER_MESSAGES

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
//   [juliano 7/99: I think this statement is true only if
//                  the nested class is self-referential]

struct vrpnLogFilterEntry {
    vrpn_LOGFILTER filter;   // routine to call
    void * userdata;         // passed along
    vrpnLogFilterEntry * next;
};



//**********************************************************************
//**  This section has been pulled from the "SDI" library and had its
//**  functions renamed to vrpn_ from sdi_.  This removes our dependence
//**  on libsdi.a for VRPN.

#ifdef sparc

// On capefear and swift, getdtablesize() isn't declared in unistd.h
// even though the man page says it should be.  Similarly, wait3()
// isn't declared in sys/{wait,time,resource}.h.
extern "C" {
extern int getdtablesize (void);
pid_t wait3 (int * statusp, int options, struct rusage * rusage);
}
#endif

/* On HP's, this defines how many possible open file descriptors can be
 * open at once.  This is what is returned by the getdtablesize() function
 * on other architectures. */
#ifdef  hpux
#define getdtablesize() MAXFUPLIM
#endif

#ifdef __hpux
#define getdtablesize() MAXFUPLIM
#endif

/* The version of rsh in /usr/local/bin is the AFS version that passes tokens
 * to the remote machine.  This will allow remote execution of anything you
 * can execute locally.  This is the default location from which to get rsh.
 * If the VRPN_RSH environment variable is set, that will be used as the full
 * path instead.  */
#ifdef  linux
#define RSH             (char *) "/usr/local/bin/ssh"
#else
#define RSH             (char *) "/usr/local/bin/rsh"
#endif

/* How long to wait for a UDP packet to cause a callback connection,
 * and how many times to retry. */
#define	UDP_CALL_TIMEOUT	(2)
#define	UDP_CALL_RETRIES	(5)

/* How long to wait for the server to connect, and how many times to wait
 * this long.  The death of the child is checked for between each of the
 * waits, in order to allow faster exit if the child quits before calling
 * back. */
#define SERVCOUNT       (20)
#define SERVWAIT        (120/SERVCOUNT)

#if 0

#define time_greater(t1,t2)     ( (t1.tv_sec > t2.tv_sec) || \
                                 ((t1.tv_sec == t2.tv_sec) && \
                                  (t1.tv_usec > t2.tv_usec)) )
#define time_add(t1,t2, tr)     { (tr).tv_sec = (t1).tv_sec + (t2).tv_sec ; \
                                  (tr).tv_usec = (t1).tv_usec + (t2).tv_usec ; \
                                  if ((tr).tv_usec >= 1000000L) { \
                                        (tr).tv_sec++; \
                                        (tr).tv_usec -= 1000000L; \
                                  } }
#define	time_subtract(t1,t2, tr) { (tr).tv_sec = (t1).tv_sec - (t2).tv_sec ; \
				   (tr).tv_usec = (t1).tv_usec - (t2).tv_usec ;\
				   if ((tr).tv_usec < 0) { \
					(tr).tv_sec--; \
					(tr).tv_usec += 1000000L; \
				   } }
#endif  // 0


//-------------------------------------------------------------------
// UTILITY FUNCTIONS
//-------------------------------------------------------------------

// opens a udp socket at specified port if possible. returns port
// number actually allocated
vrpn_int32 vrpn_open_udp_socket(vrpn_uint16* port);

// marshalls a message into the output buffer for a protocol socket
// this got move out of vrpn_Connection and made into a global function
// so that the multicast sender class could have access to it.
vrpn_uint32 vrpn_marshall_message (
    char * outbuf,vrpn_uint32 outbuf_size,
    vrpn_uint32 initial_out,
    vrpn_uint32 len, struct timeval time,
    vrpn_int32 type, vrpn_int32 sender,
    const char * buffer);

// 1hz sync connection by default, windowed over last three bounces 
// WARNING:  vrpn_get_connection_by_name() may not be thread safe.
vrpn_ClientConnectionController *
vrpn_get_connection_by_name(
    const char * cname,
    const char * local_logfile_name   = NULL,
    vrpn_int32   local_log_mode       = vrpn_LOG_NONE,
    const char * remote_logfile_name  = NULL,
    vrpn_int32   remote_log_mode      = vrpn_LOG_NONE,
    vrpn_float64 dFreq                = 1.0, 
    vrpn_int32   cSyncWindow          = 3);


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
vrpn_int32 vrpn_get_port_number (const char * hostspecifier);
char * vrpn_copy_rsh_program (const char * hostspecifier);
char * vrpn_copy_rsh_arguments (const char * hostspecifier);
int    vrpn_getmyIP(char *myIPchar, vrpn_int32 maxlen);

// Checks the buffer to see if it is a valid VRPN header cookie.
// Returns -1 on total mismatch,
// 1 on minor version mismatch or other acceptable difference,
// and 0 on exact match.
vrpn_int32 check_vrpn_cookie (const char * buffer);

// Returns the size of the magic cookie buffer, plus any alignment overhead.
vrpn_int32 vrpn_cookie_size ();

vrpn_int32 write_vrpn_cookie (
	char * buffer, 
	vrpn_int32 length, 
	vrpn_int32 remote_log_mode);

vrpn_int32 vrpn_noint_select(
    vrpn_int32 width,
    fd_set *readfds,
    fd_set *writefds,
    fd_set *exceptfds,
    struct timeval * timeout);

vrpn_int32 vrpn_noint_block_write (
    SOCKET outfile,
    const char buffer[],
    vrpn_int32 length);

vrpn_int32 vrpn_noint_block_read(
    SOCKET infile,
    char buffer[],
    vrpn_int32 length);

vrpn_int32 vrpn_noint_block_read_timeout(
    SOCKET infile,
    char buffer[],
    vrpn_int32 length,
    struct timeval *timeout);

#endif
