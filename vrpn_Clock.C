/*****************************************************************************\
  vrpn_Clock.C
  --
  Description :
  
  ----------------------------------------------------------------------------
  Author: weberh
  Created: Sat Dec 13 11:05:16 1997
  Revised: Wed Apr  1 13:23:40 1998 by weberh
  \*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"
#if !( defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS) )
#include <netinet/in.h>
#endif

#ifndef	VRPN_NO_STREAMS
#include <iostream.h>
#endif
#include <math.h>

#include "vrpn_Clock.h"

void printTime( char *pch, const struct timeval& tv ) {
    fprintf(stderr, "%s %g msecs.\n", pch, tv.tv_sec*1000.0 + tv.tv_usec/1000.0);
}

// both server and client call this constructor
vrpn_Clock::vrpn_Clock(const char * name, vrpn_Connection *c) :
    vrpn_BaseClass(name, c)
{
  vrpn_BaseClass::init();

  // need to init vrpn_gettimeofday for the pc
  struct timeval tv;
  vrpn_gettimeofday(&tv, NULL);
}

int vrpn_Clock::register_senders(void)
{
    if (d_connection == NULL) {
	return -1;
    }

    d_sender_id = d_connection->register_sender(d_servicename);
    if (d_sender_id == -1) {
      fprintf(stderr,"vrpn_Clock: Can't register sender ID\n");
      d_connection = NULL;
      return -1;
    }
    return 0;
}

int vrpn_Clock::register_types(void)
{
    if (d_connection == NULL) {
	return -1;
    }

    queryMsg_id = d_connection->register_message_type("vrpn_Clock query");
    replyMsg_id = d_connection->register_message_type("vrpn_Clock reply");
    if ( (queryMsg_id == -1) || (replyMsg_id == -1) ) {
      fprintf(stderr, "vrpn_Clock: Can't register type IDs\n");
      d_connection = NULL;
      return -1;
    }
    return 0;
}

/** Packs time of message sent and all data sent to it back into buffer.
    buf is outgoing buffer, tvSRep is current time, tvCReq is the time
    that the just-received message had in it, cChars is the length of
    the old message payload, and pch is a pointer to the old payload.
    When encoding the old payload, the first vrpn_int32 in the payload
    is skipped (it is the version number).

    Returns the number of characters encoded total.
*/

int vrpn_Clock::encode_to(char *buf, const struct timeval& tvSRep,
			  const struct timeval& tvCReq, 
			  vrpn_int32 cChars, const char *pch ) {

  char *bufptr = buf;
  int	buflen = 100;

  // pack version number, client time, and the old message time.
  vrpn_buffer( &bufptr, &buflen, CLOCK_VERSION );
  vrpn_buffer( &bufptr, &buflen, tvSRep );
  vrpn_buffer( &bufptr, &buflen, tvCReq );

  // Skip the version number from the old message
  pch += sizeof(vrpn_int32);
  cChars -= sizeof(vrpn_int32);

  // Pack the payload of the old message
  vrpn_buffer( &bufptr, &buflen, pch, cChars );

  // Tell how many characters we sent.
  return 100-buflen;
}


vrpn_Clock_Server::vrpn_Clock_Server(vrpn_Connection *c) 
  : vrpn_Clock("clockServer", c){

  // Register the callback handler (along with "this" as user data)
  // It will take messages from any sender (no sender arg specified)
  if (register_autodeleted_handler(queryMsg_id, clockQueryHandler, this)) {
    fprintf(stderr, "vrpn_Clock: can't register handler\n");
    d_connection = NULL;
    return;
  }
}

vrpn_Clock_Server::~vrpn_Clock_Server() 
{
}

