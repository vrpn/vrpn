#include "vrpn_ClientConnectionController.h"

#include "vrpn_NetConnection.h"
#include "vrpn_NewFileConnection.h"

////////////////////////////////////////////////
// List of controllers that are already open
//
//  struct vrpn_KNOWN_CONTROLLER {
//      char name[1000];                    // The name of the connection
//      vrpn_ClientConnectionController *c; // The conntroller
//      vrpn_KNOWN_CONTROLLER *next;        // Next on the list
//  };

//  static vrpn_KNOWN_CONTROLLER *known_controllers = NULL;


///////////////////////////////////////////////
// handler that sets the clock offsets for
// synchronize clocks between client and server
//
void vrpn_setClockOffset(void *userdata, const vrpn_CLOCKCB& info )
{
#if 0
    cerr << "clock offset is " << vrpn_TimevalMsecs(info.tvClockOffset) 
         << " msecs (used round trip which took " 
         << 2*vrpn_TimevalMsecs(info.tvHalfRoundTrip) << " msecs)." << endl;
#endif
    (*(struct timeval *) userdata) = info.tvClockOffset;
}

//	This routine will return a pointer to the connection whose name
// is passed in.  If the routine is called multiple times with the same
// name, it will return the same pointer, rather than trying to make
// multiple of the same connection.  If the connection is in a bad way,
// it returns NULL.
//	This routine will strip off any part of the string before and
// including the '@' character, considering this to be the local part
// of a device name, rather than part of the connection name.  This allows
// the opening of a connection to "Tracker0@ioglab" for example, which will
// open a connection to ioglab.

// this now always creates synchronized connections, but that is ok
// because they are derived from connections, and the default 
// args of freq=-1 makes it behave like a regular connection

// {{{ vrpn_get_connection_by_name() ...
vrpn_ClientConnectionController * vrpn_get_connection_by_name(
    const char *  cname,
    const char *  local_logfile_name,
    vrpn_int32    local_log_mode,
    const char *  remote_logfile_name,
    vrpn_int32    remote_log_mode,
    vrpn_float64  dFreq,
    vrpn_int32    cSyncWindow)
{
    vrpn_ClientConnectionController* ccc;
    char* where_at; // Part of name past last '@'
    vrpn_int32 port;

    if (cname == NULL) {
        fprintf(stderr,"vrpn_get_connection_by_name(): NULL name\n");
        return NULL;
    }

    // Find the relevant part of the name (skip past last '@'
    // if there is one)
    if ( (where_at = strrchr(cname, '@')) != NULL) {
        cname = where_at+1;	// Chop off the front of the name
    }

    // See if the connection is one that is already open.
    ccc = vrpn_ConnectionControllerManager::instance().getByName(cname);
    
    // If its not already open, open it and add it to the list.
    if (ccc == NULL) {
        
        port = vrpn_get_port_number(cname);
        
        // connections now self-register in the known list --
        // this is kind of odd, but oh well (can probably be done
        // more cleanly later).
        
        // Just create a ClientConnectionController, it will decided
        // whether to create a vrpn_FileConnection or vrpn_NetConnection
        new vrpn_ClientConnectionController(
            local_logfile_name,
            local_log_mode,
            remote_logfile_name, 
            remote_log_mode,
            dFreq, 
            cSyncWindow );
        
        // ClientConnectionController adds itself to the list of
        // connections in its constructor
        
        // make connection to server
        ccc = vrpn_ConnectionControllerManager::instance().getByName(cname);
        if (ccc->connect_to_server(    
            cname,
            port,
            local_logfile_name, 
            local_log_mode,
            remote_logfile_name, 
            remote_log_mode,
            vrpn_CONNECTION_TCP_BUFLEN,
            vrpn_CONNECTION_TCP_BUFLEN,
            vrpn_CONNECTION_UDP_BUFLEN) == -1)
        {
            return NULL;
        }
    }

    // See if the connection is okay.  If so, return it.  If not, NULL
    ccc = vrpn_ConnectionControllerManager::instance().getByName(cname);
    if (ccc->all_connections_doing_okay()) {
        return ccc;
    } else {
        return NULL;
    }
}
// }}} end vrpn_get_connection_by_name()



