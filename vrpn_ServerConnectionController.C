#include "vrpn_ServerConnectionController.h"



//==========================================================================
//==========================================================================
//
// vrpn_ServerConnectionController: protected: c'tors and d'tors
//
//==========================================================================
//==========================================================================

vrpn_ServerConnectionController::vrpn_ServerConnectionController(
	vrpn_uint16 port,
	char * local_logfile_name,
	vrpn_int32 local_log_mode,
	vrpn_int32 tcp_inbuflen,
	vrpn_int32 tcp_outbuflen,
	vrpn_int32 udp_inbuflen,
	vrpn_int32 udp_outbuflen,
	vrpn_float64 dFreq,
	vrpn_int32 cSyncWindow ):
	// add multicast arguments later
	vrpn_BaseConnectionController(
		local_logfile_name,
		local_log_mode,
		NULL,
		vrpn_LOG_NONE,
		dFreq,
		cSyncWindow),
    // d_connection_list(NULL),
	num_live_connections(0),
	status(vrpn_CONNECTION_LISTEN),
	listen_udp_sock(INVALID_SOCKET),
	listen_port_no(port),
	// pass following on to {File,Net}Connection
    //	d_local_logfile_name(local_logfile_name),
    //	d_local_log_mode(local_log_mode),
	d_tcp_inbuflen(tcp_inbuflen),
	d_tcp_outbuflen(tcp_outbuflen),	
	d_udp_inbuflen(udp_inbuflen),
	d_udp_outbuflen(udp_outbuflen)
{

    // i think that this belongs in vrpn_BaseConnection
//      // Set all of the local IDs to -1, in case the other side
//      // sends a message of a type that it has not yet defined.
//      // (for example, arriving on the UDP line ahead of its TCP
//      // definition).
//      num_other_services = 0;
//      for (i = 0; i < vrpn_CONNECTION_MAX_SERVICES; i++) {
//          other_services[i].local_id = -1;
//          other_services[i].name = NULL;
//      }

//      num_other_types = 0;
//      for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
//          other_types[i].local_id = -1;
//          other_types[i].name = NULL;
//      }
    
    
    // create socket  to listen for incoming connections on 
    if ( (listen_udp_sock = vrpn_open_udp_socket(&listen_port_no))
         == INVALID_SOCKET) {
        status = vrpn_CONNECTION_BROKEN;
    } else {
        status = vrpn_CONNECTION_LISTEN;
        printf("vrpn: Listening for requests on port %d\n",listen_port_no);
    }
    
    
    /*============
        Multicast not being implemented in first release

    // try and create multicast sender. if it fails, we are not
    // multicast capable
    mcast_sender = new UnreliableMulticastSender(XXX);
    if( mcast_sender->created_correctly() ){
        vrpn_Mcast_Capable = vrpn_true;
    }
    else {
        vrpn_Mcast_Capable = vrpn_false;
    }

    // get mcast group info to pass to 
    // new connections
    if( vrpn_Mcast_Capable ){
        d_mcast_info = new char[sizeof(McastGroupDescrp)];
        mcast_sender->get_mcast_description(d_mcast_info);
    }
    =========*/

    // create logging object
    if( get_local_logmode() != vrpn_LOG_NONE ){
        char temp_log_ptr[150];
        get_local_logfile_name(temp_log_ptr);
        d_logger_ptr = new vrpn_FileLogger(temp_log_ptr,get_local_logmode());
    }

    
    //-----------------------------------------------------------------
    // initializations done by both clock client and server 
    //

    // register this clock by name, the query by name, and the reply
    // by name
    clockServer_id = register_service("clockServer");
    
//      queryMsg_id = vrpn_CLOCK_QUERY;
//      replyMsg_id = vrpn_CLOCK_REPLY;
    queryMsg_id = register_message_type("clock query");
    replyMsg_id = register_message_type("clock reply");
    if ( (clockServer_id == -1) || (queryMsg_id == -1) || (replyMsg_id == -1) ) {
        cerr << "vrpn_ServerConnectionController: Can't register IDs for synch clocks" 
             << endl;
        return;
    }

    
    //------------------------------------
    // server specific synch clock inits
    
    // Register the callback handler for clock server queries 
    // (along with "this" as user data)
    // It will take messages from any service (no service arg specified)
    if (register_handler(vrpn_CLOCK_QUERY, clockQueryHandler, this)) {
        cerr << "vrpn_ServerConnectionController: can't register handler\n"
             << endl;
        return;
    }

}