// to prevent the user from getting back incorrect info, clients are
// (nearly) uniquely identified by a usec timestamp.
int vrpn_Clock_Server::clockQueryHandler(void *userdata, vrpn_HANDLERPARAM p) {
  vrpn_Clock_Server *me = (vrpn_Clock_Server *) userdata;
  static struct timeval now;
  char rgch[100];
  
  // send back time client sent request and the buffer the client sent
  //  cerr << ".";
  //  printTime("client req ",p.msg_time);

  // check clock version
  if ((p.payload_len==0) || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
    fprintf(stderr, "vrpn_Clock_Server: current version is 0x%x, clock query msg uses version 0x%x.\n",
	CLOCK_VERSION, ntohl(*(vrpn_int32 *)p.buffer) );
    return -1;
  }

  vrpn_gettimeofday(&now,NULL);
  //  printTime("server rep ", now);
  int cLen = me->encode_to( rgch, now, p.msg_time, p.payload_len, p.buffer );
  me->d_connection->pack_message(cLen, now,
			       me->replyMsg_id, me->d_sender_id,
			       rgch, vrpn_CONNECTION_RELIABLE);
  return 0;
}

// the clock server only responds to requests. since the connection
// mainloop will handle routing the callback (and the general server
// program always has to call that), this mainloop does nothing.
void vrpn_Clock_Server::mainloop () {};


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
  
  if (d_connection==NULL) {
    fprintf(stderr, "vrpn_Clock_Remote: unable to connect to clock server (%s)\n", name);
    return;
  }
  sprintf( rgch, "%ld", (long) this );
  clockClient_id = d_connection->register_sender(rgch);
  
  if (clockClient_id == -1) {
    fprintf(stderr,"vrpn_Clock_Remote: Can't register ID\n");
    d_connection = NULL;
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
    if (register_autodeleted_handler(replyMsg_id, 
				     quickSyncClockServerReplyHandler,
				     this, d_sender_id)) {
      fprintf(stderr,"vrpn_Clock_Remote: Can't register handler\n");
      d_connection = NULL;
    }
  }

  // establish unique id
  struct timeval tv;
  vrpn_gettimeofday(&tv, NULL);

  lUniqueID = (vrpn_int32)tv.tv_usec;

#ifdef USE_REGRESSION
  rgdOffsets = NULL;
  rgdTimes = NULL;
#endif
}

