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

class vrpn_ClientConnectionController
    : vrpn_BaseConnectionController
{
public:  // c'tors and d'tors

    // destructor ...XXX...
    virtual ~vrpn_ClientConnectionController();

protected:  // c'tors

    // constructors ...XXX...
    vrpn_ClientConnectionController(
		char * cname, 
		vrpn_uint16 port = vrpn_DEFAULT_LISTEN_PORT_NO,
		char * local_logfile_name = NULL, 
		vrpn_int32 local_log_mode = vrpn_LOG_NONE,
		char * remote_logfile_name = NULL, 
		vrpn_int32 remote_log_mode = vrpn_LOG_NONE,
		vrpn_int32 tcp_inbuflen = vrpn_CONNECTION_TCP_BUFLEN,
		vrpn_int32 tcp_outbuflen = vrpn_CONNECTION_TCP_BUFLEN,
		vrpn_int32 udp_inbuflen = vrpn_CONNECTION_UDP_BUFLEN,
		vrpn_int32 udp_outbuflen = vrpn_CONNECTION_UDP_BUFLEN,
		vrpn_float64 dFreq = 4.0, 
		vrpn_in32 cSyncWindow = 2
		);

    // helper function
    virtual int connect_to_server( const char *machine, int port );

public: // mainloop

    virtual vrpn_int32 mainloop( const timeval * timeout = NULL );

public: // status 

    // are there any connections?
    virtual /*bool*/vrpn_int32 at_least_one_open_connection() const;
    
    // overall, all connections are doing okay
    virtual /*bool*/vrpn_int32 all_connections_doing_okay() const;
    
private: // the connection
    vrpn_BaseConnection * d_connection_ptr;

public: // clock synch functions

    void setClockOffset( void *userdata, const vrpn_CLOCKCB& info );

    // do all inits that are done in
    // vrpn_Clock_Remote constructor
    void init_clock_client();

    // mostly code from vrpn_Clock_Remote::mainloop()
    void synchronize_clocks();

    // MANIPULATORS

    // (un)Register a callback to handle a clock sync
    virtual vrpn_int32 register_clock_sync_handler(void *userdata,
                        vrpn_CLOCKSYNCHANDLER handler);
    virtual vrpn_int32 unregister_clock_sync_handler(void *userdata,
                          vrpn_CLOCKSYNCHANDLER handler);
    // request a high accuracy sync (and turn off quick syncs)
    struct timeval fullSync ();


    // ACCESSORS


    // Returns the most recent RTT estimate.  TCH April 99
    // Returns 0 if the first RTT estimate is not yet completed
    // or if no quick syncs are being done.
    struct timeval currentRTT () const;

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
    typedef struct _vrpn_CLOCKSYNCLIST {
      void          *userdata;
      vrpn_CLOCKSYNCHANDLER handler;
      struct _vrpn_CLOCKSYNCLIST    *next;
    } vrpn_CLOCKSYNCLIST;

    vrpn_CLOCKSYNCLIST  *change_list;

    static vrpn_int32 quickSyncClockServerReplyHandler(void *userdata, 
                                                       vrpn_HANDLERPARAM p);
    static vrpn_int32 fullSyncClockServerReplyHandler(void *userdata, 



};

#endif
