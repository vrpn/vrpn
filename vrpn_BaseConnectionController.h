
#ifndef VRPN_BASECONNECTIONCONTROLLER_INCLUDED
#define VRPN_BASECONNECTIONCONTROLLER_INCLUDED

#include "vrpn_CommonSystemIncludes.h"
#include "vrpn_Shared.h"
#include "vrpn_ConnectionCommonStuff.h"
#include "vrpn_ConnectionControllerCallbackInterface.h"
//#include "vrpn_BaseConnection.h"
#include "vrpn_NewFileConnection.h"
#include "vrpn_NetConnection.h"

//
// In this file, we define the classes
//
//    vrpn_BaseConnectionController
//
// vrpn_ServerConnectionController and vrpn_ClientConnectionController are
// defined in their own files.
//
// For convenience, comments in this file will often drop the vrpn_ prefix.
//
// BaseConnectionController defines the portion of the interface that is
// common to both client and server applications.  ServerConnectionController
// and ClientConnectionController define additional functionality that is
// particular to each respective type of application.
//
// servers and client devices (like vrpn_TrackerRemote or a tracker
// server) interract with a ptr to BaseConnectionController.
//
// ClientConnectionController HAS-A single BaseConnection.
// ServerConnectionController HAS-A list of BaseConnections.
//
// Each BaseConnection is a communication
// link with a single client.  BaseConnection is an abstract
// base class.  One example of a concrete class that implements
// this interface is NetworkConnection.
//
// Beyond BaseConnectionController, ClientConnectionController implements:
//  * initiates a connection (active)
//  * reconnect attempts (server not yet running or restarting)
//  * client-side logging
//
// Beyond BaseConnectionController, ServerConnectionController implements:
//  * listens for connections (passive)
//  * multiple connections
//  * multicast sending
//  * more extensive logging
//
// (*ConnectionController) used to be called Connection.
// NetworkConnection used to be called OneConnection.
// BaseConnection is new.
//

// {{{ Local defs

//-------------------------------------------------
// defs & funcs used for clock synchronization and
// debugging
//-------------------------------------------------
struct vrpn_CLOCKCB {
    struct timeval msg_time;        // Local time of this sync message
    struct timeval tvClockOffset;   // Local time - remote time in msecs
    struct timeval tvHalfRoundTrip; // half of the roundtrip time
                                    // for the roundtrip used to calc offset
};

typedef void (*vrpn_CLOCKSYNCHANDLER)(void *userdata,
                                      const vrpn_CLOCKCB& info);

// Connect to a clock server that is on the other end of a connection
// and figure out the offset between the local and remote clocks
// This is the type of clock that user code will deal with.
// You need only supply the server name (no device name) and the 
// desired frequency (in hz) of clock sync updates.  IF THE FREQ IS
// NEGATIVE, THEN YOU WILL GET NO AUTOMATIC SYNCS. At any time you
// can get a very accurate sync by calling fullSync();

// special message identifiers for remote clock syncs
#define VRPN_CLOCK_FULL_SYNC 1
#define VRPN_CLOCK_QUICK_SYNC 2



// HACK
// These structs must be declared outside of vrpn_Connection
// (although we'd like to make them protected/private members)
// because aCC on PixelFlow doesn't handle nested classes correctly.
//   [juliano 7/99: I think this statement is true only if
//                  the nested class is self-referential]

// Description of a callback entry for a user type.
struct vrpn_MsgCallbackEntry {
    vrpn_MESSAGEHANDLER     handler;   // Routine to call
    void                  * userdata;  // Passed along
    vrpn_int32              sender;    // Only if from sender
    vrpn_MsgCallbackEntry  * next;      // Next handler
};


//  // This is the interface that we provide to the Connection
//  // classes so that they can invoke callbacks. The only function
//  // of the ConnectionControllers that we want the Connection
//  // classes to have access to is do_callbacks_for
//  struct vrpn_ConnectionControllerCallbackInterface
//  {
//      virtual vrpn_int32 do_callbacks_for( 
//  		vrpn_int32 type, 
//  		vrpn_int32 sender,
//  		timeval time, 
//  		vrpn_uint32 len,
//  		const char * buffer) = 0;

//  };

// }}}

// {{{ class vrpn_BaseConnectionController
////////////////////////////////////////////////////////
//
// class vrpn_BaseConnectionController
//
// this class used to be called vrpn_Connection it defines the
// interface that is common between ServerConnectionController and
// ClientConnectionController
//