// destructor
vrpn_ServerConnectionController::~vrpn_ServerConnectionController()
{
    // NetConnection will now remove themselves from the list by
    // calling dropped_a_connection() in their destructor

//  	// Remove all connections from the "known connections" list.
//  	vrpn_CONNECTION_LIST *list_ptr = d_connection_list;
//  	vrpn_CONNECTION_LIST *list_ptrdel;
//  	while (list_ptr != NULL) {
//  		list_ptrdel = list_ptr;
//  		list_ptr = list_ptr->next;
//  		delete list_ptrdel->c;
//  		delete list_ptrdel;
//  	}

	// remove logging object
	if( d_logger_ptr ) delete d_logger_ptr;

	// remove mulitcast sender. there won't be one though.
	if( d_mcast_sender ) delete d_mcast_sender;

}

//==========================================================================
//==========================================================================
//
// vrpn_ServerConnectionController: public: connection setup

//
//==========================================================================
//==========================================================================

//---------------------------------------------------------------------------
//  This routine checks for a request for attention from a remote machine.
//  The requests come as datagrams to the well-known port number on this
// machine.
//  The datagram includes the machine name and port number on the remote
// machine to which this one should establish a connection.
//  This routine establishes the TCP connection and then sends a magic
// cookie describing this as a VRPN connection and telling which port to
// use for incoming UDP packets of data associated  with the current TCP
// session.  It reads the associated information from the other side and
// sets up the local UDP sender.
//  It then sends descriptions for all of the known packet types.
void vrpn_ServerConnectionController::listen_for_incoming_connections(
    const struct timeval * pTimeout )
{
    vrpn_int32   request;
    char msg[200];   // Message received on the request channel
    timeval timeout;
    if (pTimeout) {
        timeout = *pTimeout;
    } else {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    }
    
    // Do a zero-time select() to see if there is an incoming packet on
    // the UDP socket.
    
    fd_set f;
    FD_ZERO (&f);
    FD_SET ( listen_udp_sock, &f);
    request = select(32, &f, NULL, NULL, &timeout);
    if (request == -1 ) {    // Error in the select()
        perror("vrpn: vrpn_ServerConnectionController: select failed");
        status = vrpn_CONNECTION_BROKEN;
        return;
    } 
    else if (request != 0) { // Some data to read!  Go get it.
        struct sockaddr from;
        int fromlen = sizeof(from);
        if (recvfrom(listen_udp_sock, msg, sizeof(msg), 0,
                     &from, GSN_CAST &fromlen) == -1) {
            fprintf(stderr,
                    "vrpn: Error on recvfrom: Bad connection attempt\n");
            return;
        }
        
        // create new NetConnection
        // logging options will be set by other side.
        vrpn_BaseConnection* new_connection = new vrpn_NetConnection(this);

        // Check to see that the connection is connected
        new_connection->connect_to_client(msg);
        if ( !new_connection->connected() ) {
            fprintf(stderr,"vrpn_ServerConnectionController: "
                    "Could not connect to client\n");
            delete new_connection;
            return;
        }

        // Connection is added to the list when the connection calls
        // got_a_connection

//  	// Yank the local callbacks for new connection, and for
//  	// first connection if appropriate. Do not pack these
//  	// messages.
//  	struct timeval now;
//  	gettimeofday(&now, NULL);
//  	if (d_connection_list == NULL) {
//  		do_callbacks_for(register_message_type(vrpn_got_first_connection),
//                           register_service(vrpn_CONTROL), now, 0, NULL);

//  	}
//  	do_callbacks_for(register_message_type(vrpn_got_connection),
//                       register_service(vrpn_CONTROL), now, 0, NULL);

//          // add it to the connection list
//          vrpn_CONNECTION_LIST *curr;
        
//          if ( (curr = new(vrpn_CONNECTION_LIST)) == NULL) {
//              fprintf(stderr,
//                      "vrpn_ServerConnectionController:  Out of memory.\n");
//  	    delete new_connection;
//              return;
//          }
//          // XXX - don't think there is currently convention for knowing
//          // client name. sain 10/99  [Pull it from the msg - RMT]
//          strncpy(curr->name, "anonymous connection", sizeof(curr->name));
//          curr->c = new_connection;
//          curr->next = d_connection_list;
    }
    return;
}

