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

//#include "vrpn_ConnectionOldCommonStuff.h"
#include "vrpn_ConnectionCommonStuff.h"
#include "vrpn_BaseConnectionController.h"

class vrpn_BaseConnection
{
public:  // c'tors and d'tors
    vrpn_BaseConnection(
        ConnectionControllerCallbackInterface* ccci,
        char * local_logfile_name = NULL,
        vrpn_int32 local_log_mode = vrpn_LOG_NONE,
        char * remote_logfile_name = NULL,
        vrpn_int32 remote_logmode = vrpn_LOG_NONE
        );
    
    virtual ~vrpn_BaseConnection();

protected: // init
    virtual void init(); // don't think we need init anymore
   
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
    virtual vrpn_int32 handle_outgoing_messages(
        vrpn_uint32 len, struct timeval time,
        vrpn_int32 type, vrpn_int32 sender, const char * buffer,
        vrpn_uint32 class_of_service, vrpn_bool sent_mcast ) = 0;

    // functions for receiving messages
    // the ConnectionController calls it
    virtual vrpn_int32 handle_incoming_messages( const struct timeval * pTimeout = NULL  ) = 0;

    // {{{ services and types

public:  // public type_id and service_id stuff

    // * register a new local {type,service} that that controller
    //   has assigned a {type,service}_id to.
    // * in addition, look to see if this {type,sender} has
    //   already been registered remotely (newRemoteType/Sender)
    // * if so, record the correspondence so that
    //   local_{type,sender}_id() can do its thing
    // * send the {type,sender} to the other side of the connection
    //
    // * Return 1 if this {type,sender} was already registered
    //   by the other side, 0 if not.

    vrpn_int32 register_local_service(
        const char *service_name,  // e.g. "tracker0"
        vrpn_int32 local_id );    // from controller
    
    vrpn_int32 register_local_type(
        const char *type_name,   // e.g. "tracker_pos"
        vrpn_int32 local_id );   // from controller
    
    // Give the local mapping for the remote type
    // Returns -1 if there isn't one.
    // was: local_type_id
    vrpn_int32 translate_remote_type_to_local(
        vrpn_int32 remote_type );
    
    // Give the local mapping for the remote service
    // Returns -1 if there isn't one.
    // was: local_sender_id
    vrpn_int32 translate_remote_service_to_local(
        vrpn_int32 remote_service );

protected: // protected type_id and service_id stuff
           // [jj] I think this goes here

    // Adds a new remote sender and returns its index.
    // * called when the peer on the other side of this
    //   connection has sent notification of a new sender
    // * Returns -1 on error, the id index on success
    vrpn_int32 register_remote_service(
        const cName service_name,  // e.g. "tracker0"
        vrpn_int32 local_id );    // from controller
        
    // Adds a new remote type and returns its index.
    // * called when the peer on the other side of this
    //   connection has sent notification of a new type
    // * Returns -1 on error, the id index on success
    vrpn_int32 register_remote_type(
        const cName type_name,    // e.g. "tracker_pos"
        vrpn_int32 local_id );    // from controller
    

    ConnectionControllerCallbackInterface* d_callback_interface_ptr;
    
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

// }}}


protected:  // handling incoming and outgoing messages
    //         wrappers that forwards the functioncall to each open connection

    // Name of the remote host we are connected to.  This is kept for
    // informational purposes.  It is printed by the ceiling server,
    // for example.
    char rhostname [150];

    virtual vrpn_int32 pack_service_description( vrpn_int32 which_service ) = 0;
    virtual vrpn_int32 pack_type_description( vrpn_int32 which_type ) = 0;
    virtual vrpn_int32 pack_udp_description( vrpn_uint16 portno ) = 0;
    virtual vrpn_int32 pack_log_description( vrpn_int32 mode,
                                             const char * filename );

    // Routines that handle system messages
    // these are registered as callbacks
    static vrpn_int32 handle_incoming_service_message(
        void * userdata, vrpn_HANDLERPARAM p);
    static vrpn_int32 handle_incoming_type_message(
        void * userdata, vrpn_HANDLERPARAM p);
    static vrpn_int32 handle_incoming_udp_message(
        void * userdata, vrpn_HANDLERPARAM p);
    static vrpn_int32 handle_incoming_log_message(
        void * userdata, vrpn_HANDLERPARAM p);


protected: // logging stuff

    // we will support three different kinds of logging
    // the interface to logging needs to be enriched
    // in order to facilitate this
    //
    // what kinds of logging go here, and with what interface?

	virtual vrpn_int32 open_log() = 0;
	virtual vrpn_int32 close_log() = 0;

protected: // logging data members

	char * d_local_logname;
	vrpn_int32 d_local_logmode;
	char * d_remote_logname;
	vrpn_int32 d_remote_logmode;

public: // todo items

    // * some way to querry the name of a connection, for display
    //   purposes in the hiball control panel, etc.

};


#endif