vrpn_Clock_Remote::~vrpn_Clock_Remote (void)
{
  // Release any handlers that have not been unregistered
  // by higher-level code.
  vrpn_CLOCKSYNCLIST	*curr, *next;
  curr = change_list;
  while (curr != NULL) {
    next = curr->next;
    delete curr;
    curr = next;
  }
  
  // release the quick arrays
  if (rgtvHalfRoundTrip) {
    delete [] rgtvHalfRoundTrip;
  }
  if (rgtvClockOffset) {
    delete [] rgtvClockOffset;
  }

#ifdef USE_REGRESSION

  // release the regression arrays
  if (rgdOffsets) {
    delete rgdOffsets;
  }
  if (rgdTimes) {
    delete rgdTimes;
  }

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
void vrpn_Clock_Remote::mainloop (const struct timeval *timeout)
{
  if (d_connection) { 
    // always do this -- the first time it will register senders & msg_types
    // Also, we don't want to call the Synchronized version (if this is
    // a sync connection) since we are already in that loop (this is 
    // the routine that does the synchronization)
    d_connection->vrpn_Connection::mainloop(timeout);
    
    if (fDoQuickSyncs) {
      // just a quick sync
      //      cerr << "QuickSync" << endl;      
      struct timeval tvNow;
      vrpn_gettimeofday(&tvNow, NULL);

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
	vrpn_gettimeofday(&tv, NULL);
	d_connection->pack_message(3*sizeof(vrpn_int32), tv, queryMsg_id, 
				 clockClient_id, (char *)rgl, 
				 vrpn_CONNECTION_RELIABLE);
	tvQuickLastSync = tvNow;
	    
	// send out the clock sync messages right away
	d_connection->vrpn_Connection::mainloop();
      }
    }

    // any time a full clock sync has been requested, do it for 1 sec
    if (fDoFullSync) {
      fDoFullSync=0;
      // cerr << "FullSync" << endl;      
      // register a handler for replies from the clock server.
      // We're using the Connection's registration directly (rather than
      // autodeletion) because we're going to unregister it before the
      // object is destroyed.
      if (d_connection->register_handler(replyMsg_id, 
				       fullSyncClockServerReplyHandler,
				       this, d_sender_id)) {
	fprintf(stderr,"vrpn_Clock_Remote: Can't register handler\n");
	d_connection = NULL;
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

      fprintf(stderr, "vrpn_Clock_Remote: performing fullsync calibration, "
	"this will take %g seconds.\n", cCalibMsecs/1000.0);

#ifdef USE_REGRESSION
      // save offsets and time of offset calc for regression
      if (!rgdOffsets) {
	rgdOffsets = new vrpn_float64[cQueries];
      }
      if (!rgdTimes) {
	rgdTimes = new vrpn_float64[cQueries];
      }
#endif
      vrpn_gettimeofday( &tvCalib, NULL );

      // do bounces for one second to calibrate the clocks
      //      while (cElapsedMsecs<=cCalibMsecs) {
      while ( iQueries < cQueries ) {

	// don't let replies overwrite the buffer
	if (cBounces>=cQueries) {
	  fprintf(stderr,"vrpn_Clock_Remote::mainloop: multiple clock servers on "
	    "connection -- aborting fullSync %d\n", cBounces);
	  if (d_connection->unregister_handler(replyMsg_id, 
					     fullSyncClockServerReplyHandler,
					     this, d_sender_id)) {
	    fprintf(stderr, "vrpn_Clock_Remote: Can't unregister handler\n");
	    d_connection = NULL;
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
	  vrpn_gettimeofday(&tv, NULL);
	  d_connection->pack_message(3*sizeof(vrpn_int32), tv, queryMsg_id, 
				   clockClient_id, (char *)rgl, 
				   vrpn_CONNECTION_RELIABLE);
	  cNextTime+=cInterval;
	}
	
	// actually process the message and any callbacks from local
	// or remote messages (just call the base mainloop)
	d_connection->vrpn_Connection::mainloop();
	vrpn_gettimeofday( &tvNow, NULL );
	cElapsedMsecs = (vrpn_int32) vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvCalib));
      }
      // we don't care if we missed numerous replies, just make sure we had
      // at least half back already
      
      if (cBounces<((cCalibMsecs/cInterval)/2)) {
	fprintf(stderr,"vrpn_Clock_Remote::mainloop: calib error; did not receive "
	  "replies for at least half of the bounces\n");
	return;
      } 

      // actually, we just want to use the min roundtrip record to calc the
      // offset so we can do it interactively, and the clock offset calced
      // from that record is in tvFullClockOffset
      
      // now get rid of handler -- no need to calibrate any longer
      // so don't waste time checking the ids of other's clock server replies.
      
      if (d_connection->unregister_handler(replyMsg_id, 
					 fullSyncClockServerReplyHandler,
					 this, d_sender_id)) {
	fprintf(stderr, "vrpn_Clock_Remote: Can't unregister handler\n");
	d_connection = NULL;
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
    fprintf(stderr, "vrpn_Clock_Remote::register_clock_sync_handler:" 
	 " NULL handler\n");
    return -1;
  }
  
  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_CLOCKSYNCLIST) == NULL) {
    fprintf(stderr, "vrpn_Clock_Remote::register_clock_sync_handler:" 
	 " out of memory\n");
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
  
  vrpn_gettimeofday(&tvLNow, NULL);
  
  // all of the tv structs have an L (local) or R (remote) tag
  vrpn_Clock_Remote *me = (vrpn_Clock_Remote *)userdata;

  // look at timing info sent along
  vrpn_int32 *plTimeData = (vrpn_int32 *) p.buffer;

  // check clock version
  if ((p.payload_len==0) || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
    fprintf(stderr, "vrpn_Clock_Remote: current version is 0x%x"
	", clock server reply msg uses version 0x%x\n", CLOCK_VERSION,
	 ntohl(*(vrpn_int32 *)p.buffer) );
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
  
  vrpn_gettimeofday(&tvLNow, NULL);

  // printTime("L now time", tvLNow);

  // all of the tv structs have an L (local) or R (remote) tag
  vrpn_Clock_Remote *me = (vrpn_Clock_Remote *)userdata;

  // look at timing data sent along
  vrpn_int32 *plTimeData = (vrpn_int32 *) p.buffer;

  // check clock version
  if ((p.payload_len==0) || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
    fprintf(stderr, "vrpn_Clock_Remote: current version is 0x%x"
	", clock server reply msg uses version 0x%x\n", CLOCK_VERSION,
	 ntohl(*(vrpn_int32 *)p.buffer) );
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

