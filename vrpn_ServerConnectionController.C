
//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: protected: c'tors and init
//
//**************************************************************************
//**************************************************************************

void vrpn_ServerConnectionController::init(void)
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

    // create list for connections

    // try and create multicast sender. if it fails, we are not
    // multicast capable
    mcast_sender = new UnreliableMulticastSender(/* xxx */);
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

    // create logging object
    char temp_logname_buf[150]; // should be long enough for any filename
    get_local_logfile_name(temp_logname_buf);
    logger = new vrpn_FileLogger(temp_logname_buf,get_local_logmode());

        
    // Register the callback handler for clock server queries 
    // (along with "this" as user data)
    // It will take messages from any service (no service arg specified)
    if (register_handler(vrpn_CLOCK_QUERY, clockQueryHandler, this)) {
        cerr << "vrpn_ServerConnectionController: can't register handler\n" << endl;
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
void vrpn_ServerConnectionController::
listen_for_incoming_connections(const struct timeval * pTimeout){
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
       BaseConnection* new_connection = new NetConnection;
       
       // add it to the connection list
       
       // NetConnection will handle rest
       new_connection->connect_to_client(msg);

   }
   return;
}

// get mcast group info from mcast sender for use
// during connection setup
char* ServerConnectionController::get_mcast_info(){
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
vrpn_int32 mainloop( const timeval * timeout){
    
    switch( status ){
    case NOTCONNECTED:
        listen_for_incoming_connections();
        break;
    case CONNECTED:
        ListItr itr = ConnectionList; // create list iterator
        for( ; itr != NULL; itr++){
            itr->handle_incoming_messages(/* xxx */);
            itr->handle_outgoing_messages(/* xxx */);
        }
        listen_for_incoming_connections();
        break;
    }
}



//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: public: logging get functions
//
//**************************************************************************
//**************************************************************************

// ServerConnectionController doesn't have remote logfiles or logmodes

vrpn_int32 vrpn_ServerConnectionController::get_remote_logmode(void)
{
    return -1;
}

void vrpn_ServerConnectionController::get_remote_logfile_name(char * logname_copy)
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
vrpn_int32 vrpn_ServerConnectionController::encode_to(char *buf, 
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
vrpn_int32 vrpn_ServerConnectionController::clockQueryHandler(void *userdata, 
                                                              vrpn_HANDLERPARAM p) 
{
    vrpn_ServerConnectionController *me = (vrpn_Clock_Server *) userdata;
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
    vrpn_int32 cLen = me->encode_to( rgch, now, p.msg_time, p.payload_len, p.buffer );
    // change to handle_outgoing_message()  ??? - SS
    me->connection->pack_message(cLen, now,
                                 vrpn_CLOCK_REPLY, myID,
                                 rgch, vrpn_CONNECTION_RELIABLE);
  return 0;
}


