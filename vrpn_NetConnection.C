#include "vrpn_NetConnection.h"

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: public: c'tors, d'tors, init

//
//==========================================================================
//==========================================================================


vrpn_NetConnection::vrpn_NetConnection(
    vrpn_BaseConnectionController::SpecialAccessToken * spat,
    const char * local_logfile,
    vrpn_int32 local_logmode,
    const char * remote_logfile,
    vrpn_int32 remote_logmode,
    vrpn_int32 tcp_inbuflen,
    vrpn_int32 tcp_outbuflen,
    //  vrpn_int32 udp_inbuflen,
    vrpn_int32 udp_outbuflen)
    // add multicast arguments later
    : vrpn_BaseConnection(
        spat,
        local_logfile,
        local_logmode,
        remote_logfile,
        remote_logmode),
    status(vrpn_CONNECTION_LISTEN),
    d_tcp_outbuflen(tcp_outbuflen),
    d_tcp_inbuflen(tcp_inbuflen),
    d_udp_outbuflen(udp_outbuflen),
    //d_udp_inbuflen(udp_inbuflen),
    tcp_sock(INVALID_SOCKET),
    udp_outbound(INVALID_SOCKET),
    udp_inbound(INVALID_SOCKET)
    // add logging stuff later
{
    // make sure multicast code is not used
    set_mcast_capable(vrpn_false);
    set_peer_mcast_capable(vrpn_false);

    if (!d_tcp_outbuflen || !d_tcp_inbuflen || !d_udp_outbuflen) {
        status = vrpn_CONNECTION_BROKEN;
        fprintf(stderr, "vrpn_NetConnection couldn't allocate buffers.\n");
        return;
    }

    // create message buffers
    d_tcp_inbuf = new char[d_tcp_inbuflen];
    d_tcp_outbuf = new char[d_tcp_outbuflen];
    //  d_udp_inbuf = new char[d_udp_buflen];
    d_udp_outbuf = new char[d_udp_outbuflen];

    if (!d_tcp_outbuf || !d_udp_outbuf || !d_tcp_inbuf || !d_udp_inbuf) {
        status = vrpn_CONNECTION_BROKEN;
        fprintf(stderr, "vrpn_NetConnection couldn't allocate buffers.\n");
        return;
    }

    // Set up the handlers for the system messages we use that are
    // not set up in the base class
    system_messages[-vrpn_CONNECTION_UDP_DESCRIPTION] =
        handle_incoming_udp_message;
  
   // initialize loggin
   if (local_logfile) {
       vrpn_int32 retval = open_log();
       if (retval == -1) {
           fprintf(stderr, "vrpn_NetConnection::vrpn_NetConnection:  "
                   "Couldn't open log file.\n");
           status = vrpn_CONNECTION_BROKEN;
           return;
       }
   }

} // end NetConnection c'tor





vrpn_NetConnection::~vrpn_NetConnection(void)
{
    vrpn_int32 i;
    for (i=0;i< num_registered_remote_types;i++) {
        delete registered_remote_types[i].name;
    }   
    for (i=0;i< num_registered_remote_services;i++) {
        delete registered_remote_services[i].name;
    }

    // delete message buffers
    delete d_tcp_inbuf;
    delete d_tcp_outbuf;
    delete d_udp_inbuf;
    delete d_udp_outbuf;
    
    d_controller_token->dropped_a_connection(this);
}

// }}} end vrpn_NetConnection: public: c'tors, d'tors, init

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: public: send and receive functions

//
//==========================================================================
//==========================================================================

// rough draft of mainloop
vrpn_int32 vrpn_NetConnection::mainloop (const struct timeval * timeout){

    // Send any queued messages
    if (send_pending_reports() == -1) {
        return -1;
    }

    // Handle any incoming messages
    return handle_incoming_messages( timeout );
}


// This function gets called by the ConnectionController
// replaces pack_message in the interface, but calls it internally
vrpn_int32 vrpn_NetConnection::queue_outgoing_message(vrpn_uint32 len, struct timeval time,
        vrpn_int32 type, vrpn_int32 service, const char * buffer,
        vrpn_uint32 class_of_service, vrpn_bool sent_mcast)
{

    // if mcast capable and message sent_mcast, log message and return
    // mulitcast functions not implemented

    return pack_message(len,time,type,service,buffer,class_of_service);
}


