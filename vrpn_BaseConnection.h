////////////////////////////////////////////////////////
//
// class vrpn_BaseConnection
//
// base class for classes that provide a single connection,
// like vrpn_NetConnection.
//
// this class used to be called vrpn_OneConnection.
// now, it is an abstract base class for different kinds
// of connections, including a NetConnection and 
// probably a FileConnection.


#ifndef VRPN_BASECONNECTION_INCLUDED
#define VRPN_BASECONNECTION_INCLUDED

#include "vrpn_CommonSystemIncludes.h"
#include "vrpn_ConnectionCommonStuff.h"
#include "vrpn_FileLogger.h"
#include "vrpn_BaseConnectionController.h"


#ifndef VRPN_HAVE_DYNAMIC_CAST
class vrpn_NewFileConnection;
#endif

class vrpn_BaseConnection
{
public:  // c'tors and d'tors
    vrpn_BaseConnection(
        vrpn_BaseConnectionController::RestrictedAccessToken *,

        const char * local_logfile_name  = NULL,
        vrpn_int32   local_log_mode      = vrpn_LOG_NONE,

        const char * remote_logfile_name = NULL,
        vrpn_int32   remote_logmode      = vrpn_LOG_NONE
        );
    
    virtual ~vrpn_BaseConnection();

public:  // status
    
    // a connection was made
    virtual vrpn_bool connected() const = 0;

    // no errors
    virtual vrpn_bool doing_okay() const = 0;

    // get status of connection
    virtual vrpn_int32 get_status() const = 0;

public: // clock stuff
    virtual vrpn_int32 time_since_connection_open( timeval * elapsed_time );

    // has a clock sync occured yet (slight prob of false negative, but
    // only for a brief period)
    virtual vrpn_int32 clockSynched();

protected: // clock stuff
    timeval tvClockOffset;

public: // initiating connection
    // these functions are only used by NetConnection for now, but are in here
    // so that they can be invoke w/ a ptr to BaseConnection

    virtual vrpn_int32 connect_to_server(
        const char* machine, 
        vrpn_int16  port)
    {
        return 0;
    }
    
    virtual vrpn_int32 start_server(
        const char* machine,
        char*       server_name, 
        char*       args)
    {
        return 0;
    }

    // This is similar to check connection except that it can be
    // used to receive requests from before a server starts up
    // Returns the name of the service that the connection was first
    // constructed to talk to, or NULL if it was built as a server.
    //inline const char * name () const { return my_name; }
    virtual vrpn_int32 connect_to_client(const char* msg) { return 0; }

public:  // sending and receiving

    // Call each time through program main loop to handle receiving any
    // incoming messages and sending any packed messages.
    // Returns -1 when connection dropped due to error, 0 otherwise.
    // (only returns -1 once per connection drop).
    // Optional argument is TOTAL time to block on select() calls;
    // there may be multiple calls to select() per call to mainloop(),
    // and this timeout will be divided evenly between them.
    virtual vrpn_int32 mainloop (const struct timeval * timeout = NULL) = 0;
    
    // functions for sending messages
    // the ConnectionController calls it
    virtual vrpn_int32 queue_outgoing_message(
        vrpn_uint32 len, struct timeval time,
        vrpn_int32 type, vrpn_int32 sender, const char * buffer,
        vrpn_uint32 class_of_service, vrpn_bool sent_mcast ) = 0;
    
    // functions for receiving messages
    // the ConnectionController calls it
    virtual vrpn_int32 handle_incoming_messages( const struct timeval * pTimeout = NULL  ) = 0;

    
    // * send pending report (that have been packed), and clear the buffer
    // * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
    virtual vrpn_int32 send_pending_reports() = 0;

    // These functions are called during connection setup to exchange settings
    // and service/ype/descriptions
    // They return zero so that derived classes such as FileConnection can 
    // inherit default behavior. 
    
    virtual vrpn_int32 pack_service_description (char* name, vrpn_int32 id);
    virtual vrpn_int32 pack_type_description (
        const vrpn_BaseConnectionController::vrpnLocalMapping &,
        vrpn_int32 id);
    virtual vrpn_int32 pack_udp_description (vrpn_uint16 portno) {return 0;}
    virtual vrpn_int32 pack_log_description (vrpn_int32 mode,
                                             const char * filename ) {return 0;}

public:  // public type_id and service_id stuff

    // * register a new local {type,service} that that controller
    //   has assigned a {type,service}_id to.
    // * in addition, look to see if this {type,service} has
    //   already been registered remotely (newRemoteType/Service)
    // * if so, record the correspondence so that
    //   local_{type,service}_id() can do its thing
    // * XXX proposed new name:
    //         register_local_{type,service}
    //
    //Return 1 if this {type,service} was already registered
    //by the other side, 0 if not.

    // was: newLocalSender
    virtual vrpn_int32 register_local_service(
        const char * service_name,   // e.g. "tracker0"
        vrpn_int32   local_id);      // from controller
    
    // was: newLocalType
    virtual vrpn_int32 register_local_type(
        const char * type_name,   // e.g. "tracker_pos"
        vrpn_int32   local_id);   // from controller
    

    // Adds a new remote type/service and returns its index.
    // Returns -1 on error.
    // * called by the ConnectionController when the peer on the
    //   other side of this connection has sent notification
    //   that it has registered a new type/service
    // * don't call this function if the type/service has
    //   already been locally registered
    // * XXX proposed new name:
    //         register_remote_{type,service}
    
