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
#include "vrpn_UnreliableMulticastSender.h"

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
        vrpn_int32    cSyncWindow         = 2 );
        // add multicast arguments later
    
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
    virtual vrpn_int32 queue_outgoing_message(
        vrpn_uint32 len, 
        timeval time,
        vrpn_int32 type,
        vrpn_int32 service,
        const char * buffer,
        vrpn_uint32 class_of_service );

    // * send pending report (that have been packed), and clear the buffer
    // * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
    virtual vrpn_int32 send_pending_reports();

public: // status

    // are there any connections?
    virtual /*bool*/vrpn_int32 at_least_one_open_connection() const;
    
    // overall, all connections are doing okay
    virtual /*bool*/vrpn_int32 all_connections_doing_okay() const;

    // number of connections
    virtual vrpn_int32 num_connections() const;

    virtual void got_a_connection(void *);
    virtual void dropped_a_connection(void *); 


private: // the connections
    // XXX - this is now handled by vrpn_ConnectionManager
	// vrpn_ConnectionManager *d_connection_list;

protected: // initializaton and connection setup

    // listen for and process incoming connections on well known port
    // might have equivalent in Connection
    void listen_for_incoming_connections(const timeval * pTimeout);
    
    // get mcast group info from mcast sender
    char* get_mcast_info();
    
    // helper function
    virtual vrpn_int32 connect_to_client( const char *machine,
                                          vrpn_int16 port );
    
private: // data members

	// mulitcast 
    char* d_mcast_info;                 // stores mcast group info
	vrpn_UnreliableMulticastSender *d_mcast_sender;

    vrpn_int32  status;                 // Status of the connection
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
    

    // This calls a function by the same name in BaseConnection, wich
    // calls the same named function in FileLogger
    //
    // Sets up a filter
    // function for logging.  Any user message to be logged is first
    // passed to this function, and will only be logged if the
    // function returns zero (XXX).  NOTE: this only affects local
    // logging - remote logging is unfiltered!  Only user messages are
    // filtered; all system messages are logged.  Returns nonzero on
    // failure.
    virtual vrpn_int32 register_log_filter (
        vrpn_LOGFILTER filter, 
        void * userdata);

    // this function registers log filters with the
    // ServerConnectionController's FileLogger object
    virtual vrpn_int32 register_server_side_log_filter (
        vrpn_LOGFILTER filter, 
        void * userdata);

    virtual vrpn_int32 get_remote_logmode();
    virtual void get_remote_logfile_name(char *);

public: // clock server functions

    static vrpn_int32 clockQueryHandler( void *userdata, vrpn_HANDLERPARAM p );

protected: // clock server function

    virtual vrpn_int32 encode_to(char *buf, const timeval& tvSRep, 
                                 const timeval& tvCReq, 
                                 vrpn_int32 cChars, const char* pch);

};


//
// vrpn_ConnectionManager
// Singleton class that keeps track of all known VRPN connections
// and makes sure they're deleted on shutdown.
// We make it static to guarantee that the destructor is called
// on program close so that the destructors of all the vrpn_Connections
// that have been allocated are called so that all open logs are flushed
// to disk.
//

//      This section holds data structures and functions to open
// connections by name.
//      The intention of this section is that it can open connections for
// objects that are in different libraries (trackers, buttons and sound),
// even if they all refer to the same connection.

// forward declaration
class vrpn_ConnectionItr;

class vrpn_ConnectionManager {

    friend class vrpn_ConnectionItr;

  public:

    ~vrpn_ConnectionManager (void);

    static vrpn_ConnectionManager & instance (void);
      // The only way to get access to an instance of this class.
      // Guarantees that there is only one, global object.
      // Also guarantees that it will be constructed the first time
      // this function is called, and (hopefully?) destructed when
      // the program terminates.


    void addConnection (vrpn_BaseConnection *, const char * name);
    void deleteConnection (vrpn_BaseConnection *);
      // NB implementation is not particularly efficient;  we expect
      // to have O(10) connections, not O(1000).

    //vrpn_BaseConnection * getByName (const char * name);
      // Searches through d_kcList but NOT d_anonList
      // (Connections constructed with no name)

    vrpn_bool isEmpty( ) const { return d_anonList == NULL; }
    vrpn_int32 numConnections();

  private:

    struct knownConnection {
      char name [1000];
      vrpn_BaseConnection * connection;
      knownConnection * next;
    };

    //knownConnection * d_kcList;
      // named connections

    knownConnection * d_anonList;
      // unnamed (server) connections

    vrpn_int32 d_numConnections;

    vrpn_ConnectionManager (void);

    vrpn_ConnectionManager (const vrpn_ConnectionManager &);
      // copy constructor undefined to prevent instantiations

    static void deleteConnection (vrpn_BaseConnection *, knownConnection **);

}; // class vrpn_ConnectionManager


// vrpn_ConnectionItr class interface; maintains "current position"
//
// CONSTRUCTION: with (a) vrpn_ConnectionManager to which
// vrpn_ConnectionItr is permanently bound or (b) another
// vrpn_ConnectionItr; Copying of vrpn_ConnectionItr objects not
// supported in current form
//
// ******************PUBLIC OPERATIONS*********************
// void First( )          --> Set current position to first
// void operator++        --> Advance (both prefix and postfix)
// int operator+( )       --> True if at valid position in list
// vrpn_knownConnection 
//       operator( )      --> Return item in current position


class vrpn_ConnectionItr
{
  public:
    vrpn_ConnectionItr( const vrpn_ConnectionManager & CM ) : d_header( CM.d_anonList )
            { d_current = CM.isEmpty( ) ? NULL : d_header; }
    ~vrpn_ConnectionItr( ) { }

    // Returns 1 if d_current is not NULL or d_header, 0 otherwise
    vrpn_bool operator+( ) const { return d_current && d_current != d_header; }

    // Returns the Element in Current node
    // Errors: If Current is pointing at a node in the list
    vrpn_BaseConnection * operator( ) ( ) { return d_current->connection; }

    // Set d_current to first node in the list
    // set to NULL If List is empty
    void First( )  { d_current = d_header; }

    // Set d_current to d_current->Next; no return value
    // Errors: If d_current is NULL on entry
    void operator++( ) { d_current = d_current->next; }
    void operator++( int ) { operator++( ); }

  private:
    vrpn_ConnectionManager::knownConnection * const d_header;   // List Header
    vrpn_ConnectionManager::knownConnection *d_current;         // Current position

};


#endif