// gets called by ConnectionController. handles any incoming messages
// on any of the channels, as well as logging and invoking callbacks.
// This function uses some of the functionality found in
// vrpn_NetConnection::handle_incoming_messages()
vrpn_int32 vrpn_NetConnection::handle_incoming_messages( const struct timeval* pTimeout ){

    vrpn_int32  tcp_messages_read;
    vrpn_int32  udp_messages_read;
    vrpn_int32  mcast_messages_read = 0; // mcast not implemented

    timeval timeout;
    if (pTimeout) {
        timeout = *pTimeout;
    } else {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    }

#ifdef  VERBOSE2
    printf("vrpn_NetConnection::handle_incoming_messages called 
        (status %d)\n",status);
#endif
 

    // struct timeval perSocketTimeout;
    // const int numSockets = 2;
    // divide timeout over all selects()
    //  if (timeout) {
    //    perSocketTimeout.tv_sec = timeout->tv_sec / numSockets;
    //    perSocketTimeout.tv_usec = timeout->tv_usec / numSockets
    //      + (timeout->tv_sec % numSockets) *
    //      (1000000L / numSockets);
    //  } else {
    //    perSocketTimeout.tv_sec = 0L;
    //    perSocketTimeout.tv_usec = 0L;
    //  }

    // BETTER IDEA: do one select rather than multiple -- otherwise you are
    // potentially holding up one or the other.  This allows clients which
    // should not do anything until they get messages to request infinite
    // waits (servers can't usually do infinite waits this because they need
    // to service other devices to generate info which they then send to
    // clients) .  weberh 3/20/99
  

    switch (status) {
        /*
    case vrpn_CONNECTION_LISTEN:
        // check_connection(pTimeout); now done in Controller
        break;
        */
    case vrpn_CONNECTION_CONNECTED: 
   

        // check for pending incoming tcp, udp, or multicast reports
        // we do this so that we can trigger out of the timeout
        // on either type of message without waiting on the other
    
        fd_set  readfds, exceptfds;
        FD_ZERO(&readfds);              /* Clear the descriptor sets */
        FD_ZERO(&exceptfds);

        // Read incoming messages from the UDP, TCP. and Multicast channels

        if (udp_inbound != -1) {
            FD_SET(udp_inbound, &readfds); /* Check for read */
            FD_SET(udp_inbound, &exceptfds);/* Check for exceptions */
        }
        // =========
        // mcast_capable should always return false, because
        // multicast is not yet fully implemented
        if( mcast_capable() ) { // if there is a multicast receiver
            FD_SET(mcast_recvr->get_mcast_sock(), &readfds);
            FD_SET(mcast_recvr->get_mcast_sock(), &exceptfds);
        }
        FD_SET(tcp_sock, &readfds);
        FD_SET(tcp_sock, &exceptfds);

        // Select to see if ready to hear from other side, or exception
        if (select(32, &readfds, NULL ,&exceptfds, (timeval *)&timeout) == -1) {
            if (errno == EINTR) { /* Ignore interrupt */
                break;
            } 
            else {
                perror("vrpn: vrpn_NetConnection::handle_incoming_messages: select failed.");
                drop_connection();
                if (status != vrpn_CONNECTION_LISTEN) {
                    status = vrpn_CONNECTION_DROPPED;
                    return -1;
                }
            }
        }


        // See if exceptional condition on any socket
        if (FD_ISSET(tcp_sock, &exceptfds) ||
            ((udp_inbound!=-1) && FD_ISSET(udp_inbound, &exceptfds)) ||
            ( mcast_capable() && FD_ISSET(mcast_recvr->get_mcast_sock(), &exceptfds) ) ){
            fprintf(stderr, "vrpn_NetConnection::handle_incoming_messages: Exception on socket\n");
            return(-1);
        }

        // Read incoming messages from the UDP channel
        if ((udp_inbound != -1) && FD_ISSET(udp_inbound,&readfds)) {
            if ( (udp_messages_read = handle_udp_messages(udp_inbound,NULL)) == -1) {
                fprintf(stderr,"vrpn: UDP handling failed, dropping connection\n");
                drop_connection();
                if (status != vrpn_CONNECTION_LISTEN) {
                    status = vrpn_CONNECTION_DROPPED;
                    return -1;
                }
                break;
            }   

#ifdef VERBOSE3
            if(udp_messages_read != 0 )
                printf("udp message read = %d\n",udp_messages_read);
#endif
        }

        // Read incoming messages from the Multicast channel
        // =========
        // mcast_capable should always return false, because
        // multicast is not yet fully implemented
        if ( mcast_capable() ){
            if( FD_ISSET(mcast_recvr->get_mcast_sock(),&readfds)) {
                if ( (mcast_messages_read = handle_mcast_messages(/* XXX */)) == -1) {
                    fprintf(stderr,"vrpn: Mcast handling failed, dropping connection\n");
                    drop_connection();
                    if (status != vrpn_CONNECTION_LISTEN) {
                        status = vrpn_CONNECTION_DROPPED;
                        return -1;
                    }
                    break;
                }   
            }

#ifdef VERBOSE3
            if(mcast_messages_read != 0 )
                printf("mcast message read = %d\n",mcast_messages_read);
#endif
        }

        // Read incoming messages from the TCP channel
        if (FD_ISSET(tcp_sock,&readfds)) {
            if ((tcp_messages_read = 
                 handle_tcp_messages(tcp_sock,NULL)) == -1) {
                printf("vrpn: TCP handling failed, dropping connection\n");
                drop_connection();
                if (status != vrpn_CONNECTION_LISTEN) {
                    status = vrpn_CONNECTION_DROPPED;
                    return -1;
                }
                break;
            }
#ifdef VERBOSE3
            else {
                if(tcp_messages_read!=0)
                printf("tcp_message_read %d bytes\n",tcp_messages_read);
            }
#endif
        }

#ifdef  PRINT_READ_HISTOGRAM
#define      HISTSIZE 25
        
        static vrpn_uint32 count = 0;
        static vrpn_int32 tcp_histogram[HISTSIZE+1];
        static vrpn_int32 udp_histogram[HISTSIZE+1];
        static vrpn_int32 mcast_histogram[HISTSIZE+1];

        count++;

        if (tcp_messages_read > HISTSIZE) {tcp_histogram[HISTSIZE]++;}
        else {tcp_histogram[tcp_messages_read]++;};

        if (udp_messages_read > HISTSIZE) {udp_histogram[HISTSIZE]++;}
        else {udp_histogram[udp_messages_read]++;};

        if (mcast_messages_read > HISTSIZE) {mcast_histogram[HISTSIZE]++;}
        else {mcast_histogram[mcast_messages_read]++;};

        if (count == 3000L) {
            vrpn_int32 i;
            count = 0;
            
            printf("\nHisto (tcp): ");
            for (i = 0; i < HISTSIZE+1; i++) {
                printf("%d ",tcp_histogram[i]);
                tcp_histogram[i] = 0;
            }
            printf("\n");
            printf("      (udp): ");
            for (i = 0; i < HISTSIZE+1; i++) {
                printf("%d ",udp_histogram[i]);
                udp_histogram[i] = 0;
            }
            printf("\n");
            printf("      (mcast): ");
            for (i = 0; i < HISTSIZE+1; i++) {
                printf("%d ",mcast_histogram[i]);
                mcast_histogram[i] = 0;
            }
            printf("\n");
        }   
    

#endif
        break;
    case vrpn_CONNECTION_BROKEN:
        fprintf(stderr, "vrpn: Socket failure.  Giving up!\n");
        status = vrpn_CONNECTION_DROPPED;
        return -1;
    case vrpn_CONNECTION_DROPPED:
        break;
       
    }
#ifndef VERBOSE3
    // Keep the compiler happy
    tcp_messages_read = tcp_messages_read;
    udp_messages_read = udp_messages_read;
    mcast_messages_read = mcast_messages_read;
#endif
    return 0;
}
    
  
// pack message into send buffer. the buffer contains
// many messages/reports
vrpn_int32 vrpn_NetConnection::pack_message(vrpn_uint32 len, 
                                            struct timeval time,
                                            vrpn_int32 type, 
                                            vrpn_int32 service, 
                                            const char * buffer,
                                            vrpn_uint32 class_of_service)
{
    vrpn_int32  ret;

    // Logging must come before filtering and should probably come before
    // any other failure-prone action (such as do_callbacks_for()).  Only
    // semantic checking should precede it.
    
    if (d_local_logmode & vrpn_LOG_OUTGOING) // FileLogger object exists
        if (d_logger_ptr->log_message(len, time, type, service, buffer))
            return -1;

    // If we're not connected, nowhere to send the message so just
    // toss it out.
    if (!connected()) { return 0; }

    // TCH 28 Oct 97
    // If the other side doesn't care about the message, toss it out.
#ifdef vrpn_FILTER_MESSAGES
    if ((type >= 0) && !my_types[type].cCares) {
#ifdef VERBOSE
        printf("  vrpn_NetConnection:  discarding a message of type %ld "
                       "because the receiver doesn't care.\n", type);
#endif // VERBOSE
        return 0;
    }
#endif  // vrpn_FILTER_MESSAGES

    // If we don't have a UDP outbound channel, send everything TCP
    if (udp_outbound == -1) {
        ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_outbuflen, d_tcp_num_out,
                   len, time, type, service, buffer);
        d_tcp_num_out += ret;
        //      return -(ret==0);

        // If the marshalling failed, try clearing the outgoing buffers
        // by sending the stuff in them to see if this makes enough
        // room.  If not, we'll have to give up.
        if (ret == 0) {
            if (send_pending_reports() != 0) { return -1; }
            ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_outbuflen,
                                        d_tcp_num_out, len, time, type, service, buffer);
            d_tcp_num_out += ret;
        }
        return (ret==0) ? -1 : 0;
    }

    // Determine the class of service and pass it off to the
    // appropriate service (TCP for reliable, UDP for everything else).
    if (class_of_service & vrpn_CONNECTION_RELIABLE) {
        ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_outbuflen, d_tcp_num_out,
                   len, time, type, service, buffer);
        d_tcp_num_out += ret;
        //      return -(ret==0);

        // If the marshalling failed, try clearing the outgoing buffers
        // by sending the stuff in them to see if this makes enough
        // room.  If not, we'll have to give up.
        if (ret == 0) {
        if (send_pending_reports() != 0) { return -1; }
        ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_outbuflen,
                d_tcp_num_out, len, time, type, service, buffer);
        d_tcp_num_out += ret;
        }
        return (ret==0) ? -1 : 0;
    } else {
        ret = vrpn_marshall_message(d_udp_outbuf, d_udp_outbuflen, d_udp_num_out,
                   len, time, type, service, buffer);
        d_udp_num_out += ret;
        //      return -(ret==0);

        // If the marshalling failed, try clearing the outgoing buffers
        // by sending the stuff in them to see if this makes enough
        // room.  If not, we'll have to give up.
        if (ret == 0) {
        if (send_pending_reports() != 0) { return -1; }
        ret = vrpn_marshall_message(d_udp_outbuf, d_udp_outbuflen,
                d_udp_num_out, len, time, type, service, buffer);
        d_udp_num_out += ret;
        }
        return (ret==0) ? -1 : 0;
    }
}


// sends out message buffers over the network
vrpn_int32 vrpn_NetConnection::send_pending_reports()
{
   vrpn_int32   ret, sent = 0;

   // Do nothing if not connected.
   if (!connected()) {
       return 0;
   }

   // Check for an exception on the socket.  If there is one, shut it
   // down and go back to listening.
   timeval timeout;
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   fd_set f;
   FD_ZERO (&f);
   FD_SET ( tcp_sock, &f);

   vrpn_int32 connection = select(32, NULL, NULL, &f, &timeout);
   if (connection != 0) {
       drop_connection();
       return -1;
   }

   // Send all of the messages that have built
   // up in the TCP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
#ifdef  VERBOSE
   if (d_tcp_num_out) printf("TCP Need to send %d bytes\n",d_tcp_num_out);
#endif
   while (sent < d_tcp_num_out) {
       ret = send(tcp_sock, &d_tcp_outbuf[sent],
                  d_tcp_num_out - sent, 0);
#ifdef  VERBOSE
       printf("TCP Sent %d bytes\n",ret);
#endif
       if (ret == -1) {
           printf("vrpn: TCP send failed\n");
           drop_connection();
           return -1;
       }
       sent += ret;
   }

   // Send all of the messages that have built
   // up in the UDP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
   if (d_udp_num_out > 0) {
       ret = send(udp_outbound, d_udp_outbuf, d_udp_num_out, 0);
#ifdef  VERBOSE
       printf("UDP Sent %d bytes\n",ret);
#endif 
       if (ret == -1) {
           printf("vrpn: UDP send failed\n");
           drop_connection();
           return -1;
       }
   }

   d_tcp_num_out = 0;   // Clear the buffer for the next time
   d_udp_num_out = 0;   // Clear the buffer for the next time
   return 0;
}

// }}} end vrpn_NetConnection: public: send and receive functions

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: protected: send and receive functions

//
//==========================================================================
//==========================================================================


// === N.B. === 
// it seems to me that the whole message is treated as a single report. i thought
// that it was possible for multiple reports to be in a single
// tcp message. - stefan

// Read all messages available on the given file descriptor (a TCP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.