class vrpn_BaseConnectionController: public vrpn_ConnectionControllerCallbackInterface
{
    // {{{ c'tors, and d'tors
public:

    // OLD COMMENT:
    //   No public constructors because ...
    //   Users should not create vrpn_Connection directly -- use 
    //   vrpn_Synchronized_Connection (for servers) or 
    //   vrpn_get_connection_by_name (for clients)

    // Destructor should delete all entries from callback lists
    // and all BaseConnection's
    virtual ~vrpn_BaseConnectionController();

protected:

    // constructor should include these for clock synch
    vrpn_BaseConnectionController(  
        const char *  local_logfile  = NULL,
        vrpn_int32    local_logmode  = vrpn_LOG_NONE,
        const char *  remote_logfile = NULL,
        vrpn_int32    remote_logmode = vrpn_LOG_NONE,
        vrpn_float64  _dFreq          = 4.0, 
        vrpn_int32    _cOffsetWindow  = 2);
    
    
    // }}}
    // {{{ status

protected:

    vrpn_int32 status;

public:

    // are there any connections?
    virtual /*bool*/vrpn_int32 at_least_one_open_connection() const = 0;
    
    // overall, all connections are doing okay
    virtual /*bool*/vrpn_int32 all_connections_doing_okay() const = 0;

    // number of connections
    virtual vrpn_int32 num_connections() const = 0;

    virtual void got_a_connection(void *) = 0;
    virtual void dropped_a_connection(void *) = 0; 
    
    // some way to get a list of open connection names
    // (one need is for the hiball control panel)

    // }}}
    // {{{ mainloop, incomming and outgoing messages
public:

    // * call each time through the program main loop
    // * handles receiving any incoming messages
    //   and sending any packed messages
    // * returns -1 when connection dropped due to error, 0 otherwise
    //   (only returns -1 once per connection drop)
    // * optional argument is TOTAL time to block on select() calls;
    //   there may be multiple calls to select() per call to mainloop(),
    //   and this timeout will be divided evenly between them.
    virtual vrpn_int32 mainloop( const timeval * timeout = NULL ) = 0;

    
    // * send pending report (that have been packed), and clear the buffer
    // * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
    virtual vrpn_int32 send_pending_reports() = 0;
    
    // * pack a message that will be sent the next time mainloop() is called
    // * turn off the RELIABLE flag if you want low-latency (UDP) send
    // * was: pack_message
    virtual vrpn_int32 queue_outgoing_message(
        vrpn_uint32 len, 
        timeval time,
        vrpn_int32 type,
        vrpn_int32 service,
        const char * buffer,
        vrpn_uint32 class_of_service ) = 0;

    // }}}
    // {{{ services and types

public:
    // * messages in vrpn are identified by two different ID's
    // * one is the service id.  It will be renamed.  An example is
    //   tracker0 or tracker1, button0 or button1, etc.
    // * the other ID is the type id.  This one speaks to how to
    //   decode the message.  Examples are ...XXX...
    // * the layer sitting above the connection (...XXX_name...)
    //   defines the types.  For example, vrpn_Tracker defines
    //   ...XXX_vel/pos/acc...
    // * when a client instantiates a vrpn_TrackerRemote,
    //   a connection is made and the pertainant types are registered
    // * the service name is part of the connection name, for example
    //   "tracker0@ioglab.cs.unc.edu".
    //
    // XXX todo
    // * check into why (in the old code) one of the register
    //   functions uses char * while the other uses char[100]
    // * make name const for both register functions

    // * get a token to use for the string name of the service
    // * remember to check for -1 meaning failure
    vrpn_int32 register_service( const char * service_name );

    // * get a token to use for the string name of the message type
    // * remember to check for -1 meaning failure
    vrpn_int32 register_message_type( const char * type_name );

    // * returns service ID, or -1 if unregistered
    vrpn_int32 get_service_id( const char * service_name ) const;
    
    // * returns message type ID, or -1 if unregistered
    // * was message_type_is_registered  
    vrpn_int32 get_message_type_id( const char * type_name ) const;

    // * returns the name of the specified service,
    //   or NULL if the parameter is invalid
    // * was: sender_name
    const char * get_service_name( vrpn_int32 service_id ) const;
    
