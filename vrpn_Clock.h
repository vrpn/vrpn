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
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_Clock.h,v $
  $Locker:  $
  $Revision: 1.9 $
\*****************************************************************************/
#ifndef _VRPN_CLOCK_H_
#define _VRPN_CLOCK_H_

#include "vrpn_Connection.h"

class vrpn_Connection;

#define CLOCK_VERSION 0x10

// Base class for clock servers.  Definition of remote/client 
// clock class for the user is at the end.
class vrpn_Clock {
public:
  vrpn_Clock(const char *name, vrpn_Connection *c = NULL);
  
  virtual void mainloop(void) = 0;	// Report changes to connection
  
protected:
  vrpn_Connection *connection;
  vrpn_int32 clockServer_id;	// ID of this clock to connection
  vrpn_int32 queryMsg_id;	// ID of clockQuery message to connection
  vrpn_int32 replyMsg_id;	// ID of clockReply message to connection
  virtual int encode_to(char *buf, const struct timeval& tvSRep, 
			const struct timeval& tvCReq, 
			vrpn_int32 cChars, const char* pch);
};


class vrpn_Clock_Server : public vrpn_Clock {
public:
  vrpn_Clock_Server(vrpn_Connection *c);

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

  vrpn_Clock_Remote(const char *name, vrpn_float64 dFreq=1,
		vrpn_int32 cOffsetWindow=3);
  virtual ~vrpn_Clock_Remote();

  // This routine calls does the sync and calls the mainloop of the 
  // connection it's on
  virtual void mainloop(void);

  // (un)Register a callback to handle a clock sync
  virtual int register_clock_sync_handler(void *userdata,
					  vrpn_CLOCKSYNCHANDLER handler);
  virtual int unregister_clock_sync_handler(void *userdata,
					    vrpn_CLOCKSYNCHANDLER handler);
  // request a high accuracy sync (and turn off quick syncs)
  void fullSync();

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



/*****************************************************************************\
  $Log: vrpn_Clock.h,v $
  Revision 1.9  1999/02/24 15:58:27  taylorr
  BIG CHANGES.
  I modified the code so that it can compile on 64-bit SGI machines.

  To do so, I did what I should have done in the first place and defined
  architecture-independent types (vrpn_in32, vrpn_int16, vrpn_float32,
  vrpn_float64 and so on).  These are defined per architecture in the
  vrpn_Shared.h file.

  FROM NOW ON, folks should use these rather than the non-specific types
  (int, long, float, double) that may vary between platforms.

  Revision 1.8  1998/06/26 15:48:54  hudson
  Wrote vrpn_FileConnection.
  Changed connection naming convention.
  Changed all the base classes to reflect the new naming convention.
  Added #ifdef sgi around vrpn_sgibox.

  Revision 1.7  1998/03/23 17:06:45  weberh
  clock changes:

  fixed up clock constructor to allow proper independent clock server/client
  creation.

  added code to do fullsync regression for offset slope estimation (added,
  but not currently used because it is not beneficial enough).

  fixed up a few comments and a minor bug which affected quick sync accuracy.


  connection changes:

  added -2 as a sync connection freq arg which performs an immediate
  fullsync.

  Revision 1.6  1998/03/19 06:04:38  weberh
  added a fullSync call to vrpn_Synchronized connection for convenience.

  Revision 1.5  1998/03/18 20:24:31  weberh
  vrpn_Clock -- added better unique id for clock sync messages

  vrpn_Connection -- fixed error which caused calls to get_connection_by_name
                     to create multiple connections.

  Revision 1.4  1998/02/20 20:26:50  hudson
  Version 02.10:
    Makefile:  split library into server & client versions
    Connection:  added sender "VRPN Control", "VRPN Connection Got/Dropped"
      messages, vrpn_ANY_TYPE;  set vrpn_MAX_TYPES to 2000.  Bugfix for sender
      and type names.
    Tracker:  added Ruigang Yang's vrpn_Tracker_Canned

  Revision 1.2  1998/02/13 15:50:58  hudson
  *** empty log message ***

  Revision 1.1  1998/01/29 20:09:35  hudson
  Initial revision

  Revision 1.3  1998/01/20 15:53:27  taylorr
  	This version allows the client-side compilation of VRPN on NT.

  Revision 1.2  1998/01/08 23:32:49  weberh
  Summary of changes
  1) vrpn_Tracker_Ceiling is now in the cvs repository instead of just
     the tracker hierarchy
  2) vrpn uses doubles to transmit tracker data instead of floats
  3) vrpn has a vrpn_ALIGN macro and uses 8 byte alignment
  4) vrpn_Synchronized_Connection class was derived from regular connection
     and transforms time stamps to the local time frame (see html man pages
     for more info)
  5) vrpn_Shared was modified to support time stamp math ops and gettimeofday
     under win nt/95

  Revision 2.0  1997/12/21 05:13:40  weberh
  WORKING!

  Revision 1.2  1997/12/20 00:02:12  weberh
  cleaned up.

  Revision 1.1  1997/12/16 19:39:34  weberh
  Initial revision

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