vrpn_int32 vrpn_NetConnection::handle_tcp_messages (
    vrpn_int32 fd,
    const timeval * timeout)
{
    vrpn_int32  sel_ret;
    fd_set      readfds, exceptfds;
    timeval     localTimeout;
    vrpn_int32  num_messages_read = 0;

#ifdef  VERBOSE2
    printf("vrpn_NetConnection::handle_tcp_messages() called\n");
#endif
    
    if (timeout) {
        localTimeout.tv_sec = timeout->tv_sec;
        localTimeout.tv_usec = timeout->tv_usec;
    } else {
        localTimeout.tv_sec = 0L;
        localTimeout.tv_usec = 0L;
    }
    
    // Read incoming messages until there are no more characters to
    // read from the other side.  For eachselect message, determine what
    // type it is and then pass it off to the appropriate handler
    // routine.
    
    do {
        // Select to see if ready to hear from other side, or exception
        FD_ZERO(&readfds);              /* Clear the descriptor sets */
        FD_ZERO(&exceptfds);
        FD_SET(fd, &readfds);     /* Check for read */
        FD_SET(fd, &exceptfds);   /* Check for exceptions */
        sel_ret = select(32,&readfds,NULL,&exceptfds, &localTimeout);
        if (sel_ret == -1) {
            if (errno == EINTR) { /* Ignore interrupt */
                continue;
            } else {
                perror("vrpn: vrpn_NetConnection::handle_tcp_messages:"
                       " select failed");
                return(-1);
            }
        }
        
        // See if exceptional condition on socket
        if (FD_ISSET(fd, &exceptfds)) {
            fprintf(stderr,
                    "vrpn: vrpn_NetConnection::handle_tcp_messages:"
                    " Exception on socket\n");
            return(-1);
        }
        
        // If there is anything to read, get the next message
        if (FD_ISSET(fd, &readfds)) {
            vrpn_int32  header[5];
            struct timeval time;
            vrpn_int32  service, type;
            vrpn_int32  len, payload_len, ceil_len;
#ifdef  VERBOSE2
            printf("vrpn_NetConnection::handle_tcp_messages()"
                   " something to read\n");
#endif
            
            // Read and parse the header
            if (vrpn_noint_block_read(fd,(char*)header,sizeof(header)) !=
                sizeof(header)) {
                printf("vrpn_connection::handle_tcp_messages:  "
                       "Can't read header\n");
                return -1;
            }
            len = ntohl(header[0]);
            time.tv_sec = ntohl(header[1]);
            time.tv_usec = ntohl(header[2]);
            service = ntohl(header[3]);
            type = ntohl(header[4]);
#ifdef  VERBOSE2
            printf("  header: Len %d, Service %d, Type %d\n",
                   (int)len, (int)service, (int)type);
#endif
            
            // skip up to alignment
            vrpn_int32 header_len = sizeof(header);
            if (header_len%vrpn_ALIGN) {
                header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;
            }
            if (header_len > sizeof(header)) {
                // the difference can be no larger than this
                char rgch[vrpn_ALIGN];
                if (vrpn_noint_block_read (fd, (char*)rgch,
                                           header_len-sizeof(header))
                    != (vrpn_int32)(header_len-sizeof(header)))
                {
                    printf("vrpn_NetConnection::handle_tcp_messages:  "
                           "Can't read header + alignment\n");
                    return -1;
                }
            }
            
            // Figure out how long the message body is, and how long it
            // is including any padding to make sure that it is a
            // multiple of four bytes long.
            payload_len = len - header_len;
            ceil_len = payload_len;
            if (ceil_len%vrpn_ALIGN) {
                ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;
            }
            
            // Make sure the buffer is long enough to hold the whole
            // message body.
            if ((vrpn_int32)d_tcp_inbuflen < ceil_len) {
                if (d_tcp_inbuf) {
                    d_tcp_inbuf = (char*)realloc(d_tcp_inbuf,ceil_len);
                }
                else {
                    d_tcp_inbuf = (char*)malloc(ceil_len);
                }
                if (d_tcp_inbuf == NULL) {
                    fprintf(stderr,
                            "vrpn: vrpn_NetConnection::handle_tcp_messages:"
                            " Out of memory\n");
                    return -1;
                }
                d_tcp_inbuflen = ceil_len;
            }
            
            // Read the body of the message 
            if (vrpn_noint_block_read(fd,d_tcp_inbuf,ceil_len) != ceil_len) {
                perror("vrpn: vrpn_NetConnection::handle_tcp_messages:"
                       " Can't read body");
                return -1;
            }
            
            // Call the handler(s) for this message type
            // If one returns nonzero, return an error.
            if (type >= 0) {    // User handler, map to local id
                
                // adjust timestamp before sending to vrpn_FileLogger,
                // because that is no longer done in there
                timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);
                
                // log regardless of whether local id is set,
                // but only process if it has been (ie, log ALL
                // incoming data -- use a filter on the log
                // if you don't want some of it).
                if (d_local_logmode & vrpn_LOG_INCOMING) {
                    if (d_logger_ptr->log_message(
                        payload_len, tvTemp,
                        translate_remote_type_to_local(type),
                        translate_remote_service_to_local(service),
                        d_tcp_inbuf))
                    {
                        return -1;
                    }
                }
                
                if (translate_remote_type_to_local(type) >= 0) {
                    if (d_controller_token->do_callbacks_for(
                        translate_remote_type_to_local(type),
                        translate_remote_service_to_local(service),
                        time, payload_len, d_tcp_inbuf))
                    {
                        return -1;
                    }
                }
                
            } else {    // Call system handler if there is one
                
                // Special cases have been added to handle Clock
                // synchronization messages because their handlers are in
                // the ConnectionControllers and not BaseConnection like
                // the others [sain:11/99]
                if (system_messages[-type] != NULL
                    || type == vrpn_CONNECTION_CLOCK_QUERY ||
                    type == vrpn_CONNECTION_CLOCK_REPLY)
                {
                    // adjust timestamp before sending to vrpn_FileLogger,
                    // because that is no longer done in there
                    timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);
                    
                    if (d_local_logmode & vrpn_LOG_INCOMING) {
                        if (d_logger_ptr->log_message(
                            payload_len, tvTemp, 
                            translate_remote_type_to_local(type),
                            translate_remote_service_to_local(service),
                            d_tcp_inbuf))
                        {
                            return -1;
                        }
                    }
                
                    // Fill in the parameter to be passed to the routines
                    vrpn_HANDLERPARAM p;
                    p.type = type;
                    p.service = service;
                    p.msg_time = time;
                    p.payload_len = payload_len;
                    p.buffer = d_tcp_inbuf;
                    
                    // if it is a clocl synchronization message invoke the
                    // registered non-system handler, otherwise call the
                    // system handler
                    if( type == vrpn_CONNECTION_CLOCK_QUERY ||
                        type == vrpn_CONNECTION_CLOCK_REPLY )
                    {
                        if (d_controller_token->do_callbacks_for(
                            type,service,time, 
                            payload_len,d_tcp_inbuf))
                        {
                            return -1;
                        }
                    }
                    else {
                        if (system_messages[-type](this, p)) {
                            fprintf(stderr, 
                                    "vrpn: vrpn_NetConnection::handle_tcp_messages: Nonzero system handler return\n");
                            return -1;
                        }
                    }
                }
            }
            // Got one more message
            num_messages_read++;
        }
    } while (sel_ret);
    
    return num_messages_read;
}

// Read all messages available on the given file descriptor (a UDP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.
// XXX At some point, the common parts of this routine and the TCP read
// routine should be merged.  Using read() on the UDP socket fails, so
// we need the UDP version.  If we use the UDP version for the TCP code,
// it hangs when we the client drops its connection, so we need the
// TCP code as well.

