/*****************************************************************************\
  vrpn_Clock.h
  --
  Description : Note: for both types of clocks (server and client), users
                      need only to pass in the station which runs the 
		      clock server (a name from the sdi_devices file).
		      The vrpn_Clock class takes care of the actual clock
		      device name (since there is only one clock device).
		      (i.e., you just say vrpn_Clock("server name") or
		      vrpn_Clock_Remote("server name"), where "server name"
		      is an entry in the .sdi_devices file.)

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Sat Dec 13 11:21:15 1997
  Revised: Mon Mar 23 11:34:26 1998 by weberh
\*****************************************************************************/
#ifndef _VRPN_CLOCK_H_
#define _VRPN_CLOCK_H_

#include "vrpn_Connection.h"
#include "vrpn_BaseClass.h"

class vrpn_Connection;

#define CLOCK_VERSION 0x10

// Base class for clock servers.  Definition of remote/client 
// clock class for the user is at the end.
class vrpn_Clock : public vrpn_BaseClass {
public:
  vrpn_Clock(const char *name, vrpn_Connection *c = NULL);
  
protected:
  vrpn_int32 queryMsg_id;	// ID of clockQuery message to connection
  vrpn_int32 replyMsg_id;	// ID of clockReply message to connection

  virtual int register_senders(void);
  virtual int register_types(void);
  virtual int encode_to(char *buf, const struct timeval& tvSRep, 
			const struct timeval& tvCReq, 
			vrpn_int32 cChars, const char* pch);
};


class vrpn_Clock_Server : public vrpn_Clock {
public:
  vrpn_Clock_Server(vrpn_Connection *c);
  virtual ~vrpn_Clock_Server();

  // Called once through each main loop iteration to handle
  // clock updates.
  // Report changes to connection
  virtual void mainloop ();
  static int clockQueryHandler( void *userdata, vrpn_HANDLERPARAM p );
};


//----------------------------------------------------------
// ************* Users deal with the following *************

// User routine to handle messages regarding the clock offset between 
// the server host and the client host.
// Right now this is called a short while after the mainloop is
// first called.  In a future version this might regularly re-sync the
// host and client whenever a new clock_sync_handler is registered.
// It is best to call this before the server is sending all kinds of
// other messages.
// The clock sync procedure takes 1 second to run.

typedef	struct {
	struct timeval	msg_time;	// Local time of this sync message
	struct timeval  tvClockOffset;  // Local time - remote time in msecs
	struct timeval  tvHalfRoundTrip;  // half of the roundtrip time
                                          // for the roundtrip used to calc offset
} vrpn_CLOCKCB;

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

class vrpn_Clock_Remote: public vrpn_Clock {


  public:


    // The name of the station which runs the clock server to connect to
    // (from sdi_devices), and the frequency at which to do quick re-syncs 
    // (1 hz by default).  If the user specifies a freq < 0, then there will
    // be no quick syncs -- they will have to request syncs with fullSync().
    // The final arg is the number of reports over which the user wants to
    // maintain a window from which the offset routine will choose the min
    // round trip report and use the offset from that trip.
    // A high setting (e.g., 40) works well for arrangements with little drift
    // while a low setting (e.g., 3) works well when drift is present.
    // See cMaxQuickRecords below for more detail.

    vrpn_Clock_Remote (const char * name, vrpn_float64 dFreq = 1,
		       vrpn_int32 cOffsetWindow = 3);
    virtual ~vrpn_Clock_Remote (void);


    // MANIPULATORS


    // This routine calls does the sync and calls the mainloop of the 
    // connection it's on.  The clock does not call client_mainloop()
    // or server_mainloop, because it does not need heartbeats or
    // initialization calls.
    virtual void mainloop (const struct timeval *timeout);
    virtual void mainloop (void) { mainloop(NULL); };

    // (un)Register a callback to handle a clock sync
    virtual int register_clock_sync_handler(void *userdata,
					    vrpn_CLOCKSYNCHANDLER handler);
    virtual int unregister_clock_sync_handler(void *userdata,
					      vrpn_CLOCKSYNCHANDLER handler);
    // request a high accuracy sync (and turn off quick syncs)
    void fullSync (void);


    // ACCESSORS


    // Returns the most recent RTT estimate.  TCH April 99
    // Returns 0 if the first RTT estimate is not yet completed
    // or if no quick syncs are being done.
    struct timeval currentRTT (void) const;


  protected:


    vrpn_int32 clockClient_id;             // vrpn id for this client

    // unique id for this particular client (to disambiguate replies)
    vrpn_int32 lUniqueID;

    // vars for stats on clock for quick sync
    int fDoQuickSyncs;
    int cQuickBounces;

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
    int cMaxQuickRecords;
    struct timeval *rgtvHalfRoundTrip;
    struct timeval *rgtvClockOffset;
    int irgtvQuick;

    vrpn_float64 dQuickIntervalMsecs;
    struct timeval tvQuickLastSync;

    // vars for stats on clock for full sync
    int fDoFullSync;
    int cBounces;

#ifdef USE_REGRESSION
    vrpn_float64 *rgdOffsets;
    vrpn_float64 *rgdTimes;
#endif

    struct timeval tvMinHalfRoundTrip;

    // offset to report to user
    struct timeval tvFullClockOffset;

    // vars for user specified handlers
    typedef struct _vrpn_CLOCKSYNCLIST {
      void			*userdata;
      vrpn_CLOCKSYNCHANDLER	handler;
      struct _vrpn_CLOCKSYNCLIST	*next;
    } vrpn_CLOCKSYNCLIST;

    vrpn_CLOCKSYNCLIST	*change_list;

    static int quickSyncClockServerReplyHandler(void *userdata, 
					        vrpn_HANDLERPARAM p);
    static int fullSyncClockServerReplyHandler(void *userdata, 
					       vrpn_HANDLERPARAM p);
};

#endif // ifndef _VRPN_CLOCK_H_