    // Adds a new remote type/service and returns its index
    // was: newRemoteSender
    virtual vrpn_int32 register_remote_service(
        const cName service_name,  // e.g. "tracker0"
        vrpn_int32 local_id );    // from controller
        
    // Adds a new remote type/service and returns its index
    // was: newRemoteType
    virtual vrpn_int32 register_remote_type(
        const cName type_name,    // e.g. "tracker_pos"
        vrpn_int32 local_id );    // from controller


    // Give the local mapping for the remote type or service.
    // Returns -1 if there isn't one.
    // Pre: have already registered this type/service remotely
    //      and locally using register_local_{type,service}
    //      and register_remote_{type_service}
    // * XXX proposed new name:
    //         translate_remote_{type,service}_to_local


    // Give the local mapping for the remote type
    // was: local_type_id
    virtual vrpn_int32 translate_remote_type_to_local(
        vrpn_int32 remote_type );
    
    // Give the local mapping for the remote service
    // was: local_sender_id
    virtual vrpn_int32 translate_remote_service_to_local(
        vrpn_int32 remote_service );


    // XXX todo
    // * check into why one of the register functions
    //   uses char * while the other uses char[100]
    // * make name const for both register functions

protected: // protected type_id and service_id stuff

    vrpn_BaseConnectionController::RestrictedAccessToken* const d_controller_token;

    // Holds one entry for a mapping of remote strings to local IDs
    struct cRemoteMapping {
        // remote/local service/type equivalence
        // is based on name string comparison
        char * name;
        
        // each side assigns an id to each name
        // the id's may differ on the two ends of a connection
        vrpn_int32 local_id;
    };
    
    
    // * the number of services that have been registered by the
    //   other side of the connection
    // * was: num_other_senders
    vrpn_int32 num_registered_remote_services;

    // * The services we know about that have been described by
    //   the other end of the connection.
    // * indexed by the ID from the other side, and store
    //   the name and local ID that corresponds to each.
    // * was: other_senders
    cRemoteMapping registered_remote_services[vrpn_CONNECTION_MAX_SERVICES];
        

    // * the number of types that have been registered by the
    //   other side of the connection
    // * was: num_other_types
    vrpn_int32 num_registered_remote_types;
    
    // * The types we know about that have been described by
    //   the other end of the connection.
    // * indexed by the ID from the other side, and store
    //   the name and local ID that corresponds to each.
    // * was: other_types
    cRemoteMapping registered_remote_types[vrpn_CONNECTION_MAX_TYPES];



protected:  // handling incoming and outgoing messages
    //         wrappers that forwards the functioncall to each open connection

    // Name of the remote host we are connected to.  This is kept for
    // informational purposes.  It is printed by the ceiling server,
    // for example.
    char rhostname [150];


    // Routines that handle system messages
    // these are registered as callbacks
    // handle_incoming_udp_message is declared as virtual because
    // original contained variables and functions  specific to
    // vrpn_NetConnection
    static vrpn_int32 handle_incoming_service_message(
        void * userdata, vrpn_HANDLERPARAM p);
    static vrpn_int32 handle_incoming_type_message(
        void * userdata, vrpn_HANDLERPARAM p);
    static vrpn_int32 handle_incoming_log_message(
        void * userdata, vrpn_HANDLERPARAM p);

    // Pointers to the handlers for system messages
	vrpn_MESSAGEHANDLER	system_messages [vrpn_CONNECTION_MAX_TYPES];

public: // logging functions

    // This calls a function by the same name in FileLogger
    //
    // Sets up a filter
    // function for logging.  Any user message to be logged is first
    // passed to this function, and will only be logged if the
    // function returns zero (XXX).  NOTE: this only affects local
    // logging - remote logging is unfiltered!  Only user messages are
    // filtered; all system messages are logged.  Returns nonzero on
    // failure.
    virtual vrpn_int32 register_log_filter (vrpn_LOGFILTER filter, 
                                            void * userdata);

protected: // logging function

    // we will support three different kinds of logging
    // the interface to logging needs to be enriched
    // in order to facilitate this
    //
    // what kinds of logging go here, and with what interface?

    virtual vrpn_int32 open_log();
    virtual vrpn_int32 close_log();
    
protected: // logging data members

    // logging object
    vrpn_FileLogger* d_logger_ptr;

    char * d_local_logname;
    vrpn_int32 d_local_logmode;
    char * d_remote_logname;
    vrpn_int32 d_remote_logmode;
    
#ifndef VRPN_HAVE_DYNAMIC_CAST
public:
    // this is a temporary measure until you can assume
    // dynamic_cast in all compilers
    virtual vrpn_NewFileConnection* get_FileConnectionPtr()
    {
        // override only in vrpn_NewFileConnection, where it returns 'this'
        return 0;
    }
#endif
    
public: // todo items
    
    // * some way to querry the name of a connection, for display
    //   purposes in the hiball control panel, etc.
    
};


//////////////////////////////////////////////
// List of connections that are already open
//  // 
//  struct vrpn_CONNECTION_LIST {
//  	char	name[1000];	// The name of the connection
//  	vrpn_BaseConnection	*c;	// The connection
//  	struct vrpn_CONNECTION_LIST *next;	// Next on the list
//  };

#endif