vrpn_int32  vrpn_NetConnection::handle_udp_messages (vrpn_int32 fd,
                                             const struct timeval * timeout)
{   vrpn_int32  sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval localTimeout;
    vrpn_int32  num_messages_read = 0;
    vrpn_int32  inbuf_len;
    char    *inbuf_ptr;

#ifdef  VERBOSE2
    printf("vrpn_NetConnection::handle_udp_messages() called\n");
#endif

        if (timeout) {
      localTimeout.tv_sec = timeout->tv_sec;
      localTimeout.tv_usec = timeout->tv_usec;
        } else {
      localTimeout.tv_sec = 0L;
      localTimeout.tv_usec = 0L;
        }

    // Read incoming messages until there are no more packets to
    // read from the other side.  Each packet may have more than one
    // message in it.  For each message, determine what
    // type it is and then pass it off to the appropriate handler
    // routine.

    do {
      // Select to see if ready to hear from server, or exception
      FD_ZERO(&readfds);              /* Clear the descriptor sets */
      FD_ZERO(&exceptfds);
      FD_SET(fd, &readfds);     /* Check for read */
      FD_SET(fd, &exceptfds);   /* Check for exceptions */
      sel_ret = select(32, &readfds, NULL, &exceptfds, &localTimeout);
      if (sel_ret == -1) {
        if (errno == EINTR) { /* Ignore interrupt */
        continue;
        } else {
        perror("vrpn: vrpn_NetConnection::handle_udp_messages: select failed()");
        return(-1);
        }
      }

      // See if exceptional condition on socket
      if (FD_ISSET(fd, &exceptfds)) {
        fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Exception on socket\n");
        return(-1);
      }

      // If there is anything to read, get the next message
      if (FD_ISSET(fd, &readfds)) {
        vrpn_int32  header[5];
        struct timeval  time;
        vrpn_int32  service, type;
        vrpn_uint32 len, payload_len, ceil_len;

#ifdef  VERBOSE2
    printf("vrpn_NetConnection::handle_udp_messages() something to read\n");
#endif
        // Read the buffer and reset the buffer pointer
        inbuf_ptr = d_udp_inbuf;
        if ( (inbuf_len = recv(fd, d_udp_inbuf, sizeof(d_UDPinbufToAlignRight), 0)) == -1) {
           perror("vrpn: vrpn_NetConnection::handle_udp_messages: recv() failed");
           return -1;
        }

          // Parse each message in the buffer
          while ( (inbuf_ptr - d_udp_inbuf) != (vrpn_int32)inbuf_len) {

        // Read and parse the header
        // skip up to alignment
        vrpn_uint32 header_len = sizeof(header);
        if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
        
        if ( ((inbuf_ptr - d_udp_inbuf) + header_len) > (vrpn_uint32)inbuf_len) {
           fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read header");
           return -1;
        }
        memcpy(header, inbuf_ptr, sizeof(header));
        inbuf_ptr += header_len;
        len = ntohl(header[0]);
        time.tv_sec = ntohl(header[1]);
        time.tv_usec = ntohl(header[2]);
        service = ntohl(header[3]);
        type = ntohl(header[4]);

#ifdef VERBOSE
        printf("Message type %ld (local type %ld), service %ld received\n",
            type,translate_remote_type_to_local(type),service);
#endif
        
        // Figure out how long the message body is, and how long it
        // is including any padding to make sure that it is a
        // multiple of vrpn_ALIGN bytes long.
        payload_len = len - header_len;
        ceil_len = payload_len;
        if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}

        // Make sure we received enough to cover the entire payload
        if ( ((inbuf_ptr - d_udp_inbuf) + ceil_len) > inbuf_len) {
           fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read payload");
           return -1;
        }

        // Call the handler for this message type
        // If it returns nonzero, return an error.
        if (type >= 0) {    // User handler, map to local id

            // adjust timestamp before sending to vrpn_FileLogger,
            // because that is no longer done in there
            timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);

            // log regardless of whether local id is set,
            // but only process if it has been (ie, log ALL
            // incoming data -- use a filter on the log
            // if you don't want some of it).

            if (d_local_logmode & vrpn_LOG_INCOMING) {
                if (d_logger_ptr->log_message(payload_len, tvTemp,
                                        translate_remote_type_to_local(type),
                                        translate_remote_service_to_local(service),
                                        inbuf_ptr)) {
                    return -1;
                }
            }

            if (translate_remote_type_to_local(type) >= 0) {
                if (d_controller_token->do_callbacks_for(translate_remote_type_to_local(type),
                                     translate_remote_service_to_local(service),
                                     time, payload_len, inbuf_ptr)) {
                    return -1;
                }
            }
            
        } else {    // System handler

            // adjust timestamp before sending to vrpn_FileLogger,
            // because that is no longer done in there
            timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);
          
            if (d_local_logmode & vrpn_LOG_INCOMING)
                if (d_logger_ptr->log_message(payload_len, tvTemp,
                                translate_remote_type_to_local(type),
                                translate_remote_service_to_local(service),
                                inbuf_ptr))
                    return -1;
            
            // Fill in the parameter to be passed to the routines
            vrpn_HANDLERPARAM p;
            p.type = type;
            p.service = service;
            p.msg_time = time;
            p.payload_len = payload_len;
            p.buffer = inbuf_ptr;
            
            if (system_messages[-type](this, p)) {
                fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Nonzero system return\n");
                return -1;
            }
        }
        inbuf_ptr += ceil_len;
        
        // Got one more message
        num_messages_read++;
          }
      }

    } while (sel_ret);

    return num_messages_read;
}


// handle incoming messages on the multicast channel
// this function is called by NetConnection::handle_incoming_messages
// Return the number of messages read, or -1 on failure.
vrpn_int32 vrpn_NetConnection::handle_mcast_messages(/* XXX */){

    vrpn_int32 num_messages_read = 0;
    vrpn_int32 ret_val;
    // i think that the multicast handle_incoming_message should only read one
    // message from the multicast channel at a time. it should be called in a loop
    // to process all messages that might be waiting on that socket

        
    vrpn_int32  header[5];
    struct timeval  time;
    vrpn_int32  service, type;
    vrpn_uint32 len, payload_len, ceil_len;

    // XXX where do these really come from?
    char * inbuf_ptr;
    vrpn_int32 inbuf_len;

    // HACK - declared time_arg so this would compile. multicast code is inactive
    timeval *time_arg;
    gettimeofday(time_arg,NULL);

    while( (ret_val = mcast_recvr->handle_incoming_message(d_mcast_inbuf,time_arg)) != -1 ){
    // could be a different return code for no message versus an error

        // parse message into reports

#ifdef  VERBOSE2
    printf("vrpn_NetConnection::handle_mcast_messages() something to read\n");
#endif
        // Reset the buffer pointer
        inbuf_ptr = d_mcast_inbuf;

        // Parse each report in the buffer
        while ( (inbuf_ptr - d_mcast_inbuf) != (vrpn_int32)inbuf_len) {

            // Read and parse the header
            // skip up to alignment
            vrpn_uint32 header_len = sizeof(header);
            if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
        
            if ( ((inbuf_ptr - d_mcast_inbuf) + header_len) > (vrpn_uint32)inbuf_len) {
                fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read header");
                return -1;
            }
            memcpy(header, inbuf_ptr, sizeof(header));
            inbuf_ptr += header_len;
            len = ntohl(header[0]);
            time.tv_sec = ntohl(header[1]);
            time.tv_usec = ntohl(header[2]);
            service = ntohl(header[3]);
            type = ntohl(header[4]);

#ifdef VERBOSE
        printf("Message type %ld (local type %ld), service %ld received\n",
            type,translate_remote_type_to_local(type),service);
#endif
        
            // Figure out how long the message body is, and how long it
            // is including any padding to make sure that it is a
            // multiple of vrpn_ALIGN bytes long.
            payload_len = len - header_len;
            ceil_len = payload_len;
            if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}
    
            // Make sure we received enough to cover the entire payload
            if ( ((inbuf_ptr - d_udp_inbuf) + ceil_len) > inbuf_len) {
               fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read payload");
               return -1;
            }

            // Call the handler for this message type
            // If it returns nonzero, return an error.
            if (type >= 0) {    // User handler, map to local id

                // adjust timestamp before sending to vrpn_FileLogger,
                // because that is no longer done in there
                timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);

                // log regardless of whether local id is set,
                // but only process if it has been (ie, log ALL
                // incoming data -- use a filter on the log
                // if you don't want some of it).

                if (d_local_logmode & vrpn_LOG_INCOMING) {
                    if (d_logger_ptr->log_message(payload_len, tvTemp,
                                    translate_remote_type_to_local(type),
                                    translate_remote_service_to_local(service),
                                    inbuf_ptr)) {
                        return -1;
                    }
                }
                
                if (translate_remote_type_to_local(type) >= 0) {
                    if (d_controller_token->do_callbacks_for(translate_remote_type_to_local(type),
                                         translate_remote_service_to_local(service),
                                         time, payload_len, inbuf_ptr)) {
                        return -1;
                    }
                }
                
            }
            else {  // System handler

                // adjust timestamp before sending to vrpn_FileLogger,
                // because that is no longer done in there
                timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);
                       
                if (d_local_logmode & vrpn_LOG_INCOMING) {
                    if (d_logger_ptr->log_message(payload_len, tvTemp,
                        translate_remote_type_to_local(type),
                        translate_remote_service_to_local(service),
                        inbuf_ptr)) {
                    return -1;
                    }
                }

                // Fill in the parameter to be passed to the routines
                vrpn_HANDLERPARAM p;
                p.type = type;
                p.service = service;
                p.msg_time = time;
                p.payload_len = payload_len;
                p.buffer = inbuf_ptr;

                if (system_messages[-type](this, p)) {
                    fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Nonzero system return\n");
                    return -1;
                }
            }
            inbuf_ptr += ceil_len;
            
        } // end report parsing while loop
        
        // got another message
        num_messages_read++;

    } // end while( mcast_recvr->handle_incoming_message() )

    if( ret_val == -1 ) {
        return ret_val;
    } else {
        return num_messages_read;
    }
}


// XXX - requires functions only found in NetConnection
// Get the UDP port description from the other side and connect the
// outgoing UDP socket to that port so we can send lossy but time-
// critical (tracker) messages that way.

vrpn_int32 vrpn_NetConnection::handle_incoming_udp_message(
    void *userdata,
    vrpn_HANDLERPARAM p)
{
    char    rhostname[1000];    // name of remote host
    vrpn_NetConnection * me = (vrpn_NetConnection *)userdata;

#ifdef  VERBOSE
    printf("  Received request for UDP channel to %s\n", p.buffer);
#endif

    // Get the name of the remote host from the buffer (ensure terminated)
    strncpy(rhostname, p.buffer, sizeof(rhostname));
    rhostname[sizeof(rhostname)-1] = '\0';

    // Open the UDP outbound port and connect it to the port on the
    // remote machine.
    // (remember that the service field holds the UDP port number)
    me->udp_outbound = me->connect_udp_to(rhostname, (int)p.service);
    if (me->udp_outbound == -1) {
        perror("vrpn: vrpn_NetConnection: Couldn't open outbound UDP link");
        return -1;
    }

    // Put this here because currently every connection opens a UDP
    // port and every host will get this data.  Previous implementation
    // did it in connect_tcp_to, which only gets called by servers.

    strncpy(me->rhostname, rhostname,
        sizeof(me->rhostname));

#ifdef  VERBOSE
    printf("  Opened UDP channel to %s:%d\n", rhostname, p.service);
#endif
    return 0;
}

// }}} end vrpn_NetConnection: protected: send and receive functions

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: public: connection setup: server side
//
//==========================================================================
//==========================================================================


