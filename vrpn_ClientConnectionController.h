#ifndef VRPN_CLIENTCONNECTIONCONTROLLER_INCLUDED
#define VRPN_CLIENTCONNECTIONCONTROLLER_INCLUDED



////////////////////////////////////////////////////////
//
// class vrpn_ClientConnectionController
//
// this class HAS-A single BaseConnection object that it
// communicates through
//
// beyond BaseConnectionController, it implements
//  * initiates a connection (active)
//  * reconnect attempts (server not yet running or restarting)
//  * client-side logging
//

#include "vrpn_BaseConnectionController.h"

// {{{ class vrpn_ClientConnectionController

class vrpn_ClientConnectionController
    : public vrpn_BaseConnectionController
{
    // {{{ c'tors and d'tors
public:

    // destructor ...XXX...
    virtual ~vrpn_ClientConnectionController();

    // constructors ...XXX...
    vrpn_ClientConnectionController(
		const char * cname, 
		vrpn_int16 port = vrpn_DEFAULT_LISTEN_PORT_NO,
		const char * local_logfile_name = NULL, 
		vrpn_int32 local_log_mode = vrpn_LOG_NONE,
		const char * remote_logfile_name = NULL, 
		vrpn_int32 remote_log_mode = vrpn_LOG_NONE,
		vrpn_int32 tcp_inbuflen = vrpn_CONNECTION_TCP_BUFLEN,
		vrpn_int32 tcp_outbuflen = vrpn_CONNECTION_TCP_BUFLEN,
		//vrpn_int32 udp_inbuflen = vrpn_CONNECTION_UDP_BUFLEN,
		vrpn_int32 udp_outbuflen = vrpn_CONNECTION_UDP_BUFLEN,
		vrpn_float64 dFreq = 4.0, 
		vrpn_int32 cSyncWindow = 2
		);

    // helper function
    // called by vrpn_get_connection_by_name()
    virtual int connect_to_server( const char *machine, int port );

    // called by NewFileController to get an interface to a FileConnection.
    //
    // XXX it would be nice if this were private for now, it's public.  it's
    // only needed by FileConnection for now, by may be needed by other stuff
    // in the future.  An alternative is to make it a friend.
    virtual vrpn_BaseConnection* get_BaseConnection()
    { return d_connection_ptr; }

    // }}} end c'tors and d'tors

public: // mainloop

    virtual vrpn_int32 mainloop( const timeval * timeout = NULL );

    // {{{ services and types

protected: // called by methods in the base class

    virtual void register_service_with_connections(
        const char * service_name, vrpn_int32 service_id );
    
    virtual void register_type_with_connections(
        const char * type_name, vrpn_int32 type_id );

    // }}} end services and types

public: // sending and receving

    // * send pending report (that have been packed), and clear the buffer
    // * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
    virtual vrpn_int32 send_pending_reports();

    
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

public: // logging

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
                                            void * userdata);

public: // status 

    // are there any connections?
    virtual /*bool*/vrpn_int32 at_least_one_open_connection() const;
    
    // overall, all connections are doing okay
    virtual /*bool*/vrpn_int32 all_connections_doing_okay() const;

    // number of connections
    virtual vrpn_int32 num_connections() const { return (d_connection_ptr ? 1 : 0); }

    // these are only needed by the Server, but they had to be
    // included in vrpn_ConnectionControllerCallbackInterface
    virtual void got_a_connection(void *) {}
    virtual void dropped_a_connection(void *) {}
    
private: // the connection
    vrpn_BaseConnection * d_connection_ptr;

    // {{{ clock synch functions

public:

    // do all inits that are done in
    // vrpn_Clock_Remote constructor
    void init_clock_client();

    // mostly code from vrpn_Clock_Remote::mainloop()
    void synchronize_clocks();

    // MANIPULATORS

    // (un)Register a callback to handle a clock sync
    virtual vrpn_int32 register_clock_sync_handler(
        void *userdata,
        vrpn_CLOCKSYNCHANDLER handler);
    virtual vrpn_int32 unregister_clock_sync_handler(
        void *userdata,
        vrpn_CLOCKSYNCHANDLER handler);
    // request a high accuracy sync (and turn off quick syncs)
    struct timeval fullSync ();


    // ACCESSORS


    // Returns the most recent RTT estimate.  TCH April 99
    // Returns 0 if the first RTT estimate is not yet completed
    // or if no quick syncs are being done.
    timeval currentRTT () const;

