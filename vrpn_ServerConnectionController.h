#ifndef VRPN_SERVERCONNECTIONCONTROLLER_INCLUDED
#define VRPN_SERVERCONNECTIONCONTROLLER_INCLUDED

////////////////////////////////////////////////////////
//
// class vrpn_ServerConnectionController
//
// this class HAS-A list of BaseConnection objects that it
// communicates through
//
// beyond BaseConnectionController, it implements
//  * listens for connections (passive)
//  * multiple connections
//  * multicast sending
//

// NOTE:
// multicast will not be supported for the first release of the new
// connection system. all multicast funtonality will either be 
// commented out or rendered inactive by a setting d_mcast_capable
// to false dutring initialization of the system.
//
// Stefan Sain, 8/99

#include "vrpn_BaseConnectionController.h"

class vrpn_ServerConnectionController
    : public vrpn_BaseConnectionController
{
public:  // c'tors and d'tors

    // destructor ...XXX...
    virtual ~vrpn_ServerConnectionController();
        
protected:  // c'tors and init
        
    // constructors ...XXX...
    vrpn_ServerConnectionController(
        vrpn_uint16   port                = vrpn_DEFAULT_LISTEN_PORT_NO,
        char *        local_logfile_name  = NULL, 
        vrpn_int32    local_log_mode      = vrpn_LOG_NONE,
        vrpn_int32    tcp_inbuflen        = vrpn_CONNECTION_TCP_BUFLEN,
        vrpn_int32    tcp_outbuflen       = vrpn_CONNECTION_TCP_BUFLEN,
        vrpn_int32    udp_inbuflen        = vrpn_CONNECTION_UDP_BUFLEN,
        vrpn_int32    udp_outbuflen       = vrpn_CONNECTION_UDP_BUFLEN,
        vrpn_float64  dFreq               = 4.0, 
        vrpn_in32     cSyncWindow         = 2
        // add multicast arguments later
        );
    
public: // sending and receving messages
    
    // * call each time through the program main loop
    // * handles receiving any incoming messages
    //   and sending any packed messages
    // * returns -1 when connection dropped due to error, 0 otherwise
    //   (only returns -1 once per connection drop)
    // * optional argument is TOTAL time to block on select() calls;
    //   there may be multiple calls to select() per call to mainloop(),
    //   and this timeout will be divided evenly between them.
    virtual vrpn_int32 mainloop( const timeval * timeout = NULL );

    
    // * pack a message that will be sent the next time mainloop() is called
    // * turn off the RELIABLE flag if you want low-latency (UDP) send
    // * was: pack_message
    virtual vrpn_int32 handle_outgoing_message(
        vrpn_uint32 len, 
        timeval time,
        vrpn_int32 type,
        vrpn_int32 service,
        const char * buffer,
        vrpn_uint32 class_of_service );


private: // the connections

	vrpn_CONNECTION_LIST *d_connection_list = NULL;

protected: // initializaton and connection setup

    // listen for and process incoming connections on well known port
    // might have equivalent in Connection
    void listen_for_incoming_connections(const struct timeval * pTimeout);
    
    // get mcast group info from mcast sender
    char* get_mcast_info();
    
    // helper function
    virtual vrpn_int32 connect_to_client( const char *machine,
                                          vrpn_int16 port );
    
private: // data members
    
    char* d_mcast_info;                 // stores mcast group info
    vrpn_int32  status;                 // Status of the connection
    
    /*
      // perhaps we should move to enums
      enum ControllerStatus = { NOTCONNECTED, CONNECTED, ERROR };
      ControllerStatus status;
    */
    
    vrpn_int32  num_live_connections;
    
    // Only used for a vrpn_Connection that awaits incoming connections
    vrpn_int32  listen_udp_sock;        // Connect requests come here
    vrpn_uint16 listen_port_no;
    
    // logging object
    vrpn_FileLogger* d_logger_ptr;
    
    // data needed to pass to Connection c'tor
    vrpn_int32 d_tcp_inbuflen;
    vrpn_int32 d_tcp_outbuflen;
    vrpn_int32 d_udp_inbuflen;
    vrpn_int32 d_udp_outbuflen;
    
    
public: // logging functions
    
    virtual vrpn_int32 get_remote_logmode();
    virtual void get_remote_logfile_name(char *);

public: // clock server functions

    static vrpn_int32 clockQueryHandler( void *userdata, vrpn_HANDLERPARAM p );

protected: // clock server function

    virtual vrpn_int32 encode_to(char *buf, const struct timeval& tvSRep, 
                                 const struct timeval& tvCReq, 
                                 vrpn_int32 cChars, const char* pch);

};

#endif