vrpn_int32 vrpn_NetConnection::connect_to_client (const char *msg)
{

    if (status != vrpn_CONNECTION_LISTEN) return -1;
    printf("vrpn: Connection request received: %s\n", msg);
    tcp_sock = connect_tcp_to(msg);
    if (tcp_sock != -1) {
        status = vrpn_CONNECTION_CONNECTED;
    }
    if (status != vrpn_CONNECTION_CONNECTED) {   // Something broke
        return -1;
    }
    else
        handle_connection();
    return 0;
}
// }}} end vrpn_NetConnection: public: connection setup: server side

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: protected: connection setup: server side

//
//==========================================================================
//==========================================================================

//  This routine opens a TCP socket and connects it to the machine and port
// that are passed in the msg parameter.  This is a string that contains
// the machine name, a space, then the port number.
//  The routine returns -1 on failure and the file descriptor on success.

vrpn_int32 vrpn_NetConnection::connect_tcp_to (const char * msg)
{   
    char    machine [5000];
    vrpn_int32  port;
#ifndef _WIN32
    vrpn_int32  server_sock;        /* Socket on this host */
#else
    SOCKET server_sock;
#endif
    struct  sockaddr_in client;     /* The name of the client */
    struct  hostent *host;          /* The host to connect to */

    // Find the machine name and port number
    if (sscanf(msg, "%s %d", machine, &port) != 2) {
        return -1;
    }

    /* set up the socket */

    server_sock = socket(AF_INET,SOCK_STREAM,0);
    if ( server_sock < 0 ) {
        fprintf(stderr,
              "vrpn_NetConnection::connect_tcp_to: can't open socket\n");
        return(-1);
    }
    client.sin_family = AF_INET;
    host = gethostbyname(machine);
    if (host) {

#ifdef CRAY
        { 
            vrpn_int32 i;
            u_long foo_mark = 0;
            for  (i = 0; i < 4; i++) {
                u_long one_char = host->h_addr_list[0][i];
                foo_mark = (foo_mark << 8) | one_char;
            }
            client.sin_addr.s_addr = foo_mark;
        }
#else

        memcpy(&(client.sin_addr.s_addr), host->h_addr,  host->h_length);
#endif

    } else {

        perror("gethostbyname error:");
#if !defined(hpux) && !defined(__hpux) && !defined(_WIN32)  && !defined(sparc)
        herror("gethostbyname error:");
#endif

#ifdef _WIN32

// gethostbyname() fails on SOME Windows NT boxes, but not all,
// if given an IP octet string rather than a true name.
// Until we figure out WHY, we have this extra clause in here.
// It probably wouldn't hurt to enable it for non-NT systems
// as well.

        vrpn_int32 retval;
        vrpn_uint32 a, b, c, d;
        retval = sscanf(machine, "%u.%u.%u.%u", &a, &b, &c, &d);
        if (retval != 4) {
            fprintf(stderr,
                "vrpn_NetConnection::connect_tcp_to:  "
                    "error: bad address string\n");
            return -1;
        }
#else   // not _WIN32

// Note that this is the failure clause of gethostbyname() on
// non-WIN32 systems
        
        fprintf(stderr,
            "vrpn_NetConnection::connect_tcp_to:  "
                            "error finding host by name\n");
        return -1;
    }
#endif
#ifdef _WIN32
// XXX OK, so this doesn't work.  What's wrong???

        client.sin_addr.s_addr = (a << 24) + (b << 16) + (c << 8) + d;
        //client.sin_addr.s_addr = (d << 24) + (c << 16) + (b << 8) + a;
        fprintf(stderr, "vrpn_NetConnection::connect_tcp_to:  "
                      "gethostname() failed;  we think we're\n"
                      "looking for %d.%d.%d.%d.\n", a, b, c, d);

        // here we can try an alternative strategy:
        vrpn_uint32 addr = inet_addr(machine);
        if (addr == INADDR_NONE) {  // that didn't work either
            fprintf(stderr, "vrpn_NetConnection::connect_tcp_to:  "
                    "error reading address format\n");

            return -1;
        } else {
            host = gethostbyaddr((char *)&addr,sizeof(addr), AF_INET);
            if (host){
                printf("gethostbyaddr() was successful\n");
                memcpy(&(client.sin_addr.s_addr), host->h_addr,  host->h_length);
            }
            else{
                fprintf(stderr, "vrpn_NetConnection::connect_tcp_to:  "
                            " gethostbyaddr() failed\n");
                return -1;
            }
        }
    }

        
#endif


#ifndef _WIN32
    client.sin_port = htons(port);
#else
    client.sin_port = htons((u_short)port);
#endif
 
    if (connect(server_sock,(struct sockaddr*)&client,sizeof(client)) < 0 ){
        fprintf(stderr,
             "vrpn_NetConnection::connect_tcp_to: Could not connect\n");
#ifdef _WIN32
        vrpn_int32 error = WSAGetLastError();
        fprintf(stderr, "Winsock error: %d\n", error);
#endif
        close(server_sock);
        return(-1);
    }

    /* Set the socket for TCP_NODELAY */

    {   struct  protoent    *p_entry;
        vrpn_int32  nonzero = 1;

        if ( (p_entry = getprotobyname("TCP")) == NULL ) {
            fprintf(stderr,
              "vrpn_NetConnection::connect_tcp_to: getprotobyname() failed.\n");
            close(server_sock);
            return(-1);
        }

//#ifndef _WIN32
        if (setsockopt(server_sock, p_entry->p_proto,
            TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
//#else
//      if (setsockopt(server_sock, p_entry->p_proto,
//          TCP_NODELAY, (const char *)&nonzero, sizeof(nonzero))==SOCKET_ERROR) {
//#endif

            perror("vrpn_NetConnection::connect_tcp_to: setsockopt() failed");
            close(server_sock);
            return(-1);
        }
    }

    return(server_sock);
}

//---------------------------------------------------------------------------
//  This routine opens a UDP socket and then connects it to a the specified
// port on the specified machine.
//  The routine returns -1 on failure and the file descriptor on success.

#ifdef _WIN32
static  SOCKET connect_udp_to (char * machine, vrpn_int16 portno)
{
   SOCKET sock; 
#else
static  vrpn_int32 connect_udp_to (char * machine, vrpn_int16 portno)
{
   vrpn_int32 sock;
#endif
   struct   sockaddr_in client;     /* The name of the client */
   struct   hostent *host;          /* The host to connect to */

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)  {
    perror("vrpn: vrpn_NetConnection: can't open socket");
    return INVALID_SOCKET;
   }

   // Connect it to the specified port on the specified machine
    client.sin_family = AF_INET;
    if ( (host=gethostbyname(machine)) == NULL ) {
#ifndef _WIN32
        close(sock);
        fprintf(stderr,
             "vrpn: connect_udp_to: error finding host by name\n");
        return(INVALID_SOCKET);
#else
        vrpn_uint32 addr = inet_addr(machine);
        if (addr == INADDR_NONE){
            close(sock);
            fprintf(stderr,
                 "vrpn: connect_udp_to: error finding host by name\n");
            return(INVALID_SOCKET);
        } else{
            host = gethostbyaddr((char *)&addr,sizeof(addr), AF_INET);
            if (!host){
                close(sock);
                fprintf(stderr,
                     "vrpn: connect_udp_to: error finding host by name\n");
                return(INVALID_SOCKET);
            }
        }
#endif
    }
#ifdef CRAY
        { vrpn_int32 i;
          u_long foo_mark = 0;
          for  (i = 0; i < 4; i++) {
              u_long one_char = host->h_addr_list[0][i];
              foo_mark = (foo_mark << 8) | one_char;
          }
          client.sin_addr.s_addr = foo_mark;
        }
#elif defined(_WIN32)
    memcpy((char*)&(client.sin_addr.s_addr), host->h_addr, host->h_length);
#else
    bcopy(host->h_addr, (char*)&(client.sin_addr.s_addr), host->h_length);
#endif
    client.sin_port = htons(portno);
    if (connect(sock,(struct sockaddr*)&client,sizeof(client)) < 0 ){
        perror("vrpn: connect_udp_to: Could not connect to client");
#ifdef _WIN32
        vrpn_int32 error = WSAGetLastError();
        fprintf(stderr, "Winsock error: %d\n", error);
#endif
        close(sock);
        return(INVALID_SOCKET);
    }

   return sock;
}

// handle system message from peer connection
// informing if it is multicast capable
vrpn_int32 vrpn_NetConnection::mcast_reply_handler(){

    // unpack message

    // set local data member
    // set_peer_mcast_capable(/* true/false */);

    // inform ServerConnectionController whether
    // client is mcast capable

    return 0;
}

// get mcast info from mcast service and pack into message
vrpn_int32 vrpn_NetConnection::pack_mcast_description(vrpn_int32 service){

    /*======================== INACTIVE MCAST CODE ========================
    vrpn_uint32 pid;
    vrpn_uint32 ip;
    char temp_buf[20];
    struct timeval now;
    char ** insertPt;
    vrpn_int32 buflen = siseof(d_mcast_cookie);

    gettimeofday(&now, NULL);

    // pack magic mcast cookie
    // cookie used to determine if mcast message came from
    // who we expected it from. since we are using random
    // mulitcast addresses, we could theoretically get
    // messages from unwanted sources
    // 
    // first ip addr as 32 bit u_int
    // then pid as 32 bit u_int
    vrpn_getmyIP(temp_buf,sizeof(temp_buf));
    ip = inet_addr(temp_buf);
    pid = getpid();
    insertPt = d_mcast_cookie;
    vrpn_buffer(insertPt,&buflen,ip);
    vrpn_buffer(insertPt,&buflen,pid);
    
    // NOT DONE - need to pack d_mcast_cookie into first part of message

    return pack_message((vrpn_uint32)sizeof(vrpn_McastGroupDescrp), 
                        now, vrpn_CONNECTION_MCAST_DESCRIPTION,
                        service, controller->get_mcast_info(),
                        vrpn_CONNECTION_RELIABLE, vrpn_false);

    ==============================*/
    return 0;
}

