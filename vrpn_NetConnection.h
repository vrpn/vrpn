////////////////////////////////////////////////////////
//
// class vrpn_NetConnection
//
// This class provides a single communication channel over the network.
// It encompasses a tcp connection, a udp connection, and a udp connection
// for receiving multicast messages. In addition to the various types of
// network connections, the NetConnection also provides logging 
// cpapbilities.
//
// NetConnection replaces the OneConnection class.

// NOTE:
// multicast will not be supported for the first release of the new
// connection system. all multicast funtonality will either be 
// commented out or rendered inactive by a setting d_mcast_capable
// to false dutring initialization of the system.
//
// Stefan Sain, 8/99

#ifndef VRPN_NETCONNECTION_INCLUDED
#define VRPN_NETCONNECTION_INCLUDED

#include "vrpn_ConnectionCommonStuff.h"
#include "vrpn_BaseConnection.h"
#include "vrpn_BaseConnectionManager.h"
#include "vrpn_UnreliableMulticastRecvr.h"
#include "vrpn_CommonSystemIncludes.h"

class vrpn_NetConnection
    : public vrpn_BaseConnection {

public:  // c'tors and d'tors

    vrpn_NetConnection(
        vrpn_BaseConnectionManager::RestrictedAccessToken *,
        const char*  local_logfile  = NULL,
        vrpn_int32   local_logmode  = vrpn_LOG_NONE,

        const char*  remote_logfile = NULL,
        vrpn_int32   remote_logmode = vrpn_LOG_NONE,

        vrpn_int32   tcp_inbuflen   = vrpn_CONNECTION_TCP_BUFLEN,
        vrpn_int32   tcp_outbuflen  = vrpn_CONNECTION_TCP_BUFLEN,
        // vrpn_int32   udp_inbuflen  = vrpn_CONNECTION_UDP_BUFLEN,
        vrpn_int32   udp_outbuflen  = vrpn_CONNECTION_UDP_BUFLEN
        // add multicast arguments later
        );
    
    virtual ~vrpn_NetConnection();
    
    
public:  // sending and receiving


    // Call each time through program main loop to handle receiving any
    // incoming messages and sending any packed messages.
    // Returns -1 when connection dropped due to error, 0 otherwise.
    // (only returns -1 once per connection drop).
    // Optional argument is TOTAL time to block on select() calls;
    // there may be multiple calls to select() per call to mainloop(),
    // and this timeout will be divided evenly between them.
    virtual vrpn_int32 mainloop (const struct timeval * timeout = NULL);

    // functions for sending messages and receiving messages
    // the ConnectionManager will call these functions
  
    vrpn_int32 queue_outgoing_message(
        vrpn_uint32 len, 
        struct timeval time,
        vrpn_int32 type, 
        vrpn_int32 sender, 
        const char * buffer,
        vrpn_uint32 class_of_service, 
        vrpn_bool sent_mcast);

    vrpn_int32 handle_incoming_messages( const struct timeval * pTimeout = NULL );

    static vrpn_int32 handle_incoming_udp_message(
        void * userdata, 
        vrpn_HANDLERPARAM p);


    // Pack a message that will be sent the next time mainloop() is called.
    // Turn off the RELIABLE flag if you want low-latency (UDP) send.
    virtual vrpn_int32 pack_message(
        vrpn_uint32 len, 
        struct timeval time,
        vrpn_int32 type, 
        vrpn_int32 sender, 
        const char * buffer,
        vrpn_uint32 class_of_service);

    // send pending report, clear the buffer.
    // This function was protected, now is public, so we can use it
    // to send out intermediate results without calling mainloop
    virtual vrpn_int32 send_pending_reports();

protected:  // sending and receiving

    // Routines to handle incoming messages on file descriptors
    // called by handle_incoming_message

    vrpn_int32 handle_udp_messages (vrpn_int32 fd, const struct timeval * timeout);
    vrpn_int32 handle_tcp_messages (vrpn_int32 fd, const struct timeval * timeout);
    vrpn_int32 handle_mcast_messages(/* XXX */);


    vrpn_int32 pack_service_description (char* name, vrpn_int32 id);
    vrpn_int32 pack_type_description (
        const vrpn_BaseConnectionManager::vrpnLocalMapping &,
        vrpn_int32 id);
    vrpn_int32 pack_udp_description (vrpn_uint16 portno);
    vrpn_int32 pack_log_description (vrpn_int32 mode, const char * filename);


public: // status

    // Routines that report status of connections
    vrpn_int32 outbound_udp_open () const { return udp_outbound != -1; }
    vrpn_int32 outbound_mcast_open () const
    { return mcast_recvr ? 1 : 0; } // if multicast recvr ptr non-null
    
    vrpn_bool connected() const
    { return status == vrpn_CONNECTION_CONNECTED; }
    vrpn_bool doing_okay() const { return status >= 0; }
    // get status of connection
    vrpn_int32 get_status() const { return status; }

public:  // buffer sizes. 

    // Applications that need to send very large messages may want to
    // know the buffer size used or to change its size.  Buffer size
    // is returned in bytes.
    vrpn_int32 tcp_outbuf_size () const { return d_tcp_outbuflen; }
    vrpn_int32 udp_outbuf_size () const { return d_udp_outbuflen; }

    // Allows the application to attempt to reallocate the buffer.
    // If allocation fails (error or out-of-memory), -1 will be returned.
    // On successful allocation or illegal input (negative size), the
    // size in bytes will be returned.  These routines are NOT guaranteed
    // to shrink the buffer, only to increase it or leave it the same.
    vrpn_int32 set_tcp_outbuf_size (vrpn_int32 bytecount);
    //XXX need to give defn


public: // setting up connections

    //----------------------------
    // server side

    // This is similar to check connection except that it can be
    // used to receive requests from before a server starts up
    // Returns the name of the service that the connection was first
    // constructed to talk to, or NULL if it was built as a server.
    //inline const char * name () const { return my_name; }
    virtual vrpn_int32 connect_to_client(const char* msg);

    // set up network
    vrpn_int32 mcast_reply_handler();
    vrpn_int32 pack_mcast_description(vrpn_int32 sender);
    vrpn_bool peer_mcast_capable(){return d_peer_mcast_capable;}
    void set_peer_mcast_capable(vrpn_bool capable){ d_peer_mcast_capable = capable;}

    //-----------------------------
    // client side


    virtual vrpn_int32 connect_to_server(const char* machine, vrpn_int16 port);
	virtual vrpn_int32 start_server(const char *machine, char *server_name, char *args);

    // setting up network
    vrpn_int32 mcast_description_handler(char* message);
    vrpn_int32 pack_mcast_reply(/* XXX */);
    vrpn_bool mcast_capable(){return d_mcast_capable;}
    void set_mcast_capable(vrpn_bool capable){ d_mcast_capable = capable;}

protected: // setting up connections

    //----------------------------
    // server side

    vrpn_int32 connect_udp_to(const char* machine, vrpn_int16 port);
    virtual vrpn_int32 connect_tcp_to(const char* msg);

    //-----------------------------
    // client side

    // formerly vrpn_udp_request_call
    int udp_request_connection_call(const char* machine, int port);
    int udp_request_lob_packet(
		const char * machine,
		const int remote_port,
		const int local_port);
    int get_a_TCP_socket(SOCKET *listen_sock, int *listen_portnum);
    int poll_for_accept(SOCKET listen_sock, SOCKET *accept_sock, double timeout = 0.0);

    //-----------------------------
    // common functions

    virtual void    handle_connection();
    // set up data
    virtual vrpn_int32  setup_new_connection (vrpn_int32 logmode = vrpn_LOG_NONE,
                                              const char * logfile = NULL);
    // tear down network
    virtual void    drop_connection ();
 
private: // data members
    
    // magic cookie so clients can make sure
    // mcast packet came from correct sender
    // contains pid and ip addr of server
	//
	// cookie used to determine if mcast message came from
	// who we expected it from. since we are using random
	// mulitcast addresses, we could theoretically get
	// messages from unwanted sources
    char d_mcast_cookie[8];

    // multicast capability flags
    vrpn_bool d_mcast_capable;
    vrpn_bool d_peer_mcast_capable;

    // Status of the connection
    vrpn_int32  status;  

    // mulitcast receiver object
    vrpn_UnreliableMulticastRecvr* mcast_recvr;

    // Output buffers storing messages to be sent
    // Convention:  use d_ to denote a data member of an object so
    // that inside a method you can recognize data members at a glance
    // and differentiate them from parameters or local variables.

    char *  d_tcp_outbuf;
    char *  d_udp_outbuf;
    vrpn_int32  d_tcp_outbuflen;  // total buffer size, in bytes
    vrpn_int32  d_udp_outbuflen;
    vrpn_int32  d_tcp_num_out;  // number of bytes currently used
    vrpn_int32  d_udp_num_out;


    // Input buffers
    // HACK:  d_TCPbuf uses malloc()/realloc()/free() instead of
    //   new/delete.
    vrpn_uint32 d_tcp_inbuflen;
    char * d_tcp_inbuf;
    char * d_udp_inbuf;
	char * d_mcast_inbuf;
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

};



#endif // VRPN_NETCONNECTION_INCLUDED