// get mcast group info from mcast sender for use
// during connection setup
char* vrpn_ServerConnectionController::get_mcast_info()
{
    return d_mcast_info;
}


//==========================================================================
//==========================================================================
//
// vrpn_ServerConnectionController: public: sending and receiving
//
//==========================================================================
//==========================================================================

// rough draft of mainloop function 
// XXX - how should the timeval value
// be divided among all the different calls
vrpn_int32 vrpn_ServerConnectionController::mainloop(
    const timeval * timeout)
{
    vrpn_int32 retval = 0;
    vrpn_int32 retval_sum;
    vrpn_ConnectionItr itr(vrpn_ConnectionManager::instance());

    switch( status ) {
    case vrpn_CONNECTION_LISTEN:
        listen_for_incoming_connections(timeout);
        break;
    case vrpn_CONNECTION_CONNECTED:
        // go through list of connections, then check for any
        // new incoming connections
        itr.First();
        while( +itr ){
            retval = itr()->mainloop(timeout); 
            if( retval == -1 ){
                return -1;
            } else {
                retval_sum += retval;
            }
            itr++;
        } 
        listen_for_incoming_connections(timeout);
        break;
    default:
        break;
    }
    return retval_sum;
}


// * pack a message that will be sent the next time mainloop() is called
// * turn off the RELIABLE flag if you want low-latency (UDP) send
// * was: pack_message
vrpn_int32 vrpn_ServerConnectionController::queue_outgoing_message(
    vrpn_uint32 len, 
    timeval time,
    vrpn_int32 type,
    vrpn_int32 service,
    const char * buffer,
    vrpn_uint32 class_of_service )
{
    vrpn_int32 ret;
    
    // Make sure type is either a system type (-) or a legal user type
    if ( type >= num_my_types ) {
        printf("vrpn_ServerConnectionController::queue_outgoing_message: bad type (%ld)\n",
               type);
        return -1;
    }
    
    // If this is not a system message, make sure the service is legal.
    if (type >= 0) {
        if ( (service < 0) || (service >= num_my_services) ) {
            printf("vrpn_ServerConnectionController::queue_outgoing_message: bad service (%ld)\n",
                   service);
            return -1;
        }
    }
    
    // when implemented send message out multicast channel
    // if appropriate
    
    // See if there are any local handlers for this message type from
    // this sender.  If so, yank the callbacks.
    if (do_callbacks_for(type, service, time, len, buffer)) {
        return -1;
    }
    
    // iterate over list of Connections and pass message to 
    // them
    vrpn_ConnectionItr itr(vrpn_ConnectionManager::instance());
    itr.First();
    while( +itr ) {
        ret = itr()->queue_outgoing_message(
            len,time,
            type,service,
            buffer,class_of_service,
            vrpn_false);
        if( ret == -1 ) {
            return -1; 
        }
        itr++;
    }
    return 0;
}

// * send pending report (that have been packed), and clear the buffer
// * this function was protected, now is public, so we can use
//   it to send out intermediate results without calling mainloop
vrpn_int32 vrpn_ServerConnectionController::send_pending_reports()
{
  vrpn_int32 retval;
  // go through list of connections, then check for any
  // new incoming connections
  vrpn_ConnectionItr itr(vrpn_ConnectionManager::instance());
  itr.First();
  while( +itr ) {
	retval = itr()->send_pending_reports();
	if( retval == -1 ){
	  fprintf(stderr,"vrpn_ServerConnectionController::send_pending_reports: error\n");
	  return retval;
	}
	itr++;
  }
  return retval;
}


