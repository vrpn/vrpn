
//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: protected: c'tors and init
//
//**************************************************************************
//**************************************************************************

void vrpn_ServerConnectionController::ServerConnectionController(
    char * cname, 
    vrpn_uint16 port,
    char * local_logfile_name,
    vrpn_int32 local_log_mode,
    vrpn_int32 tcp_inbuflen,
    vrpn_int32 tcp_outbuflen,
    vrpn_int32 udp_inbuflen,
    vrpn_int32 udp_outbuflen,
    vrpn_float64 dFreq,
    vrpn_in32 cSyncWindow
    // add multicast arguments later
    )
    : BaseConnectionController( local_logfile_name,
                                local_log_mode,
                                NULL,
                                vrpn_LOG_NONE,
                                dFreq,
                                cSyncWindow),
    num_live_connections(0),
    status(NOT_CONNECTED),
    listen_udp_sock(INVALID_SOCKET),
    listen_port_no(port),
    // pass following on to Connection
    d_local_logfile_name(local_logfile_name),
    d_local_log_mode(local_log_mode),
    d_tcp_inbuflen(tcp_inbuflen),
    d_tcp_outbuflen(tcp_outbuflen),     
    d_udp_inbuflen(udp_inbuflen),
    d_udp_outbuflen(udp_outbuflen)
{
    vrpn_int32 i;

    // Set all of the local IDs to -1, in case the other side
    // sends a message of a type that it has not yet defined.
    // (for example, arriving on the UDP line ahead of its TCP
    // definition).
    num_other_services = 0;
    for (i = 0; i < vrpn_CONNECTION_MAX_SERVICES; i++) {
        other_services[i].local_id = -1;
        other_services[i].name = NULL;
    }

    num_other_types = 0;
    for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
        other_types[i].local_id = -1;
        other_types[i].name = NULL;
    }
    
    
    // create socket  to listen for incoming connections on 
    if ( (listen_udp_sock = open_udp_socket(&listen_port_no))
         == INVALID_SOCKET) {
        status = BROKEN;
    } else {
        status = LISTEN;
        printf("vrpn: Listening for requests on port %d\n",listen_port_no);
    }
    
    
    /************
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
    **********/

    // create logging object
    if( d_local_log_mode != vrpn_LOG_NONE ){
        char temp_log_ptr[150];
        get_local_logfile_name(temp_log_ptr);
        d_logger_ptr = new vrpn_FileLogger(temp_log_ptr,get_local_logmode());
    }
    
    // Register the callback handler for clock server queries 
    // (along with "this" as user data)
    // It will take messages from any service (no service arg specified)
    if (register_handler(vrpn_CLOCK_QUERY, clockQueryHandler, this)) {
        cerr << "vrpn_ServerConnectionController: can't register handler\n"
             << endl;
        return;
    }
}

//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: public: connection setup

//
//**************************************************************************
//**************************************************************************

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
        status = BROKEN;
        return;
    } 
    else if (request != 0) { // Some data to read!  Go get it.
        struct sockaddr from;
        vrpn_int32 fromlen = sizeof(from);
        if (recvfrom(listen_udp_sock, msg, sizeof(msg), 0,
                     &from, GSN_CAST &fromlen) == -1) {
            fprintf(stderr,
                    "vrpn: Error on recvfrom: Bad connection attempt\n");
            return;
        }
        
        // create new NetConnection
        BaseConnection* new_connection = new NetConnection(/* xxx */);
        
        // add it to the connection list
        vrpn_CONNECTION_LIST *curr;
        
        if ( (curr = new(vrpn_CONNECTION_LIST)) == NULL) {
            fprintf(stderr,
                    "vrpn_ServerConnectionController:  Out of memory.\n");
            return;
        }
        strncpy(curr->name, station_name, sizeof(curr->name));
        curr->c = this;
        curr->next = d_connection_list;
        
        // NetConnection will handle rest
        new_connection->connect_to_client(msg);
    }
    return;
}

// get mcast group info from mcast sender for use
// during connection setup
char* ServerConnectionController::get_mcast_info()
{
    return d_mcast_info;
}


//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: public: sending and receiving
//
//**************************************************************************
//**************************************************************************

// rough draft of mainloop function
vrpn_int32 vrpn_ServerConnectionController::mainloop(
    const timeval * timeout)
{
    switch( status ) {
    case NOTCONNECTED:
        listen_for_incoming_connections();
        break;
    case CONNECTED:
        // go through list of connections, then check for any
        // new incoming connections
        vrpn_CONNECTION_LIST *itr;
        itr = d_connection_list;
        do {
            itr->c->mainloop(timeout); // XXX - should timeval arg b divided evenly among connections?
            itr = itr->next;
        } while( itr->next != NULL );
        listen_for_incoming_connections();
        break;
    }
}


// * pack a message that will be sent the next time mainloop() is called
// * turn off the RELIABLE flag if you want low-latency (UDP) send
// * was: pack_message
vrpn_int32 vrpn_ServerConnectionController::handle_outgoing_message(
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
        printf("vrpn_ServerConnectionController::handle_outgoing_messages: bad type (%ld)\n",
               type);
        return -1;
    }
    
    // If this is not a system message, make sure the service is legal.
    if (type >= 0) {
        if ( (service < 0) || (service >= num_my_services) ) {
            printf("vrpn_ServerConnectionController::handle_outgoing_messages: bad service (%ld)\n",
                   service);
            return -1;
        }
    }
    
    // when implemented send message out multicast channel
    // if appropriate
    
    // See if there are any local handlers for this message type from
    // this sender.  If so, yank the callbacks.
    if (do_callbacks_for(type, sender, time, len, buffer)) {
        return -1;
    }
    
    // iterate over list of Connections and pass message to 
    // them
    while( conn_list_ptr != NULL ){
        ret = conn_list_ptr->handle_outgoing_messages(
            len,time,
            type,service,
            buffer,vrpn_false);
        if( ret = -1 ) return -1; 
    }
    return 0;
}

//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: public: logging get functions
//
//**************************************************************************
//**************************************************************************

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


//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: clock server functions
//
//**************************************************************************
//**************************************************************************

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
    vrpn_ServerConnectionController *me = (vrpn_Clock_Server *) userdata;
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
    // change to handle_outgoing_message()  ??? - SS
    me->connection->pack_message(cLen, now,
                                 vrpn_CLOCK_REPLY, myID,
                                 rgch, vrpn_CONNECTION_RELIABLE);
    return 0;
}