//==========================================================================
//==========================================================================
//
// {{{ vrpn_ClientConnectionController: public: c'tors and d'tors

//
//==========================================================================
//==========================================================================

vrpn_ClientConnectionController::vrpn_ClientConnectionController(
	const char * local_logfile_name, 
	vrpn_int32 local_log_mode,
	const char * remote_logfile_name, 
	vrpn_int32 remote_log_mode,
	vrpn_float64 dFreq, 
	vrpn_int32 cSyncWindow):
	vrpn_BaseConnectionController(
		local_logfile_name,
		local_log_mode,
		remote_logfile_name,
		remote_log_mode,
		dFreq,
		cSyncWindow)
{

    
    //-----------------------------------------------------------------
    // initializations done by both clock client and server 
    //

    // If the connection is valid, use it to register this clock by
    // name, the query by name, and the reply by name
    clockServer_id = register_service("clockServer");

    queryMsg_id = vrpn_CONNECTION_CLOCK_QUERY;
    replyMsg_id = vrpn_CONNECTION_CLOCK_REPLY;
    
//      queryMsg_id = register_message_type("clock query");
//      replyMsg_id = register_message_type("clock reply");
    if ( (clockServer_id == -1) || (queryMsg_id == -1) || (replyMsg_id == -1) ) {
        cerr << "vrpn_ClientConnectionController: Can't register IDs for synch clocks" 
             << endl;
        return;
    }
  
    //------------------------------------
    // client specific synch clock inits

    register_clock_sync_handler( &tvClockOffset, 
                                 vrpn_setClockOffset );
    // -2 as freq tells connection to immediately perform
    // a full sync to calc clock offset accurately.
    if (dFreq==-2) {
        // do full sync
        fullSync();
        mainloop();
    }
    
    init_clock_client();

	// add self to list of known controllers
    vrpn_ConnectionControllerManager::instance().addController(this, NULL);
//  	if (cname) {
//  		vrpn_KNOWN_CONTROLLER *curr;
		
//  		if ( (curr = new(vrpn_KNOWN_CONTROLLER)) == NULL) {
//  			fprintf(stderr, "vrpn_ClientConnectionController:  Out of memory.\n");
//  			return;
//  		}
//  		strncpy(curr->name, cname, sizeof(curr->name));
//  		curr->c = this;
//  		curr->next = known_controllers;
//  		known_controllers = curr;
//  	}

}
	
	