// }}} end vrpn_NetConnection: protected: connection setup: server side

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: public: connection setup: client side
//
//==========================================================================
//==========================================================================


// make connection to server
vrpn_int32 vrpn_NetConnection::connect_to_server(
    const char* machine, 
    vrpn_int16 port)
{
    vrpn_int32 retval = 0;

    // initiate connection by sending message to well
    // known port on server
    retval = udp_request_connection_call(machine,port);
    if( retval == -1 ){
        fprintf(stderr,"vrpn_NetConnection::udp_request_connection_call :",
                " error connecting to server.\n");
        return retval;
    } else {
        tcp_sock = retval;
    }

    // setup rest of connection
    handle_connection();
    return retval;
}


// This is like sdi_start_server except that the convention for
// passing information on the client machine to the server program is 
// different; everything else has been left the same
/**********************************
 *      This routine will start up a given server on a given machine.  The
 * name of the machine requesting service and the socket number to connect to
 * are supplied after the -client argument. The "args" parameters are
 * passed next.  They should be space-separated arguments, each of which is
 * passed separately.  It is NOT POSSIBLE to send parameters with embedded
 * spaces.
 *      This routine returns a file descriptor that points to the socket
 * to the server on success and -1 on failure.
 **********************************/
int vrpn_NetConnection::start_server(const char *machine, char *server_name, char *args)
{
#ifdef  _WIN32
        fprintf(stderr,"VRPN: vrpn_NetConnection::start_server not ported to NT!\n");
        return -1;
#else
        int     pid;    /* Child's process ID */
        int     server_sock;    /* Where the accept returns */
        int     child_socket;   /* Where the final socket is */
        int     PortNum;        /* Port number we got */

        /* Open a socket and insure we can bind it */
	if (get_a_TCP_socket(&server_sock, &PortNum)) {
		fprintf(stderr,"vrpn_start_server: Cannot get listen socket\n");
		return -1;
	}

        if ( (pid = fork()) == -1) {
                fprintf(stderr,"vrpn_start_server: cannot fork().\n");
                close(server_sock);
                return(-1);
        }
        if (pid == 0) {  /* CHILD */
                int     loop;
                int     ret;
                int     num_descriptors;/* Number of available file descr */
                char    myIPchar[100];    /* Host name of this host */
                char    command[600];   /* Command passed to system() call */
                char    *rsh_to_use;    /* Full path to Rsh command. */

                if (vrpn_getmyIP(myIPchar,sizeof(myIPchar))) {
                        fprintf(stderr,
                           "vrpn_start_server: Error finding my IP\n");
                        close(server_sock);
                        return(-1);
                }

                /* Close all files except stdout and stderr. */
                /* This prevents a hung child from keeping devices open */
                num_descriptors = getdtablesize();
                for (loop = 0; loop < num_descriptors; loop++) {
                  if ( (loop != 1) && (loop != 2) ) {
                        close(loop);
                  }
                }

                /* Find the RSH command, either from the environment
                 * variable or the default, and use it to start up the
                 * remote server. */

                if ( (rsh_to_use =(char *)getenv("VRPN_RSH")) == NULL) {
                        rsh_to_use = RSH;
                }
                sprintf(command,"%s %s %s %s -client %s %d",rsh_to_use, machine,
                        server_name, args, myIPchar, PortNum);
                ret = system(command);
                if ( (ret == 127) || (ret == -1) ) {
                        fprintf(stderr,
                                "vrpn_start_server: system() failed !!!!!\n");
                        perror("Error");
                        fprintf(stderr, "Attempted command was: '%s'\n",
                                command);
                        close(server_sock);
                        exit(-1);  /* This should never occur */
                }
                exit(0);

        } else {  /* PARENT */
                int     waitloop;

                /* Check to see if the child
                 * is trying to call us back.  Do SERVCOUNT waits, each of
                 * which is SERVWAIT long.
                 * If the child dies while we are waiting, then we can be
                 * sure that they will not be calling us back.  Check for
                 * this while waiting for the callback. */

                for (waitloop = 0; waitloop < (SERVCOUNT); waitloop++) {
		    int ret;
                    pid_t deadkid;
#if defined(sparc) || defined(FreeBSD)
                    int status;  // doesn't exist on sparc_solaris or FreeBSD
#else
                    union wait status;
#endif
		    
                    /* Check to see if they called back yet. */
		    ret = poll_for_accept(server_sock, &child_socket, SERVWAIT);
		    if (ret == -1) {
			    fprintf(stderr,"vrpn_start_server: Accept poll failed\n");
			    close(server_sock);
			    return -1;
		    }
		    if (ret == 1) {
			    break;	// Got it!
		    }

                    /* Check to see if the child is dead yet */
#if defined(hpux) || defined(sgi) || defined(__hpux)
                    /* hpux include files have the wrong declaration */
                    deadkid = wait3((int*)&status, WNOHANG, NULL);
#else
                    deadkid = wait3(&status, WNOHANG, NULL);
#endif
                    if (deadkid == pid) {
                        fprintf(stderr,
                                "vrpn_start_server: server process exited\n");
                        close(server_sock);
                        return(-1);
                    }
                }
		if (waitloop == SERVCOUNT) {
			fprintf(stderr,
                        "vrpn_start_server: server failed to connect in time\n");
                    fprintf(stderr,
                        "                  (took more than %d seconds)\n",
                        SERVWAIT*SERVCOUNT);
                    close(server_sock);
                    kill(pid,SIGKILL);
                    wait(0);
                    return(-1);
                }

                close(server_sock);
                return(child_socket);
        }
        return 0;
#endif
}

// }}} end vrpn_NetConnection: public: connection setup: client side

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: protected: connection setup: client side

//
//==========================================================================
//==========================================================================

//---------------------------------------------------------------------------
// This section deals with implementing a method of connection termed a
// UDP request.  This works by having the client open a TCP socket that
// it listens on. It then lobs datagrams to the server asking to be
// called back at the socket. This allows it to timeout on waiting for
// a connection request, resend datagrams in case some got lost, or give
// up at any time. The whole algorithm is implemented in the
// vrpn_udp_request_call() function; the functions before that are helper
// functions that have been broken out to allow a subset of the algorithm
// to be run by a connection whose server has dropped and they want to
// re-establish it.

// This routine will lob a datagram to the given port on the given
// machine asking it to call back at the port on this machine that
// is also specified. It returns 0 on success and -1 on failure.

int vrpn_NetConnection::udp_request_lob_packet(
		const char * machine,	// Name of the machine to call
		const int remote_port,	// UDP port on remote machine
		const int local_port)	// TCP port on this machine
{
	SOCKET	udp_sock;		/* We lob datagrams from here */
	struct sockaddr_in udp_name;	/* The UDP socket binding name */
	int	udp_namelen;
    struct	hostent *host;          /* The host to connect to */
	char	msg[150];		/* Message to send */
	vrpn_int32	msglen;		/* How long it is (including \0) */
	char	myIPchar[100];		/* IP decription this host */

	/* Create a UDP socket and connect it to the port on the remote
	 * machine. */

        udp_sock = socket(AF_INET,SOCK_DGRAM,0);
        if (udp_sock < 0) {
	    fprintf(stderr,"vrpn_udp_request_lob_packet: can't open udp socket().\n");
            return(-1);
        }
	udp_name.sin_family = AF_INET;
	udp_namelen = sizeof(udp_name);
	host = gethostbyname(machine);
        if (host) {

#ifdef CRAY

          { int i;
            u_long foo_mark = 0;
            for  (i = 0; i < 4; i++) {
                u_long one_char = host->h_addr_list[0][i];
                foo_mark = (foo_mark << 8) | one_char;
            }
            udp_name.sin_addr.s_addr = foo_mark;
          }

#else


	  memcpy(&(udp_name.sin_addr.s_addr), host->h_addr,  host->h_length);
#endif

        } else {

#ifdef _WIN32

// gethostbyname() fails on SOME Windows NT boxes, but not all,
// if given an IP octet string rather than a true name.
// Until we figure out WHY, we have this extra clause in here.
// It probably wouldn't hurt to enable it for non-NT systems
// as well.

          int retval;
          retval = sscanf(machine, "%u.%u.%u.%u",
                          ((char *) &udp_name.sin_addr.s_addr)[0],
                          ((char *) &udp_name.sin_addr.s_addr)[1],
                          ((char *) &udp_name.sin_addr.s_addr)[2],
                          ((char *) &udp_name.sin_addr.s_addr)[3]);

          if (retval != 4) {

#endif  // _WIN32

// Note that this is the failure clause of gethostbyname() on
// non-WIN32 systems, but of the sscanf() on WIN32 systems.

		close(udp_sock);
		fprintf(stderr,
			"vrpn_udp_request_lob_packet: error finding host by name\n");
		return(-1);

#ifdef _WIN32

          }

#endif

        }

#ifndef _WIN32
	udp_name.sin_port = htons(remote_port);
#else
	udp_name.sin_port = htons((u_short)remote_port);
#endif
        if ( connect(udp_sock,(struct sockaddr*)&udp_name,udp_namelen) ) {
	    fprintf(stderr,"vrpn_udp_request_lob_packet: can't bind udp socket().\n");
	    close(udp_sock);
	    return(-1);
        }

	/* Fill in the request message, telling the machine and port that
	 * the remote server should connect to.  These are ASCII, separated
	 * by a space. */

	if (vrpn_getmyIP(myIPchar, sizeof(myIPchar))) {
		fprintf(stderr,
		   "vrpn_udp_request_lob_packet: Error finding local hostIP\n");
		close(udp_sock);
		return(-1);
	}
	sprintf(msg, "%s %d", myIPchar, local_port);
	msglen = strlen(msg) + 1;	/* Include the terminating 0 char */

	// Lob the message
	if (send(udp_sock, msg, msglen, 0) == -1) {
	  perror("vrpn_udp_request_lob_packet: send() failed");
	  close(udp_sock);
	  return -1;
	}

	close(udp_sock);	// We're done with the port
	return 0;
}

