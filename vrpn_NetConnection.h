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


#ifndef VRPN_NETCONNECTION_INCLUDED
#define VRPN_NETCONNECTION_INCLUDED

#include "vrpn_ConnectionOldCommonStuff.h"
#include "vrpn_BaseConnection.h"
#include "vrpn_UnreliableMulticastRecvr.h"
#include "vrpn_Shared.h"

class vrpn_NetConnection: public vrpn_BaseConnection {

public:  // c'tors and d'tors
    vrpn_NetConnection();   
    virtual ~vrpn_NetConnection();

protected: //init
    virtual void init(void);

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
    // the ConnectionController will call these functions
  
    vrpn_int32 handle_outgoing_messages( const struct timeval * pTimeout = NULL );

    vrpn_int32 handle_incoming_messages( const struct timeval * pTimeout = NULL );


    // Pack a message that will be sent the next time mainloop() is called.
    // Turn off the RELIABLE flag if you want low-latency (UDP) send.
    virtual vrpn_int32 pack_message(vrpn_uint32 len, struct timeval time,
        vrpn_int32 type, vrpn_int32 sender, const char * buffer,
        vrpn_uint32 class_of_service, vrpn_bool sent_mcast);

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

public: // status

    // Routines that report status of connections
    inline vrpn_int32 outbound_udp_open (void) const
      { return udp_outbound != -1; }
    inline vrpn_int32 outbound_mcast_open (void) const
      { return mcast_recvr ? 1 : 0; } // if multicast recvr ptr non-null

public:  // buffer sizes.  XXX don't know if these belong here or not
    //                         no, they don't
    // Applications that need to send very large messages may want to
    // know the buffer size used or to change its size.  Buffer size
    // is returned in bytes.
    vrpn_int32 tcp_outbuf_size () const { return d_tcp_buflen; }
    vrpn_int32 udp_outbuf_size () const { return d_udp_buflen; }

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

    // set up network
    vrpn_int32 handle_mcast_reply();
    vrpn_int32 pack_mcast_description(vrpn_int32 sender);
    inline vrpn_bool peer_mcast_capable(void){return d_peer_mcast_capable;}
    inline void set_peer_mcast_capable(vrpn_bool capable){ d_peer_mcast_capable = capable;}

    //-----------------------------
    // client side

    // formerly vrpn_udp_request_call
    vrpn_int32 udp_request_connection_call(const char* machine, vrpn_int16 port);
    vrpn_int32 open_udp_socket(vrpn_int16 port);

    // setting up network
    vrpn_int32 handle_mcast_description(char* message);
    vrpn_int32 pack_mcast_reply(/* XXX */);
    inline vrpn_bool mcast_capable(void){return d_mcast_capable};
    inline void set_mcast_capable(vrpn_bool capable){ d_mcast_capable = capable};


    //-----------------------------
    // common functions

    // i think that these are common functions. not sure yet.
    virtual void    handle_connection(void);
    // set up data
    virtual vrpn_int32  setup_new_connection (vrpn_int32 logmode = 0L,
                                              const char * logfile = NULL);
    // set up network
    virtual void    drop_connection (void);
    virtual vrpn_int32  pack_sender_description (vrpn_int32 which);
    virtual vrpn_int32  pack_type_description (vrpn_int32 which);
    virtual vrpn_int32  pack_udp_description (vrpn_int16 portno);
    virtual vrpn_int32  pack_log_description (vrpn_int32 mode, const char * filename);
 
private: // data members
    
    // magic cookie so clients can make sure
    // mcast packet came from correct sender
    // contains pid and ip addr of server
    char d_mcast_cookie[8];

    // multicast capability flags
    vrpn_bool d_mcast_capable;
    vrpn_bool d_peer_mcast_capable;

    // Status of the connection
    vrpn_int32  status;  

    /*
    // perhaps we should move to enums 
    //typedef and move to header
    enum ConnectionStatus = { CONNECTED, CONNECTION_FAIL, BROKEN, DROPPED };
    ConnectionStatus status;
    */


    // temporary name
    vrpn_UnreliableMulticastRecvr* mcast_recvr;

    // Output buffers storing messages to be sent
    // Convention:  use d_ to denote a data member of an object so
    // that inside a method you can recognize data members at a glance
    // and differentiate them from parameters or local variables.

    char *  d_tcp_outbuf;
    char *  d_udp_outbuf;
    vrpn_int32  d_tcp_buflen;  // total buffer size, in bytes
    vrpn_int32  d_udp_buflen;
    vrpn_int32  d_tcp_num_out;  // number of bytes currently used
    vrpn_int32  d_udp_num_out;


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
        char        * name;
        vrpn_int32  local_id;
    };

    // The senders and types we know about that have been described by
    // the other end of the connection.  Also, record the local mapping
    // for ones that have been described with the same name locally.
    // The arrays are indexed by the ID from the other side, and store
    // the name and local ID that corresponds to each.
    cRemoteMapping  other_senders [vrpn_CONNECTION_MAX_SENDERS];
    vrpn_int32  num_other_senders;
    cRemoteMapping  other_types [vrpn_CONNECTION_MAX_TYPES];
    vrpn_int32  num_other_types;

    // logging data memnbers, passed to FileLogger
    FileLogger* logger;


    vrpn_LOGLIST * d_logbuffer;  // last entry in log
    vrpn_LOGLIST * d_first_log_entry;  // first entry in log
    char * d_logname;            // name of file to write log to
    vrpn_int32 d_logmode;              // logging incoming, outgoing, or both
    
    vrpn_in32 d_logfile_handle;
    FILE * d_logfile;
    vrpnLogFilterEntry *d_logfilters;


};



#endif VRPN_NETCONNECTION_INCLUDED