    // * returns the name of the specified type,
    //   or NULL if the parameter is invalid
    // * only works for user messages (type_id >= 0)
    // * was: message_type_name
    const char * get_message_type_name( vrpn_int32 type_id ) const;

    // These are const, but the compiler says that type qualifiers
    // are meaningless here.
    vrpn_int32 get_num_my_types( void ){ return num_my_types; }
    vrpn_int32 get_num_my_services( void ){ return num_my_services; }

protected:  // these are called by the public functions above

    virtual void register_service_with_connections(
        const char * service_name, vrpn_int32 service_id ) = 0;
    
    virtual void register_type_with_connections(
        const char * type_name, vrpn_int32 type_id ) = 0;
    

protected: // changed from private because derived classes need acess
    // to these and no get/set functions have been written - XXX sain 10/

    // [jj: taken from old connection.h]
    // The senders we know about and the message types we know about
    // that have been declared by the local version.
    // cCares:  has the other side of this connection registered
    //   a type by this name?  Only used by filtering.
    
    struct vrpnLocalMapping {
        char *                  name;       // Name of type
        vrpn_MsgCallbackEntry *  who_cares;  // Callbacks
        vrpn_int32              cCares;     // TCH 28 Oct 97
    };
    char *              my_services [vrpn_CONNECTION_MAX_SERVICES];
    vrpn_int32          num_my_services;
    vrpnLocalMapping    my_types [vrpn_CONNECTION_MAX_TYPES];
    vrpn_int32          num_my_types;

    // }}}
    // {{{ callbacks

public:
    // * clients and servers register callbacks.  The callbacks are how they
    //   are informed about messages passing between the connections.
    // * when registering a callback, you indicte which message types
    //   and which services are pertinant to that callback.  Only messages
    //   with matching type/service IDs will cause the callback to be invoked
    // * vrpn_ANY_SERVICE and vrpn_ANY_TYPE exist as a way to say you want
    //   a callback to be invoked regardless of the type/service id


    // * set up a handler for messages of a given
    //   type_id and service_id
    // * may use vrpn_ANY_TYPE or vrpn_ANY_SERVICE or both
    // * handlers will be called during mainloop()
    // * your handler should return 0, or a communication error
    //   is assumed and the connection will be shut down
    virtual vrpn_int32 register_handler(
        vrpn_int32 type,
        vrpn_MESSAGEHANDLER handler,
        void *userdata,
        vrpn_int32 service = vrpn_ANY_SERVICE );

    // * remove a handler for messages of a given
    //   type_id and service_id
    // * in order for this function to succeed, a handler
    //   must already have been registered with identical
    //   values of these parameters
    virtual vrpn_int32 unregister_handler(
        vrpn_int32 type,
        vrpn_MESSAGEHANDLER handler,
        void *userdata,
        vrpn_int32 service = vrpn_ANY_SERVICE );
    
    // * invoke callbacks that have registered
    //   for messages with this (type and service)
    virtual vrpn_int32 do_callbacks_for(
        vrpn_int32 type, vrpn_int32 service,
        timeval time,
        vrpn_uint32 len, const char * buffer);

private:  // implementation of callbacks

    // Callbacks on vrpn_ANY_TYPE
    vrpn_MsgCallbackEntry * generic_callbacks;

    // }}}
    // {{{ logging and clock

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
    virtual vrpn_int32 register_log_filter (vrpn_LOGFILTER filter, 
                                            void * userdata) = 0;

    virtual vrpn_int32 get_local_logmode();
    virtual void get_local_logfile_name(char *);
    
    virtual vrpn_int32 get_remote_logmode();
    virtual void get_remote_logfile_name(char *);
    
private:
    
    char * d_local_logname;      // local name of file to write log to
    vrpn_int32 d_local_logmode;  // local logging: incoming, outgoing, or both
    
    char * d_remote_logname;     // remote name of file to write log to
    vrpn_int32 d_remote_logmode; // remote logging: incoming, outgoing, both
    
public:  // clock
    
    // offset of clocks on connected machines -- local - remote
    timeval       tvClockOffset;    
    timeval       start_time;

    vrpn_float64  dFreq;
    vrpn_int32    cOffsetWindow;

    vrpn_int32 clockServer_id;
    vrpn_int32 queryMsg_id;
    vrpn_int32 replyMsg_id;
    // }}}
    
};

// }}}

#endif