// This routine will get a TCP socket that is ready to accept connections.
// That is, listen() has already been called on it.
// It will get whatever socket is available from the system. It returns
// 0 on success and -1 on failure. On success, it fills in the pointers to
// the socket and the port number of the socket that it obtained.

int vrpn_NetConnection::get_a_TCP_socket(SOCKET *listen_sock, int *listen_portnum)
{
    struct sockaddr_in listen_name;	/* The listen socket binding name */
	int	listen_namelen;

	/* Create a TCP socket to listen for incoming connections from the
	 * remote server. */

        listen_name.sin_family = AF_INET;		/* Internet socket */
        listen_name.sin_addr.s_addr = INADDR_ANY;	/* Accept any port */
        listen_name.sin_port = htons(0);
        *listen_sock = socket(AF_INET,SOCK_STREAM,0);
	listen_namelen = sizeof(listen_name);
        if (*listen_sock < 0) {
		fprintf(stderr,"vrpn_get_a_TCP_socket: can't open socket().\n");
                return(-1);
        }
        if ( bind(*listen_sock,(struct sockaddr*)&listen_name,listen_namelen) ) {
		fprintf(stderr,"vrpn_get_a_TCP_socket: can't bind socket().\n");
                close(*listen_sock);
                return(-1);
        }
        if (getsockname(*listen_sock,
	                (struct sockaddr *) &listen_name,
                        GSN_CAST &listen_namelen)) {
		fprintf(stderr,
			"vrpn_get_a_TCP_socket: cannot get socket name.\n");
                close(*listen_sock);
                return(-1);
        }
	*listen_portnum = ntohs(listen_name.sin_port);
	if ( listen(*listen_sock,2) ) {
		fprintf(stderr,"vrpn_get_a_TCP_socket: listen() failed.\n");
		close(*listen_sock);
		return(-1);
	}

	return 0;
}

// This routine will check the listen socket to see if there has been a
// connection request. If so, it will accept a connection on the accept
// socket and set TCP_NODELAY on that socket. The attempt will timeout
// in the amount of time specified.  If the accept and set are
// successfull, it returns 1. If there is nothing asking for a connection,
// it returns 0. If there is an error along the way, it returns -1.