protected: // clock synch data members and funcs

    vrpn_int32 clockClient_id;             // vrpn id for this client

    // unique id for this particular client (to disambiguate replies)
    vrpn_int32 lUniqueID;

    // vars for stats on clock for quick sync
    vrpn_int32 fDoQuickSyncs;
    vrpn_int32 cQuickBounces;

    // CC does not like this, so ...
    // const int cMaxQuickDiffs=5
    // This is the number of reports the clock keeps track of and
    // chooses the offset from the min round trip from these.
    // This is a balance between drift compensation and
    // accuracy.  In the absence of drift, this should
    // be very large.  In the presence of drift it needs
    // to be very small. 5 seems to work well -- this means
    // that drift will accumulate for at most 5*1/freq secs,
    // and that a few consecutive long roundtrips will not
    // reduce the accuracy of the offset.
    vrpn_int32 cMaxQuickRecords;
    struct timeval *rgtvHalfRoundTrip;
    struct timeval *rgtvClockOffset;
    vrpn_int32 irgtvQuick;

    vrpn_float64 dQuickIntervalMsecs;
    struct timeval tvQuickLastSync;

    // vars for stats on clock for full sync
    vrpn_int32 fDoFullSync;
    vrpn_int32 cBounces;

#ifdef USE_REGRESSION
    vrpn_float64 *rgdOffsets;
    vrpn_float64 *rgdTimes;
#endif

    struct timeval tvMinHalfRoundTrip;

    // offset to report to user
    struct timeval tvFullClockOffset;

    // vars for user specified handlers
    struct vrpn_CLOCKSYNCLIST {
      void          *userdata;
      vrpn_CLOCKSYNCHANDLER handler;
      struct vrpn_CLOCKSYNCLIST    *next;
    };

    vrpn_CLOCKSYNCLIST  *change_list;

    static vrpn_int32 quickSyncClockServerReplyHandler(
		void *userdata, 
		vrpn_HANDLERPARAM p);
    static vrpn_int32 fullSyncClockServerReplyHandler(
		void *userdata,
		vrpn_HANDLERPARAM p);

    // }}} end clock synch

};

// }}} end class vrpn_ClientConnectionController


//
// vrpn_ConnectionControllerManager
//
// Singleton class that keeps track of all known VRPN
// ClientConnectionControllers and makes sure they're deleted on
// shutdown.  We make it static to guarantee that the destructor is
// called on program close so that the destructors of all the
// vrpn_Connections that have been allocated are called so that all
// open logs are flushed to disk.
//

//      This section holds data structures and functions to open
// connections by name.
//      The intention of this section is that it can open connections for
// objects that are in different libraries (trackers, buttons and sound),
// even if they all refer to the same connection.


class vrpn_ConnectionControllerManager {

  public:

    ~vrpn_ConnectionControllerManager (void);

    static vrpn_ConnectionControllerManager & instance (void);
      // The only way to get access to an instance of this class.
      // Guarantees that there is only one, global object.
      // Also guarantees that it will be constructed the first time
      // this function is called, and (hopefully?) destructed when
      // the program terminates.

    void addController (
        vrpn_ClientConnectionController *, 
        const char * name);
    void deleteController (vrpn_ClientConnectionController *);
      // NB implementation is not particularly efficient;  we expect
      // to have O(10) connections, not O(1000).

    vrpn_ClientConnectionController * getByName (const char * name);
      // Searches through d_kcList but NOT d_anonList
      // (Connections constructed with no name)

    vrpn_int32 numControllers();

  private:

    struct knownController {
      char name [1000];
      vrpn_ClientConnectionController * controller;
      knownController * next;
    };

    knownController * d_kcList;
      // named Controllers

    knownController * d_anonList;
      // unnamed (server) Controllers

    vrpn_ConnectionControllerManager (void);

    vrpn_ConnectionControllerManager (const vrpn_ConnectionControllerManager &);
      // copy constructor undefined to prevent instantiations

    static void deleteController (vrpn_ClientConnectionController *, knownController **);

    vrpn_int32 d_numControllers;
};

#endif