//==========================================================================
//==========================================================================
//
// vrpn_ServerConnectionController: public: status
//
//==========================================================================
//==========================================================================


// are there any connections?
vrpn_int32 vrpn_ServerConnectionController::at_least_one_open_connection() const
{
    return vrpn_ConnectionManager::instance().numConnections() > 0 ;
}
    
// overall, all connections are doing okay
vrpn_int32 vrpn_ServerConnectionController::all_connections_doing_okay() const
{
    // iterate through list of connections and call doing_okay for each one.
    vrpn_int32 retval = 1;
    vrpn_ConnectionItr itr(vrpn_ConnectionManager::instance());
    itr.First();
    while( +itr ){
        retval &= itr()->doing_okay();
        itr++;
    }

    return retval;
}

// number of connections
vrpn_int32 vrpn_ServerConnectionController::num_connections() const
{
    return vrpn_ConnectionManager::instance().numConnections();
}

void vrpn_ServerConnectionController::got_a_connection(void* connection)
{

    vrpn_BaseConnection *new_connection = (vrpn_BaseConnection *) connection;

    // add connection to list - XXX currently it is an anonymous connection
    vrpn_ConnectionManager::instance().addConnection(new_connection, NULL);

    // Yank the local callbacks for new connection, and for
	// first connection if appropriate. Do not pack these
	// messages.
	struct timeval now;
	gettimeofday(&now, NULL);
	if ( vrpn_ConnectionManager::instance().numConnections() == 1 ) {
		do_callbacks_for(register_message_type(vrpn_got_first_connection),
                         register_service(vrpn_CONTROL), now, 0, NULL);

	}
	do_callbacks_for(register_message_type(vrpn_got_connection),
                     register_service(vrpn_CONTROL), now, 0, NULL);

}

void vrpn_ServerConnectionController::dropped_a_connection(void* connection)
{

   vrpn_BaseConnection *drop_connection = (vrpn_BaseConnection *) connection;

    // remove connection from list - XXX currently it is an anonymous connection
    vrpn_ConnectionManager::instance().deleteConnection(drop_connection);

    // Yank the local callbacks for new connection, and for
	// first connection if appropriate. Do not pack these
	// messages.
	struct timeval now;
	gettimeofday(&now, NULL);

	do_callbacks_for(register_message_type(vrpn_dropped_connection),
                     register_service(vrpn_CONTROL), now, 0, NULL);

	if ( vrpn_ConnectionManager::instance().isEmpty() ) {
		do_callbacks_for(register_message_type(vrpn_dropped_last_connection),
                         register_service(vrpn_CONTROL), now, 0, NULL);

	}


}
  

//==========================================================================
//==========================================================================
//
// vrpn_ServerConnectionController: public: logging functions
//
//==========================================================================
//==========================================================================

vrpn_int32 vrpn_ServerConnectionController::register_log_filter (
    vrpn_LOGFILTER filter, 
    void * userdata)
{
    vrpn_int32 retval;
    vrpn_ConnectionItr itr(vrpn_ConnectionManager::instance());

    // iterate through connections and call for each connection
    itr.First();
    while( +itr ){
        retval = itr()->register_log_filter (filter,userdata); 
        if( retval == -1 ){
            return -1;
        }
        itr++;
    } 
    return 0;
}


vrpn_int32 vrpn_ServerConnectionController::register_server_side_log_filter (
    vrpn_LOGFILTER filter, 
    void * userdata)
{
    return d_logger_ptr->register_log_filter (filter,userdata);
}


// ServerConnectionController doesn't have remote logfiles or logmodes

vrpn_int32 vrpn_ServerConnectionController::get_remote_logmode()
{
    return -1;
}

void vrpn_ServerConnectionController::get_remote_logfile_name(
    char * logname_copy)
{
    return;
}


//==========================================================================
//==========================================================================
//
// vrpn_ServerConnectionController: clock server functions
//
//==========================================================================
//==========================================================================