int vrpn_NetConnection::poll_for_accept(
    SOCKET listen_sock, 
    SOCKET *accept_sock, 
    double timeout)
{
	fd_set	rfds;
	struct	timeval t;

	// See if we have a connection attempt within the timeout
	FD_ZERO(&rfds);
	FD_SET(listen_sock, &rfds);	/* Check for read (connect) */
	t.tv_sec = (long)(timeout);
	t.tv_usec = (long)( (timeout - t.tv_sec) * 1000000L );
	if (select(32, &rfds, NULL, NULL, &t) == -1) {
	  perror("vrpn_NetConnection::poll_for_accept: select() failed");
	  return -1;
	}
	if (FD_ISSET(listen_sock, &rfds)) {	/* Got one! */
		/* Accept the connection from the remote machine and set TCP_NODELAY
		* on the socket. */
		if ( (*accept_sock = accept(listen_sock,0,0)) == -1 ) {
			perror("vrpn_NetConnection::poll_for_accept: accept() failed");
			return -1;
		}

		{	struct	protoent	*p_entry;
			int	nonzero = 1;

			if ( (p_entry = getprotobyname("TCP")) == NULL ) {
				fprintf(stderr,
				"vrpn_NetConnection::poll_for_accept: getprotobyname() failed.\n");
				close(*accept_sock);
				return(-1);
			}
	
			if (setsockopt(*accept_sock, p_entry->p_proto,
			TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
				perror("vrpn_NetConnection::poll_for_accept: setsockopt() failed");
				close(*accept_sock);
				return(-1);
			}
		}

		return 1;	// Got one!
	}

	return 0;	// Nobody called
}

/***************************
 *	This routine will send UDP packets requesting a TCP callback from
 * a remote server given the name of the machine and the socket number to
 * send the requests to.
 *	This routine returns the file descriptor of the socket on success
 * and -1 on failure.
 ***************************/
SOCKET vrpn_NetConnection::udp_request_connection_call(
    const char * machine, 
    int port)
{
	SOCKET	listen_sock;		/* The socket to listen on */
	int	listen_portnum;		/* Port number we're listening on */
	SOCKET	accept_sock;		/* The socket we get when accepting */
	int	try_connect;

	// Open the socket that we will listen on for connections
	if (get_a_TCP_socket(&listen_sock, &listen_portnum)) {
		fprintf(stderr,"vrpn_NetConnection::udp_request_call(): Can't get listen sock\n");
		return -1;
	}

	/* Repeat sending the request to the server and checking for it
	 * to call back until either there is a connection request from the
	 * server or it times out as many times as we're supposed to try. */

	for (try_connect = 0; try_connect < UDP_CALL_RETRIES; try_connect++) {
		int	ret;

		/* Send a packet to the server asking for a connection. */
		if (udp_request_lob_packet(machine, port, listen_portnum)
			== -1) {
		  perror("vrpn_NetConnection::udp_request_call: Can't request connection");
		  close(listen_sock);
		  return -1;
		}

		ret = poll_for_accept(listen_sock, &accept_sock, UDP_CALL_TIMEOUT);
		if (ret == -1) {
			fprintf(stderr,"vrpn_NetConnection::udp_request_call: Accept poll failed\n");
			close(listen_sock);
			return -1;
		}
		if (ret == 1) {
			break;		// Got one!
		}
	}
	if (try_connect == UDP_CALL_RETRIES) {	/* We didn't get an answer */
		fprintf(stderr,"vrpn_NetConnection::udp_request_call: No reply from server\n");
		fprintf(stderr,"                      (Server down or busy)\n");
		close(listen_sock);
		return -1;
	}

	close(listen_sock);	// We're done with the port

	return accept_sock;
}


// respond to system message vrpn_CONNECTION_MCAST_DESCRIPTION
// if mcast capable, create a mcast recvr w/ info from server
// then call pack_mcast_reply
vrpn_int32 vrpn_NetConnection::mcast_description_handler(char* message){
    
    if( message ){ // if message is not NULL i.e. server is mcast capable
        vrpn_McastGroupDescrp *descrp = new vrpn_McastGroupDescrp();
        memcpy(descrp,message,sizeof(vrpn_McastGroupDescrp));
        mcast_recvr = new vrpn_UnreliableMulticastRecvr(descrp->d_mcast_group,descrp->d_mcast_port);
    

        // if creation of multicast receiver did not
        // succeed, set global var to false before 
        // sending reply
        if( mcast_recvr->created_correctly() ){
            set_mcast_capable(vrpn_true);
            pack_mcast_reply();
        }
        else {
            set_mcast_capable(vrpn_false);
            pack_mcast_reply();
        }
    }
    return 0;
}

// pack reply to whether client is multicast capable
// reply gets sent to server during connection initialization
vrpn_int32 vrpn_NetConnection::pack_mcast_reply(){

    // pack value of d_mcast_capable into buffer
    return 0;
}

// }}} end vrpn_NetConnection: protected: connection setup: client side

//==========================================================================
//==========================================================================
//
// {{{ vrpn_NetConnection: public: connection setup: common functions

//
//==========================================================================
//==========================================================================

// called when new client is trying to connect
void vrpn_NetConnection::handle_connection()
{

   // Set TCP_NODELAY on the socket
/*XXX It looks like this means something different to Linux than what I
    expect.  I expect it means to send acknowlegements without waiting
    to piggyback on reply.  This causes the serial port to fail.  As
    long as the other end has this set, things seem to work fine.  From
    this end, if we set it it fails (unless we're talking to another
    PC).  Russ Taylor

   {    struct  protoent        *p_entry;
    vrpn_int32     nonzero = 1;

    init();

    if ( (p_entry = getprotobyname("TCP")) == NULL ) {
        fprintf(stderr,
          "vrpn: vrpn_Connection: getprotobyname() failed.\n");
        close(tcp_sock); tcp_sock = -1;
        status = vrpn_CONNECTION_BROKEN;
        return;
    }

    if (setsockopt(tcp_sock, p_entry->p_proto,
        TCP_NODELAY, &nonzero, sizeof(nonzero))==-1) {
        perror("vrpn: vrpn_Connection: setsockopt() failed");
        close(tcp_sock); tcp_sock = -1;
        status = vrpn_CONNECTION_BROKEN;
        return;
    }
   }
*/

   // Ignore SIGPIPE, so that the program does not crash when a
   // remote client shuts down its TCP connection.  We'll find out
   // about it as an exception on the socket when we select() for
   // read.
   // Mips/Ultrix header file signal.h appears to be broken and
   // require the following cast
#ifndef _WIN32
 #ifdef ultrix
   signal( SIGPIPE, (void (*) (int)) SIG_IGN );
 #else
   signal( SIGPIPE, SIG_IGN );
 #endif
#endif

   // Set up the things that need to happen when a new connection is
   // started.
   if (setup_new_connection()) {
    fprintf(stderr,"vrpn_Connection::handle_connection():  "
            "Can't set up new connection!\n");
    drop_connection();
    return;
   }

   // adds connection to list of connections and invokes callbacks for
   // got_connection, and got_first_connection if any
   d_controller_token->got_a_connection(this);
}

// network initialization
//
// it handles aspects of a new connection like checking version
// numbers, telling the other side what udp port to send to, and
// transfering service and types to the other side
vrpn_int32 vrpn_NetConnection::setup_new_connection (
    vrpn_int32 remote_log_mode, 
    const char * remote_logfile_name)
{
    char    sendbuf [501];  // HACK
    char    recvbuf [501];  // HACK
    vrpn_int32  received_logmode;
    vrpn_int32  sendlen;
    vrpn_int32 retval;
    vrpn_uint16 udp_portnum;

  //sprintf(sendbuf, "%s  %c", vrpn_MAGIC, remote_log_mode);
  retval = write_vrpn_cookie(sendbuf, vrpn_cookie_size() + 1,
                             remote_log_mode);
  if (retval < 0) {
    perror("vrpn_Connection::setup_new_connection:  "
           "Internal error - array too small.  The code's broken.");
    return -1;
  }
  sendlen = vrpn_cookie_size();

    // Write the magic cookie header to the server
    if (vrpn_noint_block_write(tcp_sock, sendbuf, sendlen)
            != sendlen) {
      perror(
        "vrpn_Connection::setup_new_connection: Can't write cookie");
      return -1;
    }

    // Read the magic cookie from the server
    if (vrpn_noint_block_read(tcp_sock, recvbuf, sendlen)
            != sendlen) {
      perror(
        "vrpn_Connection::setup_new_connection: Can't read cookie");
      return -1;
    }
    //recvbuf[vrpn_MAGICLEN] = '\0';  // shouldn't be needed any more

  retval = check_vrpn_cookie(recvbuf);
  if (retval < 0){
    return -1;
  }

  // Synchronize clocks before any other user messages are sent. On
  // the server side, synchronize_clocks() is an empty function and
  // will not do anything.
  d_controller_token->synchronize_clocks();

    // Find out what log mode they want us to be in BEFORE we pack
    // type, service, and udp descriptions!  If it's nonzero, the
    // filename to use should come in a log_description message later.

    received_logmode = recvbuf[vrpn_MAGICLEN + 2] - '0';
    if ((received_logmode < 0) ||
            (received_logmode > (vrpn_LOG_INCOMING | vrpn_LOG_OUTGOING))) {
      fprintf(stderr, "vrpn_Connection::setup_new_connection:  "
                          "Got invalid log mode %d\n", received_logmode);
          return -1;
        }
    d_local_logmode |= received_logmode;

    if (remote_log_mode)
      if (pack_log_description(remote_log_mode,
                               remote_logfile_name) == -1) {
        fprintf(stderr, "vrpn_Connection::setup_new_connection:  "
                        "Can't pack remote logging instructions.\n");
        return -1;
      }

    if (udp_inbound == INVALID_SOCKET) {
      // Open the UDP port to accept time-critical messages on.
      udp_portnum = (vrpn_uint16)INADDR_ANY;
      if ((udp_inbound = vrpn_open_udp_socket(&udp_portnum)) == -1)  {
        fprintf(stderr, "vrpn_Connection::setup_new_connection:  "
                            "can't open UDP socket\n");
        return -1;
      }
    }

    // Tell the other side what port number to send its UDP messages to.
    if (pack_udp_description(udp_portnum) == -1) {
      fprintf(stderr,
        "vrpn_Connection::setup_new_connection: Can't pack UDP msg\n");
      return -1;
    }

    // XXX - commented out until new implementation is done
    // Pack messages that describe the types of messages and service
    // ID mappings that have been described to this connection.  These
    // messages use special IDs (negative ones).
//      for (i = 0; i < num_my_services; i++) {
//          pack_service_description(i);
//      }
//      for (i = 0; i < num_my_types; i++) {
//          pack_type_description(i);
//      }

    //=======================
    // This is where we would pass all the multicast capability stuff to
    // the other side. this is yet to be implemented [sain]

    // Send the messages
    if (send_pending_reports() == -1) {
      fprintf(stderr,
        "vrpn_Connection::setup_new_connection: Can't send UDP msg\n");
      return -1;
    }

    return 0;
}

// drop a connection with another machine
void vrpn_NetConnection::drop_connection()
{
    //XXX Store name when opening, say which dropped here
    printf("vrpn: NetConnection dropped\n");
    if (tcp_sock != INVALID_SOCKET) {
        close(tcp_sock);
        tcp_sock = INVALID_SOCKET;
    }
    if (udp_outbound != INVALID_SOCKET) {
        close(udp_outbound);
        udp_outbound = INVALID_SOCKET;
    }
    if (udp_inbound != INVALID_SOCKET) {
        close(udp_inbound);
        udp_inbound = INVALID_SOCKET;
    }

  d_controller_token->dropped_a_connection(this);

  if (d_local_logname) {
      close_log();
  }

  if( mcast_capable() ){
      delete mcast_recvr;
  }

}

/////////////////////////////
// the following are used during connection setup to communicate 
// settings and service/type information to the other end

vrpn_int32 vrpn_NetConnection::pack_type_description(vrpn_int32 which)
{
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32  len =  1; //strlen(my_types[which].name) + 1; XXX - comment out until ne impl. is done [sain]
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_TYPE_DESCRIPTION
   // whose service ID is the ID of the type that is being
   // described and whose body contains the length of the name
   // and then the name of the type.

#ifdef  VERBOSE
   printf("  vrpn_NetConnection: Packing type '%s'\n",my_types[which].name);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   //memcpy(&buffer[sizeof(len)], my_types[which].name, (vrpn_int32) len);XXX - comment out until ne impl. is done [sain]
   gettimeofday(&now,NULL);

   return pack_message((vrpn_uint32) (len + sizeof(len)), now,
    vrpn_CONNECTION_TYPE_DESCRIPTION, which, buffer,
    vrpn_CONNECTION_RELIABLE);
}

vrpn_int32 vrpn_NetConnection::pack_service_description(vrpn_int32 which)
{
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32  len = 1; //strlen(my_services[which]) + 1;XXX - comment out until ne impl. is done [sain]
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_SERVICE_DESCRIPTION
   // whose service ID is the ID of the service that is being
   // described and whose body contains the length of the name
   // and then the name of the service.

#ifdef  VERBOSE
    printf("  vrpn_NetConnection: Packing service '%s'\n", my_services[which]);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   //   memcpy(&buffer[sizeof(len)], my_services[which], (vrpn_int32) len);XXX - comment out until ne impl. is done [sain]
   gettimeofday(&now,NULL);

   return pack_message((vrpn_uint32)(len + sizeof(len)), now,
    vrpn_CONNECTION_SERVICE_DESCRIPTION, which, buffer,
    vrpn_CONNECTION_RELIABLE);
}

vrpn_int32 vrpn_NetConnection::pack_udp_description(vrpn_uint16 portno)
{
   struct timeval now;
   vrpn_uint32  portparam = portno;
   char myIPchar[1000];

   // Find the local host name
   if (vrpn_getmyIP(myIPchar, sizeof(myIPchar))) {
    perror("vrpn_NetConnection::pack_udp_description: can't get host name");
    return -1;
   }

   // Pack a message with type vrpn_CONNECTION_UDP_DESCRIPTION
   // whose service ID is the ID of the port that is to be
   // used and whose body holds the zero-terminated string
   // name of the host to contact.

#ifdef  VERBOSE
    printf("  vrpn_NetConnection: Packing UDP %s:%d\n", myIPchar,
        portno);
#endif
   gettimeofday(&now, NULL);

   return pack_message(strlen(myIPchar) + 1, now,
        vrpn_CONNECTION_UDP_DESCRIPTION,
    portparam, myIPchar, vrpn_CONNECTION_RELIABLE);
}

vrpn_int32 vrpn_NetConnection::pack_log_description (
    vrpn_int32 logmode,
    const char * logfile) 
{
  struct timeval now;

  // Pack a message with type vrpn_CONNECTION_LOG_DESCRIPTION whose
  // service ID is the logging mode to be used by the remote connection
  // and whose body holds the zero-terminated string name of the file
  // to write to.

  gettimeofday(&now, NULL);
  return pack_message(strlen(logfile) + 1, now,
                      vrpn_CONNECTION_LOG_DESCRIPTION, logmode, logfile,
                      vrpn_CONNECTION_RELIABLE);
}

// }}} end vrpn_NetConnection: public: connection setup: common functions

//==========================================================================
//==========================================================================