vrpn_ClientConnectionController::~vrpn_ClientConnectionController () {

	// delete connection
	delete d_connection_ptr;

    // Remove myself from the "known connections" list
    //   (or the "anonymous connections" list).
    vrpn_ConnectionControllerManager::instance().deleteController(this);

//  	// Remove myself from the "known connections" list.
//  	vrpn_KNOWN_CONTROLLER **snitch = &known_controllers;
//  	vrpn_KNOWN_CONTROLLER *victim = *snitch;
//  	while ( (victim != NULL) && (victim->c != this) ) {
//  		snitch = &( (*snitch)->next );;
//  		victim = victim->next;
//  	};
//  	if (victim == NULL) {        
//          // Disabling this warning because servers aren't put on the
//          // known list.  We coudl add a flag of some sort to know if
//          // we are a server, and warn only if not...  -PBW
//          //fprintf(stderr, "VRPN:~vrpn_Connection: Can't find "
//          //                "myself on known list\n");
//  	} else {
//  		*snitch = victim->next;
//  		delete victim;
//  	}

    //------------------------------------
    // clock client stuff
    
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


vrpn_int32 vrpn_ClientConnectionController::connect_to_server(
    const char*  cname,
    vrpn_int16   port,
    const char*  local_logfile_name, 
    vrpn_int32   local_log_mode,
    const char*  remote_logfile_name, 
    vrpn_int32   remote_log_mode,
    vrpn_int32   tcp_inbuflen,
    vrpn_int32   tcp_outbuflen,
    vrpn_int32   udp_outbuflen)
{
    const char* machinename;
    vrpn_int32 isfile;
    vrpn_int32 isrsh;
    
    isfile = (strstr(cname, "file:") ? 1 : 0);
    isrsh = (strstr(cname, "x-vrsh:") ? 1 : 0);
    
    if (!isfile && !isrsh) {
        // Open a connection to the station using VRPN
        machinename = vrpn_copy_machine_name(cname);
        if (!machinename) {
            fprintf(stderr, "vrpn_ClientConnectionController:  "
                    "Out of memory!\n");
            return -1;
        }
        if (port < 0) {
            port = vrpn_DEFAULT_LISTEN_PORT_NO;
        }
        
        // create NetConnection here
        d_connection_ptr 
            = new vrpn_NetConnection(
                this->new_RestrictedAccessToken (this),
                local_logfile_name,
                local_log_mode,
                remote_logfile_name,
                remote_log_mode,
                tcp_inbuflen,
                tcp_outbuflen,
                //udp_inbuflen,
                udp_outbuflen);
        d_connection_ptr->connect_to_server(cname,port);
        
        if (machinename)
            delete [] (char *) machinename;
    }
    if (isrsh) {
        // Start up the server and wait for it to connect back
        char *server_program;
        char *server_args;   // server program plus its arguments
        char *token;
        
        machinename = vrpn_copy_machine_name(cname);
        server_program = vrpn_copy_rsh_program(cname);
        server_args = vrpn_copy_rsh_arguments(cname);
        token = server_args;
        // replace all argument separators (',') with spaces (' ')
        while (token = strchr(token, ','))
            *token = ' ';
        
        // start server on remote machine
        d_connection_ptr->start_server(machinename, server_program, 
                                       server_args);
        
        if (machinename) delete [] (char *) machinename;
        if (server_program) delete [] (char *) server_program;
        if (server_args) delete [] (char *) server_args;
        
    }
    
    // create FileConnection Here
    if (isfile) {        
        d_connection_ptr = 
            new vrpn_NewFileConnection(
                this->new_RestrictedAccessToken (this),
                cname,
                local_logfile_name,
                local_log_mode);
    }

    return 0;
}

// }}} end c'tors and d'tors
// 
//==========================================================================
//==========================================================================
//
// {{{ vrpn_ClientConnectionController: public: mainloop

//
//==========================================================================
//==========================================================================

vrpn_int32 vrpn_ClientConnectionController::mainloop(
    const timeval * timeout)
{
    vrpn_int32 retval;
    // don't know what else will go in here
    
    // do this before calling BaseConnection mainloop 
    // because it creates messages that need to be sent
    synchronize_clocks();
    
    switch( status ){
    case vrpn_CONNECTION_CONNECTED:
        // call BaseConnection mainloop
        retval = d_connection_ptr->mainloop(timeout); 
        break;
        
    }
    return retval;
}

// }}} end mainloop
//
//==========================================================================
//==========================================================================
//
// vrpn_ClientConnectionController: public: logging
//
//==========================================================================
//==========================================================================

vrpn_int32 vrpn_ClientConnectionController::register_log_filter (
    vrpn_LOGFILTER filter, 
    void * userdata)
{
    return d_connection_ptr->register_log_filter (filter,userdata);
}

 
//==========================================================================
//==========================================================================
//
// vrpn_ClientConnectionController: public: status
//
//==========================================================================
//==========================================================================

// are there any connections?
vrpn_int32 vrpn_ClientConnectionController::at_least_one_open_connection() const
{
    return d_connection_ptr->connected();
}

// overall, all connections are doing okay
vrpn_int32 vrpn_ClientConnectionController::all_connections_doing_okay() const
{
    return d_connection_ptr->doing_okay();
}       

//==========================================================================
//==========================================================================
//
// vrpn_ClientConnectionController: public : sending and receiving
//
//==========================================================================
//==========================================================================


// * send pending report (that have been packed), and clear the buffer
// * this function was protected, now is public, so we can use
//   it to send out intermediate results without calling mainloop
vrpn_int32 vrpn_ClientConnectionController::send_pending_reports()
{
    return d_connection_ptr->send_pending_reports();
}
    
// * pack a message that will be sent the next time mainloop() is called
// * turn off the RELIABLE flag if you want low-latency (UDP) send
// * was: pack_message
vrpn_int32 vrpn_ClientConnectionController::queue_outgoing_message(
    vrpn_uint32 len, 
    timeval time,
    vrpn_int32 type,
    vrpn_int32 service,
    const char * buffer,
    vrpn_uint32 class_of_service )
{
    vrpn_int32 ret;
    
    // Make sure type is either a system type (-) or a legal user type
    if ( type >= get_num_my_types() ) {
        printf("vrpn_ClientConnectionController::queue_outgoing_message: bad type (%ld)\n",
               type);
        return -1;
    }
    
    // If this is not a system message, make sure the service is legal.
    if (type >= 0) {
        if ( (service < 0) || (service >= get_num_my_services() ) ) {
            printf("vrpn_ClientConnectionController::queue_outgoing_message: bad service (%ld)\n",
                   service);
            return -1;
        }
    }
    
    // See if there are any local handlers for this message type from
    // this sender.  If so, yank the callbacks.
    if (do_callbacks_for(type, service, time, len, buffer)) {
        return -1;
    }
    
    // send on to Connection
    vrpn_bool sent_mcast = vrpn_false;
    ret = d_connection_ptr->queue_outgoing_message(
        len,time,type,service,buffer,
        class_of_service,sent_mcast);
    if( ret == -1 ) return -1; 
    
    return 0;
}


//==========================================================================
//==========================================================================
//
// {{{ vrpn_ClientConnectionController: public : clock synch functions
//
//==========================================================================
//==========================================================================

// do all inits that are done in
// vrpn_Clock_Remote constructor
void vrpn_ClientConnectionController::init_clock_client()
{
    char rgch [50];
    vrpn_int32 i;
    
    change_list = NULL;
    
    if (d_connection_ptr == NULL) {
        cerr << "vrpn_ClientConnectionController: unable to connect to clock server" 
             << endl;
        return;
    }
    // not sure what this translates to
    sprintf( rgch, "%ld", (vrpn_int32) this );
    clockClient_id = register_service(rgch);
    
    if (clockClient_id == -1) {
        cerr << "vrpn_ClientConnectionController: Can't register ID" << endl;
        d_connection_ptr = NULL;
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
        if (register_handler(replyMsg_id, 
                             quickSyncClockServerReplyHandler,
                             (void *)this, clockServer_id)) {
            cerr << "vrpn_ClientConnectionController: Can't register handler" << endl;
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


// this will take 1 second to run (sync's the clocks)
// when it is in fullSync mode
// mostly code from vrpn_Clock_Remote::mainloop()
void vrpn_ClientConnectionController::synchronize_clocks()
{
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
            rgl[1]=htonl(lUniqueID);    // An easy, unique ID
            rgl[2]=htonl((vrpn_int32)VRPN_CLOCK_QUICK_SYNC);
            gettimeofday(&tv, NULL);
            queue_outgoing_message(3*sizeof(vrpn_int32), tv, queryMsg_id, 
                                    clockClient_id, (char *)rgl, 
                                    vrpn_CONNECTION_RELIABLE);
            tvQuickLastSync = tvNow;
            
            // send out the clock sync messages right away
            d_connection_ptr->mainloop();
        }
    }
    
    // any time a full clock sync has been requested, do it for 1 sec
    if (fDoFullSync) {
        fDoFullSync=0;
        // cerr << "FullSync" << endl;      
        // register a handler for replies from the clock server.
        if (register_handler(replyMsg_id, 
                             fullSyncClockServerReplyHandler,
                             (void *) this, clockServer_id)) {
            cerr << "vrpn_ClientConnectionController: Can't register handler" << endl;
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

        cerr << "vrpn_ClientConnectionController: performing fullsync calibration, "
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
                cerr << "vrpn_ClientConnectionController::mainloop: multiple clock servers on "
                    "connection -- aborting fullSync " <<cBounces<< endl;
                if (unregister_handler(replyMsg_id, 
                                       fullSyncClockServerReplyHandler,
                                       (void *)this, clockServer_id)) {
                    cerr << "vrpn_ClientConnectionController: Can't unregister handler" << endl;
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
                rgl[1]=htonl(lUniqueID);    // An easy, unique ID
                rgl[2]=htonl((vrpn_int32)VRPN_CLOCK_FULL_SYNC);
                gettimeofday(&tv, NULL);
                queue_outgoing_message(3*sizeof(vrpn_int32), tv, queryMsg_id, 
                                        clockClient_id, (char *)rgl, 
                                        vrpn_CONNECTION_RELIABLE);
                cNextTime+=cInterval;
            }
    
            // actually process the message and any callbacks from local
            // or remote messages (just call the base mainloop)
            d_connection_ptr->mainloop();
            gettimeofday( &tvNow, NULL );
            cElapsedMsecs = (vrpn_int32) vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvCalib));
        }
        // we don't care if we missed numerous replies, just make sure we had
        // at least half back already
        
        if (cBounces<((cCalibMsecs/cInterval)/2)) {
            cerr << "vrpn_ClientConnectionController::mainloop: calib error; did not receive "
                "replies for at least half of the bounces" << endl;
            return;
        } 

        // actually, we just want to use the min roundtrip record to calc the
        // offset so we can do it interactively, and the clock offset calced
        // from that record is in tvFullClockOffset
        
        // now get rid of handler -- no need to calibrate any longer
        // so don't waste time checking the ids of other's clock server replies.
        
        if (unregister_handler(replyMsg_id, 
                               fullSyncClockServerReplyHandler,
                               (void *)this, clockServer_id)) {
            cerr << "vrpn_ClientConnectionController: Can't unregister handler" << endl;
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
        vrpn_int32 n = cBounces;

        // ignore the first ten bounces
        vrpn_int32 cIgnore = 10;

        vrpn_float64 dN = n - cIgnore;

      
        // do a scale on the times to get better 
        // use of floating point range and to find
        // slope in uSec per sec.
        vrpn_float64 dInitTime = rgdTimes[0];

        for (vrpn_int32 irgd=cIgnore;irgd<n;irgd++) {
            rgdTimes[irgd]-=dInitTime;
            rgdTimes[irgd]/=1000.0;
            rgdOffsets[irgd]*=1000.0;
            //  cerr << irgd << "\ttime " << rgdTimes[irgd] 
            //       << "\toffset " << rgdOffsets[irgd] 
            //       << endl;

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

        cerr << "regression coeffs are (int (usec), slope (usec/sec)) " 
             << dIntercept << ", " << dSlope << endl;
      
        vrpn_float64 dSEE = 0;
        vrpn_float64 dSXMXM = 0;

        for (vrpn_int32 i=cIgnore;i<n;i++) {
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
        cerr << "vrpn_ClientConnectionController::mainloop: clock offset is " 
             << tvFullClockOffset.tv_sec << "s, "
             << tvFullClockOffset.tv_usec << "us." << endl;
        cerr << "vrpn_ClientConnectionController::mainloop: half round trip is " 
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

vrpn_int32 
vrpn_ClientConnectionController::register_clock_sync_handler(
    void *userdata,
    vrpn_CLOCKSYNCHANDLER handler)
{
    vrpn_CLOCKSYNCLIST  *new_entry;
    
    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        cerr << "vrpn_ClientConnectionController::register_clock_sync_handler:" 
             << " NULL handler" << endl;
        return -1;
    }
    
    // Allocate and initialize the new entry
    if ( (new_entry = new vrpn_CLOCKSYNCLIST) == NULL) {
        cerr << "vrpn_ClientConnectionController::register_clock_sync_handler:" 
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

vrpn_int32 
vrpn_ClientConnectionController::unregister_clock_sync_handler(
    void *userdata, vrpn_CLOCKSYNCHANDLER handler)
{
    // The pointer at *snitch points to victim
    vrpn_CLOCKSYNCLIST  *victim, **snitch;
    
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
        fprintf(stderr,
                "vrpn_ClientConnectionController::unregister_handler: No such handler\n");
        return -1;
    }
  
    // Remove the entry from the list
    *snitch = victim->next;
    delete victim;
    
    return 0;
}

// return value was added to match signature found in
// vrpn_SynchronizedConnection::fullSync() - sain 8/99
struct timeval vrpn_ClientConnectionController::fullSync () 
{
    fDoFullSync=1;
    fDoQuickSyncs=0;
    return tvClockOffset;
}

// Returns the most recent RTT estimate.  TCH April 99
// Returns 0 if the first RTT estimate is not yet completed
// or if no quick syncs are being done.
// RTT is 2 * most recent estimate of half-RTT.

struct timeval vrpn_ClientConnectionController::currentRTT () const 
{
    struct timeval retval;
    vrpn_int32 last;

    // Assumes values are initialized to 0 or otherwise safe.
    // (Ensured by a clause in the constructor).

    retval.tv_sec = 0L;
    retval.tv_usec = 0L;

    if (!rgtvHalfRoundTrip)
        return retval;

    last = this->irgtvQuick - 1;
    while (last < 0) {
        last += cMaxQuickRecords;
    }
    retval = vrpn_TimevalScale(this->rgtvHalfRoundTrip[last], 2.0);
    //fprintf(stderr, "Record %d is %ld:%ld.\n",
    //        last, retval.tv_sec, retval.tv_usec);

    return retval;
}

//==========================================================================
//==========================================================================
//
// vrpn_ClientConnectionController: protected : clock synch functions
//
//==========================================================================
//==========================================================================


// we return to the user callback the clock offset calculated from the
// shortest roundtrip over all of the full sync replies.
// We use the min because it should give us the most accurate calculation of
// the clock offset.

vrpn_int32 
vrpn_ClientConnectionController::fullSyncClockServerReplyHandler(
    void *userdata, vrpn_HANDLERPARAM p) 
{
    // get current local time
    struct timeval tvLNow;
    
    gettimeofday(&tvLNow, NULL);
  
    // all of the tv structs have an L (local) or R (remote) tag
    vrpn_ClientConnectionController *me
        = (vrpn_ClientConnectionController *)userdata;

    // look at timing info sent along
    vrpn_int32 *plTimeData = (vrpn_int32 *) p.buffer;

    // check clock version
    if (  (p.payload_len==0)
          || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
        cerr << "vrpn_ClientConnectionController: current version is 0x"
             << hex << CLOCK_VERSION 
             << ", clock server reply msg uses version 0x" 
             << ntohl(*(vrpn_int32 *)p.buffer) << "." << dec << endl;
        return -1;
    }

    // now grab the id from the message to check that is is correct
    if (me->lUniqueID!=(vrpn_int32)ntohl(plTimeData[5])) {
        // Don't squawk: there could be multiple connections now
        //    cerr << "vrpn_ClientConnectionController: warning, server entertaining multiple clock" 
        //   << " sync requests simultaneously -- results may be inaccurate." 
        //   << endl;
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
    //   << " msecs." << endl;
  
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
    me->rgdOffsets[me->cBounces-1] = 
        vrpn_TimevalMsecs(vrpn_TimevalDiff( tvLServerReply, tvRRep ));
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

vrpn_int32 
vrpn_ClientConnectionController::quickSyncClockServerReplyHandler(
    void *userdata, vrpn_HANDLERPARAM p) 
{
    // get current local time
    struct timeval tvLNow;
    
    gettimeofday(&tvLNow, NULL);
    
    // printTime("L now time", tvLNow);
    
    // all of the tv structs have an L (local) or R (remote) tag
    vrpn_ClientConnectionController *me
        = (vrpn_ClientConnectionController *)userdata;
    
    // look at timing data sent along
    vrpn_int32 *plTimeData = (vrpn_int32 *) p.buffer;
    
    // check clock version
    if (  (p.payload_len==0)
          || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {
        cerr << "vrpn_ClientConnectionController: current version is 0x"
             << hex << CLOCK_VERSION 
             << ", clock server reply msg uses version 0x" 
             << ntohl(*(vrpn_int32 *)p.buffer) << "." << dec << endl;
        return -1;
    }
    
    // now grab the id from the message to check that is is correct
    if (me->lUniqueID!=(vrpn_int32)ntohl(plTimeData[5])) {
        // Don't squawk: there could be multiple connections now
        //    cerr << "vrpn_ClientConnectionController: warning, server entertaining multiple clock" 
        //   << " sync requests simultaneously -- results may be inaccurate." 
        //   << endl;
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
    vrpn_int32 cRecords = (me->cQuickBounces >= me->cMaxQuickRecords) ? me->cMaxQuickRecords :
        me->cQuickBounces;
    vrpn_int32 iMin=0;
    for(vrpn_int32 i=1;i<cRecords;i++) {
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

// }}} end clock sync functions
// 
//==========================================================================
//==========================================================================
//
// {{{ services and types
// 
//==========================================================================
//==========================================================================

void vrpn_ClientConnectionController::register_service_with_connections(
    const char * service_name, 
    vrpn_int32 service_id )
{
    // XXX fill in

#if 0  // was ...

    // If we're connected, pack the service description
    if (connected()) {
        d_connection_ptr->pack_service_description(num_my_services - 1);
    }

    // If the other side has declared this service, establish the
    // mapping for it.
    d_connection_ptr->register_local_service(name, num_my_services - 1);

#else  // changing it to ...
    
    // If we're connected, pack the service description
    if (d_connection_ptr) {
        d_connection_ptr->pack_service_description( /*service_name,*/ service_id );
    }

    // If the other side has declared this service, establish the
    // mapping for it.
    d_connection_ptr->register_local_service( service_name, service_id );
    
#endif // ... end change
}


void vrpn_ClientConnectionController::register_type_with_connections(
    const char * type_name, vrpn_int32 type_id )
{
    // XXX fill in

#if 0   // was ...

    // If we're connected, pack the type description
    if (connected()) {
        pack_type_description(num_my_types-1);
    }

    if (d_connection_ptr->register_local_type(name, num_my_types - 1)) {  //XXX-JJ
        my_types[num_my_types - 1].cCares = 1;  // TCH 28 Oct 97
    }

#else  // chaging it to ...

    if (d_connection_ptr) {
        d_connection_ptr->pack_type_description( type_id );
        
        //XXX???
        if (d_connection_ptr->register_local_type( type_name, type_id )) {
            my_types[type_id].cCares = 1;
        }
    }
    
#endif  // ... end change

}

// }}} end services and types
//
//==========================================================================
//==========================================================================


//===========================================================================
//
// {{{ vrpn_ConnectionControllerManager ...
//
//===========================================================================

vrpn_ConnectionControllerManager::~vrpn_ConnectionControllerManager() 
{
    //fprintf(stderr, "In ~vrpn_ConnectionControllerManager:  tearing down the list.\n");
    
    // Call the destructor of every known connection.
    // That destructor will call vrpn_ConnectionControllerManager::deleteConnection()
    // to remove itself from d_kcList.
    while (d_kcList) {
        delete d_kcList->controller;
    }
    while (d_anonList) {
        delete d_anonList->controller;
    }
}

// static
vrpn_ConnectionControllerManager &
vrpn_ConnectionControllerManager::instance() 
{
    static vrpn_ConnectionControllerManager manager;
    return manager;
}

void vrpn_ConnectionControllerManager::addController (
    vrpn_ClientConnectionController * c,
    const char * name) 
{
    knownController * p;
    
    p = new knownController;
    p->controller = c;
    
    if (name) {
        strncpy(p->name, name, 1000);
        p->next = d_kcList;
        d_kcList = p;
    } else {
        p->name[0] = 0;
        p->next = d_anonList;
        d_anonList = p;
    }
    
    d_numControllers++;
}

void vrpn_ConnectionControllerManager::deleteController (
    vrpn_ClientConnectionController * c) 
{
    deleteController(c, &d_kcList);
    deleteController(c, &d_anonList);
    d_numControllers--;
}

void vrpn_ConnectionControllerManager::deleteController (
    vrpn_ClientConnectionController * c,
    knownController ** snitch) 
{
    knownController * victim = *snitch;
    
    while (victim && (victim->controller != c)) {
        snitch = &((*snitch)->next);
        victim = *snitch;
    }
    
    if (!victim) {
        // No warning, because this connection might be on the *other* list.
    } else {
        *snitch = victim->next;
        delete victim;
    }
}

vrpn_ClientConnectionController * 
vrpn_ConnectionControllerManager::getByName (const char * name) 
{
    knownController * p;
    for (p = d_kcList; p && strcmp(p->name, name); p = p->next) {
        // do nothing
    }
    if (!p) {
        return NULL;
    }
    return p->controller;
}

vrpn_ConnectionControllerManager::vrpn_ConnectionControllerManager (void)
    : d_kcList (NULL),
      d_anonList (NULL),
      d_numControllers(0)
{
}

vrpn_int32 vrpn_ConnectionControllerManager::numControllers()
{
    return d_numControllers;
}

// }}} end vrpn_ConnectionControllerManager
//
//===========================================================================