// packs time of message sent and all data sent to it back into buffer
vrpn_int32 vrpn_ServerConnectionController::encode_to(
    char *buf, 
    const struct timeval& tvSRep,
    const struct timeval& tvCReq, 
    vrpn_int32 cChars, const char *pch ) 
{
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


// to prevent the user from getting back incorrect info, clients are
// (nearly) uniquely identified by a usec timestamp.
vrpn_int32 vrpn_ServerConnectionController::clockQueryHandler(
    void *userdata, 
    vrpn_HANDLERPARAM p) 
{
    vrpn_ServerConnectionController *me 
        = (vrpn_ServerConnectionController *) userdata;
    static struct timeval now;
    char rgch[50];
    
    // send back time client sent request and the buffer the client sent
    //  cerr << ".";
    //  printTime("client req ",p.msg_time);
    
    // check clock version
    if ((p.payload_len==0) 
        || (ntohl(*(vrpn_int32 *)p.buffer)!=CLOCK_VERSION)) {

        cerr << "vrpn_Clock_Server: current version is 0x"
             << hex << CLOCK_VERSION 
             << ", clock query msg uses version 0x" 
             << ntohl(*(vrpn_int32 *)p.buffer) 
             << "." << dec << endl;
        return -1;
    }
    
    gettimeofday(&now,NULL);
    //  printTime("server rep ", now);
    vrpn_int32 cLen = me->encode_to( rgch, now, p.msg_time,
                                     p.payload_len, p.buffer );
    // change to queue_outgoing_message()  ??? - SS
    me->queue_outgoing_message(
        cLen, now,
        me->replyMsg_id, me->clockServer_id,
        rgch, vrpn_CONNECTION_RELIABLE);
    return 0;
}


//====================================================================================
//
// vrpn_ConnectionManager
//
//====================================================================================

vrpn_ConnectionManager::~vrpn_ConnectionManager (void) 
{

  //fprintf(stderr, "In ~vrpn_ConnectionManager:  tearing down the list.\n");

  // Call the destructor of every known connection.
  // That destructor will call vrpn_ConnectionManager::deleteConnection()
  // to remove itself from d_kcList.
//    while (d_kcList) {
//      delete d_kcList->connection;
//    }
  while (d_anonList) {
    delete d_anonList->connection;
  }

}

// static
vrpn_ConnectionManager & vrpn_ConnectionManager::instance (void) 
{
  static vrpn_ConnectionManager manager;
  return manager;
}

void vrpn_ConnectionManager::addConnection (
    vrpn_BaseConnection * c,
    const char * name) 
{
  knownConnection * p;

  p = new knownConnection;
  p->connection = c;

//    if (name) {
//      strncpy(p->name, name, 1000);
//      p->next = d_kcList;
//      d_kcList = p;
//    } else {
    p->name[0] = 0;
    p->next = d_anonList;
    d_anonList = p;
    //  }

  d_numConnections++;
}

void vrpn_ConnectionManager::deleteConnection (vrpn_BaseConnection * c) 
{
    //  deleteConnection(c, &d_kcList);
  deleteConnection(c, &d_anonList);
  d_numConnections--;
}

void vrpn_ConnectionManager::deleteConnection (
    vrpn_BaseConnection * c,
    knownConnection ** snitch) 
{
  knownConnection * victim = *snitch;

  while (victim && (victim->connection != c)) {
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

//  vrpn_BaseConnection * vrpn_ConnectionManager::getByName (const char * name) 
//  {
//    knownConnection * p;
//    for (p = d_kcList; p && strcmp(p->name, name); p = p->next) {
//      // do nothing
//    }
//    if (!p) {
//      return NULL;
//    }
//    return p->connection;
//  }

vrpn_ConnectionManager::vrpn_ConnectionManager (void) :
    //    d_kcList (NULL),
    d_anonList (NULL),
    d_numConnections(0)
{

}

vrpn_int32 vrpn_ConnectionManager::numConnections()
{
    return d_numConnections;
}

