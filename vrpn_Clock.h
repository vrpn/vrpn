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
  Revised: Mon Dec 15 15:06:54 1997 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_Clock.h,v $
  $Locker:  $
  $Revision: 1.1 $
\*****************************************************************************/
#ifndef _VRPN_CLOCK_H_
#define _VRPN_CLOCK_H_

#include "vrpn_Connection.h"

// Base class for clock servers.  Definition of remote/client 
// clock class for the user is at the end.

class vrpn_Clock {
public:
  vrpn_Clock(char *name, vrpn_Connection *c = NULL);
  
  virtual void mainloop(void) = 0;	// Report changes to connection
  
protected:
  vrpn_Connection *connection;
  long clockServer_id;		// ID of this clock to connection
  long queryMsg_id;		// ID of clockQuery message to connection
  long replyMsg_id;		// ID of clockReply message to connection
  virtual int encode_to(char *buf, const struct timeval& tv, 
			int cChars, const char* pch);
};

class vrpn_Clock_Server : public vrpn_Clock {
public:
  vrpn_Clock_Server(char *name, vrpn_Connection *c);

  // Called once through each main loop iteration to handle
  // clock updates.
  virtual void mainloop(void);	// Report changes to connection
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
// desired frequency (in hz) of clock sync updates.  If the freq is
// negative, then you will get no automatic syncs. At any time you
// can get a very accurate sync by calling fullSync();

#ifndef _WIN32

// special message identifiers for remote clock syncs
#define VRPN_CLOCK_FULL_SYNC 1
#define VRPN_CLOCK_QUICK_SYNC 2

class vrpn_Clock_Remote: public vrpn_Clock {
  public:
  // The name of the station which runs the clock server to connect to
  // (from sdi_devices), and the frequency at which to do quick re-syncs 
  // (1 hz by default).  If the user specifies a freq < 0, then there will
  // be no quick syncs -- they will have to request syncs with fullSync().
  
  vrpn_Clock_Remote(char *name, double dFreq=1);

  // This routine calls does the sync and calls the mainloop of the 
  // connection it's on
  virtual void mainloop(void);

  // (un)Register a callback to handle a clock sync
  virtual int register_clock_sync_handler(void *userdata,
					  vrpn_CLOCKSYNCHANDLER handler);
  virtual int unregister_clock_sync_handler(void *userdata,
					    vrpn_CLOCKSYNCHANDLER handler);
  // request a high accuracy re-sync
  void fullSync();

  protected:
  long clockClient_id;             // vrpn id for this client

  // vars for stats on clock for quick sync
  int fDoQuickSyncs;
  int cQuickBounces;

  // CC does not like this, so ...
  // const int cMaxQuickDiffs=5
#define cMaxQuickRecords 5
  struct timeval rgtvHalfRoundTrip[cMaxQuickRecords];
  struct timeval rgtvClockOffset[cMaxQuickRecords];
  int irgtvQuick;

  double dQuickIntervalMsecs;
  struct timeval tvQuickLastSync;

  // vars for stats on clock for full sync
  int fDoFullSync;
  int cBounces;

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
#endif // ifndef _WIN32

#endif // ifndef _VRPN_CLOCK_H_



/*****************************************************************************\
  $Log: vrpn_Clock.h,v $
  Revision 1.1  1997/12/15 21:25:09  weberh
  Added the vrpn_Clock class to vrpn to allow users (and eventually a
  connection) to find out the offset between the server and client clock so
  that users can know, in local time, when events happened on remote servers.

  There are two basic modes -- fullSync (on request only) and quickSync
  (default).  See the .h file for more info.

  Other changes included updating vrpn_Shared.h with time val diff and sum
  operators and changing the pc version of gettimeofday so that it gets
  0.8 usec resolution rather than 6 ms resolution.

  I also changed a few things so that the entire package will compile under
  g++ or CC on sgi, hp, or win 95/nt.

\*****************************************************************************/

