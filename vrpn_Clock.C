/*****************************************************************************\
  vrpn_Clock.C
  --
  Description :
  
  ----------------------------------------------------------------------------
  Author: weberh
  Created: Sat Dec 13 11:05:16 1997
  Revised: Wed Apr  1 13:23:40 1998 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_Clock.C,v $
  $Locker:  $
  $Revision: 1.25 $
  \*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"
#ifndef _WIN32
#include <netinet/in.h>
#endif

#include <iostream.h>
#include <math.h>

#include "vrpn_Clock.h"

void printTime( char *pch, const struct timeval& tv ) {
  cerr << pch << " " << tv.tv_sec*1000.0 + tv.tv_usec/1000.0 << " msecs." << endl;
}

// both server and client call this constructor
vrpn_Clock::vrpn_Clock(const char * name, vrpn_Connection *c) {
  // If the connection is valid, use it to register this clock by
  // name, the query by name, and the reply by name
  char * servicename;
  servicename = vrpn_copy_service_name(name);
  connection = c;
  if (connection == NULL) {
    return;
  }
  clockServer_id = connection->register_sender(servicename);

  queryMsg_id = connection->register_message_type("clock query");
  replyMsg_id = connection->register_message_type("clock reply");
  if ( (clockServer_id == -1) || (queryMsg_id == -1) || (replyMsg_id == -1) ) {
    cerr << "vrpn_Clock: Can't register IDs" << endl;
    connection = NULL;
    return;
  }

  // need to init gettimeofday for the pc
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (servicename)
    delete [] servicename;
}

// packs time of message sent and all data sent to it back into buffer
int vrpn_Clock::encode_to(char *buf, const struct timeval& tvSRep,
			  const struct timeval& tvCReq, 
			  vrpn_int32 cChars, const char *pch ) {

  // pack client time and the unique client identifier
  vrpn_int32 *rgl = (vrpn_int32 *)buf;
  rgl[0]=htonl(CLOCK_VERSION);
  rgl[1]=htonl(tvSRep.tv_sec);
  rgl[2]=(vrpn_int32)htonl(tvSRep.tv_usec);
  rgl[3]=htonl(tvCReq.tv_sec);
  rgl[4]=(vrpn_int32)htonl(tvCReq.tv_usec);

  // skip over version number
  pch += sizeof(vrpn_int32);
  cChars -= sizeof(vrpn_int32);
  memcpy( (char *) &rgl[5], pch, cChars );
  
  return 5*sizeof(vrpn_int32)+cChars;
}


vrpn_Clock_Server::vrpn_Clock_Server(vrpn_Connection *c) 
  : vrpn_Clock("clockServer", c) {

  // Register the callback handler (along with "this" as user data)
  // It will take messages from any sender (no sender arg specified)
  if (connection->register_handler(queryMsg_id, clockQueryHandler, this)) {
    cerr << "vrpn_Clock: can't register handler\n" << endl;
    connection = NULL;
    return;
  }
}

vrpn_Clock_Server::~vrpn_Clock_Server() 
{
  // Unregister the change handler
  if (connection != NULL) {
	if (connection->unregister_handler(queryMsg_id, 
	     clockQueryHandler, this)) {
	  fprintf(stderr,
		"vrpn_Clock_Server: Can't unregister change handler\n");
	  fprintf(stderr,"   (VRPN internal error: expect seg fault)\n");
	}
  }
}

// to prevent the user from getting back incorrect info, clients are
// (nearly) uniquely identified by a usec timestamp.
int vrpn_Clock_Server::clockQueryHandler(void *userdata, vrpn_HANDLERPARAM p) {
  vrpn_Clock_Server *me = (vrpn_Clock_Server *) userdata;
  static struct timeval now;
  char rgch[50];
  
  // send back time client sent request and the buffer the client sent
  //  cerr << ".";
  //  printTime("client req ",p.msg_time);

  // check clock version
  if ((p.payload_len==0) || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
    cerr << "vrpn_Clock_Server: current version is 0x" << hex << CLOCK_VERSION 
	 << ", clock query msg uses version 0x" << ntohl(*(vrpn_int32 *)p.buffer) 
	 << "." << dec << endl;
    return -1;
  }

  gettimeofday(&now,NULL);
  //  printTime("server rep ", now);
  int cLen = me->encode_to( rgch, now, p.msg_time, p.payload_len, p.buffer );
  me->connection->pack_message(cLen, now,
			       me->replyMsg_id, me->clockServer_id,
			       rgch, vrpn_CONNECTION_RELIABLE);
  return 0;
}

// the clock server only responds to requests. since the connection
// mainloop will handle routing the callback (and the general server
// program always has to call that), this mainloop does nothing.
void vrpn_Clock_Server::mainloop (const struct timeval *) {};


// This next part is the class for users (client class)


// If this clock is created as part of a sync_conn constructor,
// then the connection will already exist, and the -1 will have no
// effect (the vrpn_get_connection by name will just return a
// ptr to the connection).

// If the use is actually creating a vrpn_Clock_Remote directly
// (as a clock client), then they don't want another clock on the
// connection as this will result in less accurate calibration.
// -1 as the freq arg, because if the connection does not yet
// exist, then when it is created it does not need to have another
// clock running on it (the sync_connection will still have a clock
// created on it, but the clock will be a fullSync clock and
// will be inactive.)

vrpn_Clock_Remote::vrpn_Clock_Remote(const char * name, vrpn_float64 dFreq, 
				     vrpn_int32 cOffsetWindow ) : 
  vrpn_Clock ("clockServer",
              vrpn_get_connection_by_name (name, NULL, 0L, NULL, 0L, -1)), 
  change_list(NULL)
{
  char rgch [50];
  int i;
  
  if (connection==NULL) {
    cerr << "vrpn_Clock_Remote: unable to connect to clock server \"" 
	 << name << "\"." << endl;
    return;
  }
  sprintf( rgch, "%ld", (long) this );
  clockClient_id = connection->register_sender(rgch);
  
  if (clockClient_id == -1) {
    cerr << "vrpn_Clock_Remote: Can't register ID" << endl;
    connection = NULL;
    return;
  }

  fDoFullSync = 0;
  if (dFreq <= 0) {
    // only sync on request of user, and then do it with fullSync()
    fDoQuickSyncs=0;
    rgtvHalfRoundTrip = NULL;
    rgtvClockOffset = NULL;
    
    // handler is automatically registered by full sync part of mainloop
  } else {
    fDoQuickSyncs=1;

    // init quick bounce variables
    cQuickBounces=0;
    dQuickIntervalMsecs = (1/dFreq)*1000.0;

    // do sync asap
    tvQuickLastSync.tv_sec=0;
    tvQuickLastSync.tv_usec=0;

    // set up quick arrays
    irgtvQuick=0;
    cMaxQuickRecords = cOffsetWindow;
    rgtvHalfRoundTrip = new struct timeval [cMaxQuickRecords];
    rgtvClockOffset = new struct timeval [cMaxQuickRecords];

    // Initialize rgtv to 0 so that currentRTT doesn't return garbage.
    // TCH April 99
    for (i = 0; i < cMaxQuickRecords; i++) {
      rgtvHalfRoundTrip[i].tv_sec = 0L;
      rgtvHalfRoundTrip[i].tv_usec = 0L;
    }
    
    // register a handler for replies from the clock server.
    if (connection->register_handler(replyMsg_id, 
				     quickSyncClockServerReplyHandler,
				     this, clockServer_id)) {
      cerr << "vrpn_Clock_Remote: Can't register handler" << endl;
      connection = NULL;
    }
  }

  // establish unique id
  struct timeval tv;
  gettimeofday(&tv, NULL);

  lUniqueID = (vrpn_int32)tv.tv_usec;

#ifdef USE_REGRESSION
  rgdOffsets = NULL;
  rgdTimes = NULL;
#endif
}

vrpn_Clock_Remote::~vrpn_Clock_Remote (void) {

  // Unregister the change handler if we are not doing full synchs
  if (fDoQuickSyncs != 0) {
    if (connection != NULL) {
	if (connection->unregister_handler(replyMsg_id, 
	     quickSyncClockServerReplyHandler,
	     this, clockServer_id)) {
	  fprintf(stderr,
		"vrpn_Clock_Remote: Can't unregister change handler\n");
	  fprintf(stderr,"   (VRPN internal error: expect seg fault)\n");
	}
    }
  }


  // release the quick arrays
  if (rgtvHalfRoundTrip)
    delete [] rgtvHalfRoundTrip;
  if (rgtvClockOffset)
    delete [] rgtvClockOffset;

#ifdef USE_REGRESSION

  // release the regression arrays
  if (rgdOffsets)
    delete rgdOffsets;
  if (rgdTimes)
    delete rgdTimes;

#endif  // USE_REGRESSION

}

#if 0
// for qsort for median
static vrpn_int32 dCompare( const void *pd1, const void *pd2 ) {
  if (*(vrpn_float64 *)pd1==*(vrpn_float64 *)pd2) return 0;
  if (*(vrpn_float64 *)pd1<*(vrpn_float64 *)pd2) return -1;
  return 1;
}
#endif


// this will take 1 second to run (sync's the clocks)
// when it is in fullSync mode
void vrpn_Clock_Remote::mainloop (const struct timeval * timeout)
{
  if (connection) { 
    // always do this -- the first time it will register senders & msg_types
    // Also, we don't want to call the Synchronized version (if this is
    // a sync connection) since we are already in that loop (this is 
    // the routine that does the synchronization)
    connection->vrpn_Connection::mainloop(timeout);
    
    if (fDoQuickSyncs) {
      // just a quick sync
      //      cerr << "QuickSync" << endl;      
      struct timeval tvNow;
      gettimeofday(&tvNow, NULL);

      // Check if we have passed the interval
      if (vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvQuickLastSync)) >=
	  dQuickIntervalMsecs) {
	vrpn_int32 rgl[3];
	struct timeval tv;

	// yes, we have, so pack a message and reset clock

	// send a clock query with this clock client's unique id
	rgl[0]=htonl(CLOCK_VERSION);
	rgl[1]=htonl(lUniqueID);	// An easy, unique ID
	rgl[2]=htonl((vrpn_int32)VRPN_CLOCK_QUICK_SYNC);
	gettimeofday(&tv, NULL);
	connection->pack_message(3*sizeof(vrpn_int32), tv, queryMsg_id, 
				 clockClient_id, (char *)rgl, 
				 vrpn_CONNECTION_RELIABLE);
	tvQuickLastSync = tvNow;
	    
	// send out the clock sync messages right away
	connection->vrpn_Connection::mainloop();
      }
    }

    // any time a full clock sync has been requested, do it for 1 sec
    if (fDoFullSync) {
      fDoFullSync=0;
      // cerr << "FullSync" << endl;      
      // register a handler for replies from the clock server.
      if (connection->register_handler(replyMsg_id, 
				       fullSyncClockServerReplyHandler,
				       this, clockServer_id)) {
	cerr << "vrpn_Clock_Remote: Can't register handler" << endl;
	connection = NULL;
      }

      // init the necessary variables for doing clock sync
      cBounces=0;
      tvMinHalfRoundTrip.tv_sec=1000000;
      tvMinHalfRoundTrip.tv_usec=0;
      tvFullClockOffset.tv_sec=0;
      tvFullClockOffset.tv_usec=0;
      
      // now calibrate the time diff for 1 second
      struct timeval tvCalib;
      struct timeval tvNow;
      vrpn_int32 cNextTime=0;
      const vrpn_int32 cInterval=20;
      const vrpn_int32 cCalibMsecs=1000;
      vrpn_int32 cElapsedMsecs=0;
      const vrpn_int32 cQueries = (cCalibMsecs/cInterval) + 1;
      vrpn_int32 iQueries = 0;

      cerr << "vrpn_Clock_Remote: performing fullsync calibration, "
	"this will take " << cCalibMsecs/1000.0 << " seconds." << endl;

#ifdef USE_REGRESSION
      // save offsets and time of offset calc for regression
      if (!rgdOffsets) {
	rgdOffsets = new vrpn_float64[cQueries];
      }
      if (!rgdTimes) {
	rgdTimes = new vrpn_float64[cQueries];
      }
#endif
      gettimeofday( &tvCalib, NULL );

      // do bounces for one second to calibrate the clocks
      //      while (cElapsedMsecs<=cCalibMsecs) {
      while ( iQueries < cQueries ) {

	// don't let replies overwrite the buffer
	if (cBounces>=cQueries) {
	  cerr << "vrpn_Clock_Remote::mainloop: multiple clock servers on "
	    "connection -- aborting fullSync " <<cBounces<< endl;
	  if (connection->unregister_handler(replyMsg_id, 
					     fullSyncClockServerReplyHandler,
					     this, clockServer_id)) {
	    cerr << "vrpn_Clock_Remote: Can't unregister handler" << endl;
	    connection = NULL;
	  }
	  break;
	}

	// do one every cInterval ms or so
	if (cElapsedMsecs>=cNextTime) {
	  struct timeval tv;
	  vrpn_int32 rgl[3];
	  
	  iQueries++;
	  // send a clock query with this clock client's unique id
	  // not converted using htonl because it is never interpreted
	  // by the server -- only by the client.
	  rgl[0]=htonl(CLOCK_VERSION);
	  rgl[1]=htonl(lUniqueID);	// An easy, unique ID
	  rgl[2]=htonl((vrpn_int32)VRPN_CLOCK_FULL_SYNC);
	  gettimeofday(&tv, NULL);
	  connection->pack_message(3*sizeof(vrpn_int32), tv, queryMsg_id, 
				   clockClient_id, (char *)rgl, 
				   vrpn_CONNECTION_RELIABLE);
	  cNextTime+=cInterval;
	}
	
	// actually process the message and any callbacks from local
	// or remote messages (just call the base mainloop)
	connection->vrpn_Connection::mainloop();
	gettimeofday( &tvNow, NULL );
	cElapsedMsecs = (vrpn_int32) vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvCalib));
      }
      // we don't care if we missed numerous replies, just make sure we had
      // at least half back already
      
      if (cBounces<((cCalibMsecs/cInterval)/2)) {
	cerr << "vrpn_Clock_Remote::mainloop: calib error; did not receive "
	  "replies for at least half of the bounces" << endl;
	return;
      } 

      // actually, we just want to use the min roundtrip record to calc the
      // offset so we can do it interactively, and the clock offset calced
      // from that record is in tvFullClockOffset
      
      // now get rid of handler -- no need to calibrate any longer
      // so don't waste time checking the ids of other's clock server replies.
      
      if (connection->unregister_handler(replyMsg_id, 
					 fullSyncClockServerReplyHandler,
					 this, clockServer_id)) {
	cerr << "vrpn_Clock_Remote: Can't unregister handler" << endl;
	connection = NULL;
      }

#ifdef USE_REGRESSION
      // NOTE: save this chunk of code.
      // At some point, we may want to use this code.
      // What this does is try to estimate drift (in usec/sec) during
      // the full sync by performing linear regression on the clock offsets.
      // This could be used to drift correct the time stamps.
      // I (Hans Weber) worked on this for a while and found that 
      // although the stddev of the drift rates (slope) in usec/sec
      // were fairly low (usually i could get about 2-5 usec/sec
      // stddev for a 10 second sync), they did not represent the 
      // distribution very well (ie, i don't feel like the distribution
      // is very normal).  Even if they do represent it accurately,
      // 5 usec/sec means 1/2 ms error in 100 secs, and 5 ms error
      // after running for 1000 secs (quick syncs can get us down to
      // 0.5*mainloop_service_period error regardless of how long we run, so 
      // for 60 hz, about 8 ms).
      // Neither method is great, but for now quick sync's are easier
      // and don't bias the offset over time, so i will stick with them.
      // Also, the calculations below have been checked with matlab,
      // so I am fairly confident they are correct.

      // now do the regression calculation 
      vrpn_float64 dIntercept;
      vrpn_float64 dSlope;
      // x is time, y is offset
      vrpn_float64 dSXY=0, dSX=0, dSY=0, dSXX=0, dSXSX=0;
      int n = cBounces;

      // ignore the first ten bounces
      int cIgnore = 10;

      vrpn_float64 dN = n - cIgnore;

      
      // do a scale on the times to get better 
      // use of floating point range and to find
      // slope in uSec per sec.
      vrpn_float64 dInitTime = rgdTimes[0];

      for (int irgd=cIgnore;irgd<n;irgd++) {
	rgdTimes[irgd]-=dInitTime;
	rgdTimes[irgd]/=1000.0;
	rgdOffsets[irgd]*=1000.0;
	//	cerr << irgd << "\ttime " << rgdTimes[irgd] 
	//	     << "\toffset " << rgdOffsets[irgd] 
	//	     << endl;

	dSXY += rgdTimes[irgd]*rgdOffsets[irgd];
	dSX += rgdTimes[irgd];
	dSY += rgdOffsets[irgd];
	dSXX += rgdTimes[irgd]*rgdTimes[irgd];
      }
      dSXSX = dSX*dSX;

      cerr << "dSXY= " << dSXY << endl; 
      cerr << "dSX= " << dSX << endl; 
      cerr << "dSY= " << dSY << endl; 
      cerr << "dSXX= " << dSXX << endl; 
      cerr << "dSXSX= " << dSXSX << endl; 

      dSlope = ( dSXY - (1/dN)*dSX*dSY ) /
	(dSXX - (1/dN)*dSXSX);

      dIntercept = (dSY/dN) - dSlope * (dSX/dN);

      cerr << "regression coeffs are (int (usec), slope (usec/sec)) " <<
	dIntercept << ", " << dSlope << endl;
      
      vrpn_float64 dSEE = 0;
      vrpn_float64 dSXMXM = 0;

      for (int i=cIgnore;i<n;i++) {
	dSEE += (rgdOffsets[i] - (dSlope*rgdTimes[i] + dIntercept))*
	  (rgdOffsets[i] - (dSlope*rgdTimes[i] + dIntercept));
	dSXMXM += (rgdTimes[i] - dSX/dN)*(rgdTimes[i] - dSX/dN);
      }

      vrpn_float64 dS = sqrt(dSEE/(dN-2));
      vrpn_float64 dSSlope = dS / sqrt(dSXMXM);
      vrpn_float64 dSInt = dS * sqrt((1/dN) + (dSX/dN)*(dSX/dN) / dSXMXM);

      cerr << "stddev of slope is " << dSSlope << endl;
      cerr << "stddev of intercept is " << dSInt << endl;
#endif

      // now call any user callbacks that want to know the time diff
      // when syncs occur
      vrpn_CLOCKCB cs;
      vrpn_CLOCKSYNCLIST *pHandlerInfo=change_list;
      
      cs.tvClockOffset = tvFullClockOffset;
      cs.tvHalfRoundTrip = tvMinHalfRoundTrip;
      
#if 0
     cerr << "vrpn_Clock_Remote::mainloop: clock offset is " 
	  << tvFullClockOffset.tv_sec << "s, "
	  << tvFullClockOffset.tv_usec << "us." << endl;
     cerr << "vrpn_Clock_Remote::mainloop: half round trip is " 
	  << tvMinHalfRoundTrip.tv_sec << "s, "
	  << tvMinHalfRoundTrip.tv_usec << "us." << endl;
#endif

      // go thru list of user specified clock sync callbacks
      while (pHandlerInfo!=NULL) {
	pHandlerInfo->handler(pHandlerInfo->userdata, cs);
	pHandlerInfo = pHandlerInfo->next;
      }
    }
  }
}

int vrpn_Clock_Remote::register_clock_sync_handler(void *userdata,
						   vrpn_CLOCKSYNCHANDLER handler)
{
  vrpn_CLOCKSYNCLIST	*new_entry;
  
  // Ensure that the handler is non-NULL
  if (handler == NULL) {
    cerr << "vrpn_Clock_Remote::register_clock_sync_handler:" 
	 << " NULL handler" << endl;
    return -1;
  }
  
  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_CLOCKSYNCLIST) == NULL) {
    cerr << "vrpn_Clock_Remote::register_clock_sync_handler:" 
	 << " out of memory" << endl;
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;
  
  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
  new_entry->next = change_list;
  change_list = new_entry;
  
  return 0;
}

int vrpn_Clock_Remote::unregister_clock_sync_handler(void *userdata,
						     vrpn_CLOCKSYNCHANDLER handler)
{
  // The pointer at *snitch points to victim
  vrpn_CLOCKSYNCLIST	*victim, **snitch;
  
  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  snitch = &change_list;
  victim = *snitch;
  while ( (victim != NULL) &&
	  ( (victim->handler != handler) ||
	    (victim->userdata != userdata) )) {
    snitch = &( (*snitch)->next );
    victim = victim->next;
  }
  
  // Make sure we found one
  if (victim == NULL) {
    fprintf(stderr, "vrpn_Clock_Remote::unregister_handler: No such handler\n");
    return -1;
  }
  
  // Remove the entry from the list
  *snitch = victim->next;
  delete victim;
  
  return 0;
}

void vrpn_Clock_Remote::fullSync (void) {
  fDoFullSync=1;
  fDoQuickSyncs=0;
}

// Returns the most recent RTT estimate.  TCH April 99
// Returns 0 if the first RTT estimate is not yet completed
// or if no quick syncs are being done.
// RTT is 2 * most recent estimate of half-RTT.

struct timeval vrpn_Clock_Remote::currentRTT (void) const {
  struct timeval retval;
  int last;

  // Assumes values are initialized to 0 or otherwise safe.
  // (Ensured by a clause in the constructor).

  retval.tv_sec = 0L;
  retval.tv_usec = 0L;

  if (!rgtvHalfRoundTrip)
    return retval;

  last = this->irgtvQuick - 1;
  while (last < 0)
    last += cMaxQuickRecords;
  retval = vrpn_TimevalScale(this->rgtvHalfRoundTrip[last], 2.0);
  //fprintf(stderr, "Record %d is %ld:%ld.\n", last, retval.tv_sec, retval.tv_usec);

  return retval;
}


// we return to the user callback the clock offset calculated from the
// shortest roundtrip over all of the full sync replies.
// We use the min because it should give us the most accurate calculation of
// the clock offset.

int vrpn_Clock_Remote::fullSyncClockServerReplyHandler(void *userdata, 
						       vrpn_HANDLERPARAM p) {
  // get current local time
  struct timeval tvLNow;
  
  gettimeofday(&tvLNow, NULL);
  
  // all of the tv structs have an L (local) or R (remote) tag
  vrpn_Clock_Remote *me = (vrpn_Clock_Remote *)userdata;

  // look at timing info sent along
  vrpn_int32 *plTimeData = (vrpn_int32 *) p.buffer;

  // check clock version
  if ((p.payload_len==0) || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
    cerr << "vrpn_Clock_Remote: current version is 0x" << hex << CLOCK_VERSION 
	 << ", clock server reply msg uses version 0x" 
	 << ntohl(*(vrpn_int32 *)p.buffer) << "." << dec << endl;
    return -1;
  }

  // now grab the id from the message to check that is is correct
  if (me->lUniqueID!=(vrpn_int32)ntohl(plTimeData[5])) {
// Don't squawk: there could be multiple connections now
//    cerr << "vrpn_Clock_Remote: warning, server entertaining multiple clock" 
//	 << " sync requests simultaneously -- results may be inaccurate." 
//	 << endl;
    return 0;
  }
  
  // check that it is our type
  if (plTimeData[6]!=(vrpn_int32)ntohl(VRPN_CLOCK_FULL_SYNC)) {
    // ignore quick sync messages if they are going at same time
    //    cerr << "not full sync:temporary message to check that it is working" << endl;
    return 0;
  }

  me->cBounces++;

  //  printTime( "\nnow (local)", tvLNow );
  
  // copy local time for when bounce request was sent
  struct timeval tvLReq;
  tvLReq.tv_sec = (vrpn_int32) ntohl(plTimeData[3]);
  tvLReq.tv_usec = ntohl(plTimeData[4]);
  
  //  printTime( "bounce req sent (local)", tvLReq );
  
  // the p.msg_time field gets adjusted by the clock offset, so we
  // use a separate copy which the clock server stuffs into its record
  struct timeval tvRRep;
  tvRRep.tv_sec = (vrpn_int32) ntohl(plTimeData[1]);
  tvRRep.tv_usec = ntohl(plTimeData[2]);

  // calc round trip time
  struct timeval tvRoundTrip = vrpn_TimevalDiff( tvLNow, tvLReq );
  
  //  vrpn_float64 dCurrRoundTripMsecs = tvRoundTrip.tv_sec*1000.0 + 
  //    tvRoundTrip.tv_usec/1000.0;
  //    cerr << "\nround-trip time was " << 
  //      (tvRoundTrip.tv_sec*1000.0 + tvRoundTrip.tv_usec/1000.0) 
  //	 << " msecs." << endl;
  
  //  printTime( "bounce reply sent (remote)", tvRRep ); 
  
  // Now approx calc local time when remote server replied to us ...
  // assume it occured halfway thru roundtrip time
  
  // cut round trip time in half
  struct timeval tvHalfRoundTrip = tvRoundTrip;
  
  tvHalfRoundTrip.tv_usec /= 2;
  if (tvHalfRoundTrip.tv_sec % 2) {
    // odd, so add half to usecs
    tvHalfRoundTrip.tv_usec += (vrpn_int32) (1e6/2);
  } 

  tvHalfRoundTrip.tv_sec /= 2;

  //  printTime( "half roundtrip ", tvHalfRoundTrip );

  struct timeval tvLServerReply;
  tvLServerReply.tv_sec = tvLReq.tv_sec + tvHalfRoundTrip.tv_sec;
  tvLServerReply.tv_usec = tvLReq.tv_usec +  tvHalfRoundTrip.tv_usec;
  
  if (tvLServerReply.tv_usec>=1e6) {
    tvLServerReply.tv_sec++;
    tvLServerReply.tv_usec -= (vrpn_int32) 1e6;
  }
  
#ifdef USE_REGRESSION
  // save all of the offsets for now
  me->rgdTimes[me->cBounces-1] = vrpn_TimevalMsecs(tvLNow);
  me->rgdOffsets[me->cBounces-1] = vrpn_TimevalMsecs(
			   vrpn_TimevalDiff( tvLServerReply, tvRRep ));
#endif

  //  printTime( "bounce reply sent (local approx)", tvLServerReply );
  
  // LATER: could check for drift, but probably not essential  
  
  if ((tvHalfRoundTrip.tv_sec < me->tvMinHalfRoundTrip.tv_sec) ||
      ((tvHalfRoundTrip.tv_sec == me->tvMinHalfRoundTrip.tv_sec) &&
       (tvHalfRoundTrip.tv_usec < me->tvMinHalfRoundTrip.tv_usec)) ||
      (me->cBounces==1)) {
    // use new offset
    // calc offset between clocks
    me->tvFullClockOffset = vrpn_TimevalDiff( tvLServerReply, tvRRep );
    me->tvMinHalfRoundTrip = tvHalfRoundTrip;
    //  printTime( "diff btwn local and remote", tvClockDiff );
  }
  
  return 0;
}

// we return to the user callback the clock offset calculated from the
// shortest roundtrip in recent times (last cMaxQuickRecords bounces)
// We use the min because it should give us the most accurate calculation of
// the clock offset.

int vrpn_Clock_Remote::quickSyncClockServerReplyHandler(void *userdata, 
							vrpn_HANDLERPARAM p) {

  // get current local time
  struct timeval tvLNow;
  
  gettimeofday(&tvLNow, NULL);

  // printTime("L now time", tvLNow);

  // all of the tv structs have an L (local) or R (remote) tag
  vrpn_Clock_Remote *me = (vrpn_Clock_Remote *)userdata;

  // look at timing data sent along
  vrpn_int32 *plTimeData = (vrpn_int32 *) p.buffer;

  // check clock version
  if ((p.payload_len==0) || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
    cerr << "vrpn_Clock_Remote: current version is 0x" << hex << CLOCK_VERSION 
	 << ", clock server reply msg uses version 0x" 
	 << ntohl(*(vrpn_int32 *)p.buffer) << "." << dec << endl;
    return -1;
  }

  // now grab the id from the message to check that is is correct
  if (me->lUniqueID!=(vrpn_int32)ntohl(plTimeData[5])) {
// Don't squawk: there could be multiple connections now
//    cerr << "vrpn_Clock_Remote: warning, server entertaining multiple clock" 
//	 << " sync requests simultaneously -- results may be inaccurate." 
//	 << endl;
    return 0;
  }
  
  // check that it is our type
  if (plTimeData[6]!=(vrpn_int32)ntohl(VRPN_CLOCK_QUICK_SYNC)) {
    // ignore full sync messages if they are going at the same time
    // cerr << "not quick sync:temporary message to check that it is working" << endl;
    return 0;
  }

  me->cQuickBounces++;
  
  
  // copy local time for when bounce request was sent
  struct timeval tvLReq;
  tvLReq.tv_sec = (vrpn_int32) ntohl(plTimeData[3]);
  tvLReq.tv_usec = ntohl(plTimeData[4]);

  // the p.msg_time field gets adjusted by the clock offset, so we
  // use a separate copy which the clock server stuffs into its record
  struct timeval tvRRep;
  tvRRep.tv_sec = (vrpn_int32) ntohl(plTimeData[1]);
  tvRRep.tv_usec = ntohl(plTimeData[2]);
  
  
  // printTime("L req time", tvLReq);
  // calc round trip time
  struct timeval tvRoundTrip = vrpn_TimevalDiff( tvLNow, tvLReq );
  
  // cut round trip time in half
  tvRoundTrip.tv_usec /= 2;
  if (tvRoundTrip.tv_sec % 2) {
    // odd, so add half to usecs
    tvRoundTrip.tv_usec += (vrpn_int32) (1e6/2);
  } 
  tvRoundTrip.tv_sec /= 2;
  
  //printTime( "half roundtrip ", tvRoundTrip );
  struct timeval tvLServerReply;
  tvLServerReply.tv_sec = tvLReq.tv_sec + tvRoundTrip.tv_sec;
  tvLServerReply.tv_usec = tvLReq.tv_usec +  tvRoundTrip.tv_usec;
  
  if (tvLServerReply.tv_usec>=1e6) {
    tvLServerReply.tv_sec++;
    tvLServerReply.tv_usec -= (vrpn_int32) 1e6;
  }
  
  //  printTime( "bounce reply sent (local approx)", tvLServerReply );
  // printTime( "bounce reply sent (R)", tvRRep );
  
  // LATER: could check for drift, but probably not essential  
  
  // calc offset between clocks
  // keep track of last cMaxDiffs reports, and use the min roundtrip of those
  me->rgtvHalfRoundTrip[me->irgtvQuick]=tvRoundTrip;
  me->rgtvClockOffset[me->irgtvQuick]= vrpn_TimevalDiff( tvLServerReply, tvRRep );

  //  printTime("newest", me->rgtvClockOffset[me->irgtvQuick]);

  me->irgtvQuick++;
  if (me->irgtvQuick==me->cMaxQuickRecords) {
    me->irgtvQuick=0;
  }

  // now find min round trip
  int cRecords = (me->cQuickBounces >= me->cMaxQuickRecords) ? me->cMaxQuickRecords :
    me->cQuickBounces;
  int iMin=0;
  for(int i=1;i<cRecords;i++) {
    if ((me->rgtvHalfRoundTrip[i].tv_sec < me->rgtvHalfRoundTrip[iMin].tv_sec) ||
	((me->rgtvHalfRoundTrip[i].tv_sec == me->rgtvHalfRoundTrip[iMin].tv_sec) &&
	 (me->rgtvHalfRoundTrip[i].tv_usec<me->rgtvHalfRoundTrip[iMin].tv_usec))) {
      iMin = i;
    }
  }

  // now call any user callbacks that want to know the time diff
  // when syncs occur
  vrpn_CLOCKCB cs;
  vrpn_CLOCKSYNCLIST *pHandlerInfo=me->change_list;
  
  cs.tvClockOffset =  me->rgtvClockOffset[iMin];
  cs.tvHalfRoundTrip = me->rgtvHalfRoundTrip[iMin];

  //  static ii=0;
  //  cerr.precision(4);
  //  cerr.setf(ios::fixed);

  //  cerr << ii << " " << vrpn_TimevalMsecs(cs.tvClockOffset) << endl;
  //  ii++;
  //  printTime("used", cs.tvClockOffset);
  
  // go thru list of user specified clock sync callbacks
  while (pHandlerInfo!=NULL) {
    pHandlerInfo->handler(pHandlerInfo->userdata, cs);
    pHandlerInfo = pHandlerInfo->next;
  }
  
  return 0;
}


/*****************************************************************************\
  $Log: vrpn_Clock.C,v $
  Revision 1.25  1999/11/15 22:53:58  helser
  Removes the vrpn_cygwin_hack.h file. It is not necessary if the header
  files are included in a certain order. If in doubt, include vrpn_Shared.h
  first, even before the system headers, and this fixes header conflicts in
  the cygwin environment.

  Also removes /afs/unc/proj/hmd from the makefile, because VRPN does not
  depend on those directories at all, and it will cause problems if a header
  is removed, like I did.

  Revision 1.24  1999/10/14 17:29:50  helser
  No, changing the clock messages to a system type was a bad idea. The last
  commit actually disabled clock syncing entirely (which fixed my problem,
  but...).  I plan to change file_connection so it will ignore clock sync
  messages when setting the log file start time during replay.

  Revision 1.23  1999/10/14 15:54:21  helser
  Changes vrpn_FileConnection so it will allow local logging. I used this
  ability to write a log-file manipulator. I read in a log file, check and
  apply a fix to the timestamps, then send out an identical message to the
  one I received on the file connection, which gets logged. Problem: I don't
  get system messages in the new log file.

  Played with gettimeofday in Cygwin environment, and found it is broken and
  can't be fixed using Han's tricks in vrpn_Shared.C. But, instead found a
  way to get rid of vrpn_cygwin_hack.h - just include sys/time.h AFTER the
  standard windows headers. vrpn_cygwin_hack.h will be totally removed in my
  next commit.

  Added quick hack to make Clock messages system messages. This allows clock
  sync to happen before any user messages are send, if you spin the
  connection's mainloop a few times on each end, and allows the
  File_Connection to replay a log of the session correctly later. This is
  the work-around for the broken Cygwin gettimeofday.

  Revision 1.22  1999/10/08 22:29:54  taylorr
  Shutdown/restart server w/o restarting app
  	If the client starts before the server, this is okay -- it will retry
  	If the server stops, the client will restore the connection later
  	If you are logging on the client, it logs before and after reconnect
  		The break is marked in the log file, in case we need to know
  		this for some reason.
  	Both fullsync and interval clock synchronization work
  	Client gets conection dropped and started messages

  Other fixes along the way:
  	The clock server unregisters only messages that it registered.
  	Logging won't segfault when writing if it can't open the file.
  	Fixed a couple of compiler warnings in vrpn_Connection.C
  	Connection will not establish itself if it can't do local logging.

  This involved some major surgery to the vrpn_Connection implementation.
  Some of the surgery removed old scar tissue, but most of it was new.

  Revision 1.21  1999/09/21 22:15:00  taylorr
  This corrects the major bug that you could not delete vrpn_Remote objects before
  without causing seg faults when their callbacks were yanked after they were
  deleted. All objects now remove their callbacks when deleted.

  This also adds a test_vrpn application that will verify that you can
  both send and receive messages from the various test objects (tracker,
  button, dial, etc) and that you can delete and re-create Remote objects
  without dumping core.

  This also adds a Dial example server, and lets you create it in the
  generic server (as documented in vrpn.cfg.SAMPLE.

  Revision 1.20  1999/08/23 15:39:32  taylorr
  Fixes to allow easier compilation on sparc_solaris
  Client_and_server example program added
  Added sleep commands to the extended Fastrak reset commands

  Revision 1.19  1999/04/14 21:23:37  lovelace
  Added #ifdefs so that vrpn will compile under the Cygnus solutions
  cygwin environment.  Also modified Makefile so that it correctly
  compiles under the cygwin environment.

  Revision 1.18  1999/04/11 19:54:56  helser
  Removed an annoying printf.

  Revision 1.17  1999/04/08 12:49:00  hudson
  Exposed round-trip-time estimation in vrpn_Clock.
  More details for FreeBSD port.
  Bugfixes for some of the new constraint functionality on vrpn_ForceDevice.

  Revision 1.16  1999/04/01 19:43:13  winston
  Finished changes so that the timeout given as a parameter to mainloop
  is meaningful for clients (remote devices).  Before it wasn't getting
  passed by the synchronized connection's mainloop.

  Revision 1.15  1999/03/13 17:00:56  winston
  Bug fix, initialize things to NULL that aren't used in fullsync mode

  Revision 1.14  1999/02/24 15:58:26  taylorr
  BIG CHANGES.
  I modified the code so that it can compile on 64-bit SGI machines.

  To do so, I did what I should have done in the first place and defined
  architecture-independent types (vrpn_in32, vrpn_int16, vrpn_float32,
  vrpn_float64 and so on).  These are defined per architecture in the
  vrpn_Shared.h file.

  FROM NOW ON, folks should use these rather than the non-specific types
  (int, long, float, double) that may vary between platforms.

  Revision 1.13  1998/12/02 14:08:13  taylorr
  This version packs the things needed for a single connection into the
  vrpn_OneConnection structure called 'endpoint' within a vrpn_Connection.
  This should make it much easier to allow multiple connections to the same
  server by duplication of the endpoint.  I'm checking it in at this point
  because it still works.

  Revision 1.12  1998/09/24 04:29:34  gregory
  Updated the forceDevice to handle dynamic triangular meshes,
  as well as to allow for the user to chose between the Ghost
  and Hcollide libraries for the collision detection.

  Revision 1.11  1998/06/26 15:48:53  hudson
  Wrote vrpn_FileConnection.
  Changed connection naming convention.
  Changed all the base classes to reflect the new naming convention.
  Added #ifdef sgi around vrpn_sgibox.

  Revision 1.10  1998/04/02 23:16:57  weberh
  Modified so quick sync messages get sent immediately rather than waiting
  a cycle.

  Revision 1.9  1998/03/23 17:06:43  weberh
  clock changes:

  fixed up clock constructor to allow proper independent clock server/client
  creation.

  added code to do fullsync regression for offset slope estimation (added,
  but not currently used because it is not beneficial enough).

  fixed up a few comments and a minor bug which affected quick sync accuracy.


  connection changes:

  added -2 as a sync connection freq arg which performs an immediate
  fullsync.

  Revision 1.8  1998/03/19 06:07:01  weberh
  *** empty log message ***

  Revision 1.7  1998/03/19 06:04:38  weberh
  added a fullSync call to vrpn_Synchronized connection for convenience.

  Revision 1.6  1998/03/18 20:24:30  weberh
  vrpn_Clock -- added better unique id for clock sync messages

  vrpn_Connection -- fixed error which caused calls to get_connection_by_name
                     to create multiple connections.

  Revision 1.5  1998/02/20 20:26:49  hudson
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

  Revision 1.4  1998/01/20 20:07:22  weberh
  cleaned up vrpn_Shared func names so they won't collide with other libs.

  Revision 1.3  1998/01/20 15:53:26  taylorr
  	This version allows the client-side compilation of VRPN on NT.

  Revision 1.2  1998/01/08 23:32:48  weberh
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

  Revision 1.1  1997/12/15 21:25:08  weberh
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
