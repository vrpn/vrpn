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
    vrpn_ClientConnectionController();

    // helper function
    // XXX if this doesn't go away, give a more descriptive comment
    virtual int connect_to_server( const char *machine, int port );

public: // mainloop

    virtual vrpn_int32 mainloop( const timeval * timeout = NULL );

private: // the connection
    //list <vrpn_BaseConnection *> connection_list;

public: // clock synch functions

    void setClockOffset( void *userdata, const vrpn_CLOCKCB& info );

    // do all inits that are done in
    // vrpn_Clock_Remote constructor
    void init_clock_client(void);

    // mostly code from vrpn_Clock_Remote::mainloop()
    void synchronize_clocks(void);

    // MANIPULATORS

    // (un)Register a callback to handle a clock sync
    virtual int register_clock_sync_handler(void *userdata,
                        vrpn_CLOCKSYNCHANDLER handler);
    virtual int unregister_clock_sync_handler(void *userdata,
                          vrpn_CLOCKSYNCHANDLER handler);
    // request a high accuracy sync (and turn off quick syncs)
    struct timeval fullSync (void);


    // ACCESSORS


    // Returns the most recent RTT estimate.  TCH April 99
    // Returns 0 if the first RTT estimate is not yet completed
    // or if no quick syncs are being done.
    struct timeval currentRTT (void) const;

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
