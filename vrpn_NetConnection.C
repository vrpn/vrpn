#include "vrpn_NetConnection.h"

//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: public: send and receive functions
//
//**************************************************************************
//**************************************************************************

// rough draft of mainloop
vrpn_int32 vrpn_NetConnection::mainloop (const struct timeval * timeout){

	switch(status){
	case CONNECTED:
		handle_incoming_messages(timeout);
		handle_outgoing_messages(timeout);
		break;
	case CONNECTION_FAIL:
	case BROKEN:
		fprintf(stderr, "vrpn: Socket failure.  Giving up!\n");
		status = DROPPED;
		return -1;
	case DROPPED:
      	break;
   }

   return 0;
}


// This function gets called by the ConnectionController
// i view it as a replacement for pack_message() although they
// will probably have alot of the same functionality.
vrpn_int32 vrpn_NetConnection::handle_outgoing_messages(const struct timeval* pTimeout){

	// log message if necessary

	// if not multicasted, marshall into appropriate buffer
	if( vrpn_Mcast_Capable && 

	return 0;
}


// gets called by ConnectionController. handles any incoming messages
// on any of the channels, as well as logging and invoking callbacks.
// This function uses some of the functionality found in
// vrpn_NetConnection::handle_incoming_messages()
vrpn_int32 vrpn_NetConnection::handle_incoming_messages( const struct timeval* pTimeout ){

	vrpn_int32	tcp_messages_read;
	vrpn_int32	udp_messages_read;
	vrpn_int32	mcast_messages_read;

	timeval timeout;
	if (pTimeout) {
		timeout = *pTimeout;
	} else {
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
	}

#ifdef	VERBOSE2
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
	case LISTEN:
		// check_connection(pTimeout); now done in Controller
	    break;
		*/
	case CONNECTED: 
   

		// check for pending incoming tcp, udp, or multicast reports
		// we do this so that we can trigger out of the timeout
		// on either type of message without waiting on the other
    
		fd_set  readfds, exceptfds;
		FD_ZERO(&readfds);              /* Clear the descriptor sets */
		FD_ZERO(&exceptfds);

		// Read incoming messages from the UDP, TCP. and Multicast channels

		// *** N.B. ***
		// not sure what sockets are going to be called in new system
		// maybe we'll keep the same notation

		if (udp_inbound != -1) {
			FD_SET(udp_inbound, &readfds); /* Check for read */
			FD_SET(udp_inbound, &exceptfds);/* Check for exceptions */
		}
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
				if (status != LISTEN) {
					status = DROPPED;
					return -1;
				}
			}
		}


		// See if exceptional condition on any socket
		if (FD_ISSET(tcp_sock, &exceptfds) ||
			((udp_inbound!=-1) && FD_ISSET(udp_inbound, &exceptfds)) ||
			( mcast_recvr && FD_ISSET(mcast_recvr->get_mcast_sock(), &exceptfds) ) ){
    		fprintf(stderr, "vrpn_NetConnection::handle_incoming_messages: Exception on socket\n");
 	    	return(-1);
		}

		// Read incoming messages from the UDP channel
		if ((udp_inbound != -1) && FD_ISSET(udp_inbound,&readfds)) {
			if ( (udp_messages_read = handle_udp_messages(udp_inbound,NULL)) == -1) {
				fprintf(stderr,"vrpn: UDP handling failed, dropping connection\n");
				drop_connection();
				if (status != LISTEN) {
					status = DROPPED;
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
		if ( mcast_capable() ){
			if( FD_ISSET(mcast_recvr->get_mcast_sock(),&readfds)) {
				if ( (mcast_messages_read = handle_mcast_messages(/* XXX */)) == -1) {
					fprintf(stderr,"vrpn: Mcast handling failed, dropping connection\n");
					drop_connection();
					if (status != LISTEN) {
						status = DROPPED;
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
				if (status != LISTEN) {
					status = DROPPED;
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

#ifdef	PRINT_READ_HISTOGRAM
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
	case CONNECTION_FAIL:
	case BROKEN:
      	fprintf(stderr, "vrpn: Socket failure.  Giving up!\n");
		status = DROPPED;
      	return -1;
	case DROPPED:
		break;
	   
	}
	return 0;
	
}
	
  
// pack message into send buffer. buffer contains
// many messages
vrpn_int32 vrpn_NetConnection::pack_message(vrpn_uint32 len, 
											struct timeval time,
											vrpn_int32 type, 
											vrpn_int32 sender, 
											const char * buffer,
											vrpn_uint32 class_of_service,
											vrpn_bool sent_mcast)
{
	vrpn_int32	ret;

	// Make sure type is either a system type (-) or a legal user type
	if ( type >= num_my_types ) {
	    printf("vrpn_NetConnection::pack_message: bad type (%ld)\n",
		type);
	    return -1;
	}

	// If this is not a system message, make sure the sender is legal.
	if (type >= 0) {
	  if ( (sender < 0) || (sender >= num_my_senders) ) {
	    printf("vrpn_NetConnection::pack_message: bad sender (%ld)\n",
		sender);
	    return -1;
	  }
	}

	// Logging must come before filtering and should probably come before
	// any other failure-prone action (such as do_callbacks_for()).  Only
	// semantic checking should precede it.
	
	if (d_logmode & vrpn_LOG_OUTGOING) // FileLogger object exists
		if (logger->log_message(len, time, type, sender, buffer))
			return -1;
	

	// See if there are any local handlers for this message type from
	// this sender.  If so, yank the callbacks.
	if (do_callbacks_for(type, sender, time, len, buffer)) {
		return -1;
	}

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
	    ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_buflen, d_tcp_num_out,
				   len, time, type, sender, buffer);
	    d_tcp_num_out += ret;
	    //	    return -(ret==0);

	    // If the marshalling failed, try clearing the outgoing buffers
	    // by sending the stuff in them to see if this makes enough
	    // room.  If not, we'll have to give up.
	    if (ret == 0) {
		if (send_pending_reports() != 0) { return -1; }
		ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_buflen,
				d_tcp_num_out, len, time, type, sender, buffer);
		d_tcp_num_out += ret;
	    }
	    return (ret==0) ? -1 : 0;
	}

	// Determine the class of service and pass it off to the
	// appropriate service (TCP for reliable, UDP for everything else).
	if (class_of_service & vrpn_CONNECTION_RELIABLE) {
	    ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_buflen, d_tcp_num_out,
				   len, time, type, sender, buffer);
	    d_tcp_num_out += ret;
	    //	    return -(ret==0);

	    // If the marshalling failed, try clearing the outgoing buffers
	    // by sending the stuff in them to see if this makes enough
	    // room.  If not, we'll have to give up.
	    if (ret == 0) {
		if (send_pending_reports() != 0) { return -1; }
		ret = vrpn_marshall_message(d_tcp_outbuf, d_tcp_buflen,
				d_tcp_num_out, len, time, type, sender, buffer);
		d_tcp_num_out += ret;
	    }
	    return (ret==0) ? -1 : 0;
	} else {
	    ret = vrpn_marshall_message(d_udp_outbuf, d_udp_buflen, d_udp_num_out,
				   len, time, type, sender, buffer);
	    d_udp_num_out += ret;
	    //	    return -(ret==0);

	    // If the marshalling failed, try clearing the outgoing buffers
	    // by sending the stuff in them to see if this makes enough
	    // room.  If not, we'll have to give up.
	    if (ret == 0) {
		if (send_pending_reports() != 0) { return -1; }
		ret = vrpn_marshall_message(d_udp_outbuf, d_udp_buflen,
				d_udp_num_out, len, time, type, sender, buffer);
		d_udp_num_out += ret;
	    }
	    return (ret==0) ? -1 : 0;
	}
}


// sends out mesasge buffers over the network
vrpn_int32 vrpn_NetConnection::send_pending_reports(void)
{
   vrpn_int32	ret, sent = 0;

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
#ifdef	VERBOSE
   if (d_tcp_num_out) printf("TCP Need to send %d bytes\n",d_tcp_num_out);
#endif
   while (sent < d_tcp_num_out) {
	   ret = send(tcp_sock, &d_tcp_outbuf[sent],
				  d_tcp_num_out - sent, 0);
#ifdef	VERBOSE
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
#ifdef	VERBOSE
	   printf("UDP Sent %d bytes\n",ret);
#endif 
	   if (ret == -1) {
		   printf("vrpn: UDP send failed\n");
		   drop_connection();
		   return -1;
	   }
   }

   d_tcp_num_out = 0;	// Clear the buffer for the next time
   d_udp_num_out = 0;	// Clear the buffer for the next time
   return 0;
}


//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: protected: send and receive functions
//
//**************************************************************************
//**************************************************************************


// *** N.B. *** 
// it seems to me that the whole message is treated as a single report. i thought
// that it was possible for multiple reports to be in a single
// tcp message. - stefan

// Read all messages available on the given file descriptor (a TCP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.

vrpn_int32 vrpn_NetConnection::handle_tcp_messages (vrpn_int32 fd,
													const struct timeval * timeout)
{	vrpn_int32	sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval localTimeout;
	vrpn_int32	num_messages_read = 0;

#ifdef	VERBOSE2
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
		perror(
		  "vrpn: vrpn_NetConnection::handle_tcp_messages: select failed");
		return(-1);
	    }
	  }

	  // See if exceptional condition on socket
	  if (FD_ISSET(fd, &exceptfds)) {
		fprintf(stderr, "vrpn: vrpn_NetConnection::handle_tcp_messages: Exception on socket\n");
		return(-1);
	  }

	  // If there is anything to read, get the next message
	  if (FD_ISSET(fd, &readfds)) {
		vrpn_int32	header[5];
		struct timeval time;
		vrpn_int32	sender, type;
		vrpn_int32	len, payload_len, ceil_len;
#ifdef	VERBOSE2
	printf("vrpn_NetConnection::handle_tcp_messages() something to read\n");
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
		sender = ntohl(header[3]);
		type = ntohl(header[4]);
#ifdef	VERBOSE2
	printf("  header: Len %d, Sender %d, Type %d\n",
		(int)len, (int)sender, (int)type);
#endif

		// skip up to alignment
		vrpn_int32 header_len = sizeof(header);
		if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
		if (header_len > sizeof(header)) {
		  // the difference can be no larger than this
		  char rgch[vrpn_ALIGN];
		  if (vrpn_noint_block_read(fd,(char*)rgch,header_len-sizeof(header)) !=
		      (vrpn_int32)(header_len-sizeof(header))) {
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
		if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}

		// Make sure the buffer is long enough to hold the whole
		// message body.
		if ((vrpn_int32)d_TCPbuflen < ceil_len) {
		  if (d_TCPbuf) { d_TCPbuf = (char*)realloc(d_TCPbuf,ceil_len); }
		  else { d_TCPbuf = (char*)malloc(ceil_len); }
		  if (d_TCPbuf == NULL) {
		     fprintf(stderr, "vrpn: vrpn_NetConnection::handle_tcp_messages: Out of memory\n");
		     return -1;
		  }
		  d_TCPbuflen = ceil_len;
		}

		// Read the body of the message 
		if (vrpn_noint_block_read(fd,d_TCPbuf,ceil_len) != ceil_len) {
		 perror("vrpn: vrpn_NetConnection::handle_tcp_messages: Can't read body");
		 return -1;
		}

		// Call the handler(s) for this message type
		// If one returns nonzero, return an error.
		if (type >= 0) {	// User handler, map to local id

			
			// *** N.B. ***
			// This is where we'll do our logging/filtering
			//
			// log regardless of whether local id is set,
			// but only process if it has been (ie, log ALL
			// incoming data -- use a filter on the log
			// if you don't want some of it).
			if (d_logmode & vrpn_LOG_INCOMING) {
				if (logger->log_message(payload_len, time,
										translate_remote_type_to_local(type),
										translate_remote_sender_to_local(sender),
										d_TCPbuf)) {
					return -1;
				}
			}
				  
			if (local_type_id(type) >= 0) {
				if (do_callbacks_for(local_type_id(type),
									 local_sender_id(sender),
									 time, payload_len, d_TCPbuf)) {
					return -1;
				}
			}
			
		} else {	// Call system handler if there is one

			if (system_messages[-type] != NULL) {
				
				if (d_logmode & vrpn_LOG_INCOMING)
					if (logger->log_message(payload_len, time, 
											translate_remote_type_to_local(type),
											translate_remote_sender_to_local(sender),
											d_TCPbuf))
						return -1;
				
				// Fill in the parameter to be passed to the routines
				vrpn_HANDLERPARAM p;
				p.type = type;
				p.sender = sender;
				p.msg_time = time;
				p.payload_len = payload_len;
				p.buffer = d_TCPbuf;
				
				if (system_messages[-type](this, p)) {
					fprintf(stderr, 
							"vrpn: vrpn_NetConnection::handle_tcp_messages: Nonzero system handler return\n");
					return -1;
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

vrpn_int32	vrpn_NetConnection::handle_udp_messages (vrpn_int32 fd,
											 const struct timeval * timeout)
{	vrpn_int32	sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval localTimeout;
	vrpn_int32	num_messages_read = 0;
	vrpn_uint32	inbuf_len;
	char	*inbuf_ptr;

#ifdef	VERBOSE2
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
		vrpn_int32	header[5];
		struct timeval	time;
		vrpn_int32	sender, type;
		vrpn_uint32	len, payload_len, ceil_len;

#ifdef	VERBOSE2
	printf("vrpn_NetConnection::handle_udp_messages() something to read\n");
#endif
		// Read the buffer and reset the buffer pointer
		inbuf_ptr = d_UDPinbuf;
		if ( (inbuf_len = recv(fd, d_UDPinbuf, sizeof(d_UDPinbufToAlignRight), 0)) == -1) {
		   perror("vrpn: vrpn_NetConnection::handle_udp_messages: recv() failed");
		   return -1;
		}

	      // Parse each message in the buffer
	      while ( (inbuf_ptr - d_UDPinbuf) != (vrpn_int32)inbuf_len) {

		// Read and parse the header
		// skip up to alignment
		vrpn_uint32 header_len = sizeof(header);
		if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
		
		if ( ((inbuf_ptr - d_UDPinbuf) + header_len) > (vrpn_uint32)inbuf_len) {
		   fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read header");
		   return -1;
		}
		memcpy(header, inbuf_ptr, sizeof(header));
		inbuf_ptr += header_len;
		len = ntohl(header[0]);
		time.tv_sec = ntohl(header[1]);
		time.tv_usec = ntohl(header[2]);
		sender = ntohl(header[3]);
		type = ntohl(header[4]);

#ifdef VERBOSE
		printf("Message type %ld (local type %ld), sender %ld received\n",
			type,local_type_id(type),sender);
#endif
		
		// Figure out how long the message body is, and how long it
		// is including any padding to make sure that it is a
		// multiple of vrpn_ALIGN bytes long.
		payload_len = len - header_len;
		ceil_len = payload_len;
		if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}

		// Make sure we received enough to cover the entire payload
		if ( ((inbuf_ptr - d_UDPinbuf) + ceil_len) > inbuf_len) {
		   fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read payload");
		   return -1;
		}

		// Call the handler for this message type
		// If it returns nonzero, return an error.
		if (type >= 0) {	// User handler, map to local id


			// *** N.B. ***
			// This is where we'll do our logging/filtering
			//
			// log regardless of whether local id is set,
			// but only process if it has been (ie, log ALL
			// incoming data -- use a filter on the log
			// if you don't want some of it).

			if (d_logmode & vrpn_LOG_INCOMING) {
				if (logger->log_message(payload_len, time,
										translate_remote_type_to_local(type),
										translate_remote_sender_to_local(sender),
										inbuf_ptr)) {
					return -1;
				}
			}

			if (local_type_id(type) >= 0) {
				if (do_callbacks_for(local_type_id(type),
									 local_sender_id(sender),
									 time, payload_len, inbuf_ptr)) {
					return -1;
				}
			}
			
		} else {	// System handler
		  
			if (d_logmode & vrpn_LOG_INCOMING)
				if (log_message(payload_len, time,
								translate_remote_type_to_local(type),
								translate_remote_sender_to_local(sender),
								inbuf_ptr))
					return -1;
			
			// Fill in the parameter to be passed to the routines
			vrpn_HANDLERPARAM p;
			p.type = type;
			p.sender = sender;
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
	
	while( (ret_val = mcast_recvr->handle_incoming_message(/* d_MCASTinbuf, ... */)) != -1 ){
	// could be a different return code for no message versus an error

		// parse message into reports
		vrpn_int32	header[5];
		struct timeval	time;
		vrpn_int32	sender, type;
		vrpn_uint32	len, payload_len, ceil_len;

#ifdef	VERBOSE2
	printf("vrpn_NetConnection::handle_mcast_messages() something to read\n");
#endif
		// Reset the buffer pointer
		inbuf_ptr = d_MCASTinbuf;

	    // Parse each report in the buffer
	    while ( (inbuf_ptr - d_MCASTinbuf) != (vrpn_int32)inbuf_len) {

			// Read and parse the header
			// skip up to alignment
			vrpn_uint32 header_len = sizeof(header);
			if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
		
			if ( ((inbuf_ptr - d_MCASTinbuf) + header_len) > (vrpn_uint32)inbuf_len) {
		   		fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read header");
		   		return -1;
			}
			memcpy(header, inbuf_ptr, sizeof(header));
			inbuf_ptr += header_len;
			len = ntohl(header[0]);
			time.tv_sec = ntohl(header[1]);
			time.tv_usec = ntohl(header[2]);
			sender = ntohl(header[3]);
			type = ntohl(header[4]);

#ifdef VERBOSE
		printf("Message type %ld (local type %ld), sender %ld received\n",
			type,local_type_id(type),sender);
#endif
		
			// Figure out how long the message body is, and how long it
			// is including any padding to make sure that it is a
			// multiple of vrpn_ALIGN bytes long.
			payload_len = len - header_len;
			ceil_len = payload_len;
			if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}
	
			// Make sure we received enough to cover the entire payload
			if ( ((inbuf_ptr - d_UDPinbuf) + ceil_len) > inbuf_len) {
			   fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Can't read payload");
			   return -1;
			}

			// Call the handler for this message type
			// If it returns nonzero, return an error.
			if (type >= 0) {	// User handler, map to local id
				
				// *** N.B. ***
				// This is where we'll do our logging/filtering
				//
				// log regardless of whether local id is set,
				// but only process if it has been (ie, log ALL
				// incoming data -- use a filter on the log
				// if you don't want some of it).

                if (d_logmode & vrpn_LOG_INCOMING) {
					if (log_message(payload_len, time,
									translate_remote_type_to_local(type),
									translate_remote_sender_to_local(sender),
									inbuf_ptr)) {
                 		return -1;
                    }
                }
                
				if (local_type_id(type) >= 0) {
			    	if (do_callbacks_for(local_type_id(type),
								         local_sender_id(sender),
				    				     time, payload_len, inbuf_ptr)) {
				      	return -1;
					}
		  		}
		  		
			}
			else {	// System handler
				
                if (d_logmode & vrpn_LOG_INCOMING) {
					if (log_message(payload_len, time,
									translate_remote_type_to_local(type),
									translate_remote_sender_to_local(sender),
									inbuf_ptr, 1)) {
                 		return -1;
                    }
                }

			  	// Fill in the parameter to be passed to the routines
			  	vrpn_HANDLERPARAM p;
			  	p.type = type;
		 		p.sender = sender;
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

	if( ret_val = -1 )
		return ret_val;
	else
		return num_messages_read;
}

//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: public: connection setup: server side
//
//**************************************************************************
//**************************************************************************


vrpn_int32 vrpn_NetConnection::connect_to_client (const char *machine, vrpn_int16 port)
{
    char msg[100];
    if (status != LISTEN) return -1;
    sprintf(msg, "%s %d", machine, port);
    printf("vrpn: Connection request received: %s\n", msg);
    tcp_sock = connect_tcp_to(msg);
    if (tcp_sock != -1) {
		status = CONNECTED;
    }
    if (status != CONNECTED) {   // Something broke
		return -1;
    }
    else
		handle_connection();
    return 0;
}

//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: protected: connection setup: server side
//
//**************************************************************************
//**************************************************************************

//  This routine opens a TCP socket and connects it to the machine and port
// that are passed in the msg parameter.  This is a string that contains
// the machine name, a space, then the port number.
//  The routine returns -1 on failure and the file descriptor on success.

vrpn_int32 vrpn_NetConnection::connect_tcp_to (const char * msg)
{	char	machine [5000];
	vrpn_int32	port;
#ifndef _WIN32
	vrpn_int32	server_sock;		/* Socket on this host */
#else
	SOCKET server_sock;
#endif
    struct	sockaddr_in client;     /* The name of the client */
    struct	hostent *host;          /* The host to connect to */

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
#else	// not _WIN32

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
		if (addr == INADDR_NONE) {	// that didn't work either
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

	{	struct	protoent	*p_entry;
		vrpn_int32	nonzero = 1;

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
//		if (setsockopt(server_sock, p_entry->p_proto,
//			TCP_NODELAY, (const char *)&nonzero, sizeof(nonzero))==SOCKET_ERROR) {
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
static	SOCKET connect_udp_to (char * machine, vrpn_int16 portno)
{
   SOCKET sock; 
#else
static	vrpn_int32 connect_udp_to (char * machine, vrpn_int16 portno)
{
   vrpn_int32 sock;
#endif
   struct	sockaddr_in client;     /* The name of the client */
   struct	hostent *host;          /* The host to connect to */

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
vrpn_int32 NetConnection::handle_mcast_reply(){

	// unpack message

	// set local data member
	set_peer_mcast_capable(/* true/false */);

	// inform ServerConnectionController whether
	// client is mcast capable

}

// get mcast info from mcast sender and pack into message
vrpn_int32 NetConnection::pack_mcast_description(vrpn_int32 sender){

	vrpn_uint32 pid;
	vrpn_uint32 ip;
	char temp_buf[20];
	struct timeval now;
	char ** insertPt;
	vrpn_int32 buflen = sizeof(d_mcast_cookie);

	gettimeofday(&now, NULL);

	// pack magic mcast cookie
	// first ip addr as 32 bit u_int
	// then pid as 32 bit u_int
	vrpn_getmyIP(temp_buf,sizeof(temp_buf));
	ip = inet_addr(temp_buf);
	pid = getpid();
	insertPt = d_mcast_cookie;
	vrpn_buffer(insertPt,&buflen,ip);
	vrpn_buffer(insertPt,&buflen,pid);
	

	return pack_message((vrpn_uint32)sizeof(McastGroupDescrp), 
						now, vrpn_CONNECTION_MCAST_DESCRIPTION,
						sender, controller->get_mcast_info(),
						vrpn_CONNECTION_RELIABLE, vrpn_false);
}


//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: public: connection setup: client side
//
//**************************************************************************
//**************************************************************************


// make connection to server
// this is just a rough outline
vrpn_int32 vrpn_NetConnection::connect_to_server(const char* machine, 
												 vrpn_int16 port){
	vrpn_int32 retval = 0;

	// initiate connection by sending message to well
	// known port on server
	retval = udp_request_connection_call(machine,port);
	if( retval == -1 )
		return retval;
	else
		tcp_sock = retval;

	// setup rest of connection
	handle_connection();
	return retval;
}


//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: protected: connection setup: client side
//
//**************************************************************************
//**************************************************************************

/***************************
 *	This routine will send UDP packets requesting a TCP callback from
 * a remote server given the name of the machine and the socket number to
 * send the requests to.
 *	This routine returns the file descriptor of the socket on success
 * and -1 on failure.
 ***************************/
vrpn_int32 vrpn_NetConnection::udp_request_connection_call(const char * machine, int port)
{
	struct sockaddr_in listen_name;	/* The listen socket binding name */
	vrpn_int32	listen_namelen;
#ifdef	WIN32
	SOCKET	listen_sock;		/* The socket to listen on */
	SOCKET	accept_sock;		/* The socket we get when accepting */
	SOCKET	udp_sock;		/* We lob datagrams from here */
#else
	vrpn_int32	listen_sock;		/* The socket to listen on */
	vrpn_int32	accept_sock;		/* The socket we get when accepting */
	vrpn_int32	udp_sock;		/* We lob datagrams from here */
#endif
	struct sockaddr_in udp_name;	/* The UDP socket binding name */
	vrpn_int32	udp_namelen;
	vrpn_int32	listen_portnum;		/* Port number we're listening on */
        struct	hostent *host;          /* The host to connect to */
	char	msg[150];		/* Message to send */
	vrpn_int32	msglen;		/* How long it is (including \0) */
	char	myIPchar[100];		/* IP decription this host */
	vrpn_int32	try_connect;

	/* Create a TCP socket to listen for incoming connections from the
	 * remote server. */

        listen_name.sin_family = AF_INET;		/* Internet socket */
        listen_name.sin_addr.s_addr = INADDR_ANY;	/* Accept any port */
        listen_name.sin_port = htons(0);
        listen_sock = socket(AF_INET,SOCK_STREAM,0);
		listen_namelen = sizeof(listen_name);
        if (listen_sock < 0) {
		fprintf(stderr,"vrpn_NetConnection::udp_request_connection_call: can't open socket().\n");
                return(-1);
        }
        if ( bind(listen_sock,(struct sockaddr*)&listen_name,listen_namelen) ) {
		fprintf(stderr,"vrpn_NetCOnnection::udp_request_connecton_call: can't bind socket().\n");
                close(listen_sock);
                return(-1);
        }
        if (getsockname(listen_sock,
	                (struct sockaddr *) &listen_name,
                        GSN_CAST &listen_namelen)) {
		fprintf(stderr,
			"vrpn_NetCOnnection::udp_request_connecton_call: cannot get socket name.\n");
                close(listen_sock);
                return(-1);
        }
	listen_portnum = ntohs(listen_name.sin_port);
	if ( listen(listen_sock,2) ) {
		fprintf(stderr,"vrpn_NetCOnnection::udp_request_connecton_call: listen() failed.\n");
		close(listen_sock);
		return(-1);
	}

	/* Create a UDP socket and connect it to the port on the remote
	 * machine. */

        udp_sock = socket(AF_INET,SOCK_DGRAM,0);
        if (udp_sock < 0) {
	    fprintf(stderr,"vrpn_NetCOnnection::udp_request_connecton_call: can't open udp socket().\n");
	    close(listen_sock);
            return(-1);
        }
	udp_name.sin_family = AF_INET;
	udp_namelen = sizeof(udp_name);
	host = gethostbyname(machine);
        if (host) {

#ifdef CRAY

          { vrpn_int32 i;
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

          vrpn_int32 retval;
          retval = sscanf(machine, "%u.%u.%u.%u",
                          ((char *) &udp_name.sin_addr.s_addr)[0],
                          ((char *) &udp_name.sin_addr.s_addr)[1],
                          ((char *) &udp_name.sin_addr.s_addr)[2],
                          ((char *) &udp_name.sin_addr.s_addr)[3]);

          if (retval != 4) {

#endif  // _WIN32

// Note that this is the failure clause of gethostbyname() on
// non-WIN32 systems, but of the sscanf() on WIN32 systems.

		close(listen_sock);
		fprintf(stderr,
			"vrpn_NetConnection::udp_request_connecton_call: error finding host by name\n");
		return(-1);

#ifdef _WIN32

          }

#endif

        }

#ifndef _WIN32
	udp_name.sin_port = htons(port);
#else
	udp_name.sin_port = htons((u_short)port);
#endif
        if ( connect(udp_sock,(struct sockaddr*)&udp_name,udp_namelen) ) {
	    fprintf(stderr,
				"vrpn_NetConnection::udp_request_connecton_call: can't bind udp socket().\n");
	    close(listen_sock);
	    close(udp_sock);
	    return(-1);
        }

	/* Fill in the request message, telling the machine and port that
	 * the remote server should connect to.  These are ASCII, separated
	 * by a space. */

	if (vrpn_getmyIP(myIPchar, sizeof(myIPchar))) {
		fprintf(stderr,
		   "vrpn_NetConnection::udp_request_connecton_call: Error finding local hostIP\n");
		close(listen_sock);
		close(udp_sock);
		return(-1);
	}
	sprintf(msg, "%s %d", myIPchar, listen_portnum);
	msglen = strlen(msg) + 1;	/* Include the terminating 0 char */

	/* Repeat sending the request to the server and checking for it
	 * to call back until either there is a connection request from the
	 * server or it times out as many times as we're supposed to try. */

	for (try_connect = 0; try_connect < UDP_CALL_RETRIES; try_connect++) {
		fd_set	rfds;
		struct	timeval t;

		/* Send a packet to the server asking for a connection. */
		if (send(udp_sock, msg, msglen, 0) == -1) {
		  perror("vrpn_NetConnection::udp_request_connecton_call: send() failed");
		  close(listen_sock);
		  close(udp_sock);
		  return -1;
		}

		/* See if we get a connection attempt within the timeout */
		FD_ZERO(&rfds);
		FD_SET(listen_sock, &rfds);	/* Check for read (connect) */
		t.tv_sec = UDP_CALL_TIMEOUT;
		t.tv_usec = 0;
		if (select(32, &rfds, NULL, NULL, &t) == -1) {
		  perror("vrpn_NetConnection::udp_request_connection_call: select() failed");
		  close(listen_sock);
		  close(udp_sock);
		  return -1;
		}
		if (FD_ISSET(listen_sock, &rfds)) {	/* Got one! */
			break;	/* Break out of the for loop */
		}
	}
	if (try_connect == UDP_CALL_RETRIES) {	/* We didn't get an answer */
		fprintf(stderr,"vrpn_NetConnection::udp_request_connection_call: No reply from server\n");
		fprintf(stderr,"                      (Server down or busy)\n");
		close(listen_sock);
		close(udp_sock);
		return -1;
	}

	/* Accept the connection from the remote machine and set TCP_NODELAY
	 * on the socket. */
	if ( (accept_sock = accept(listen_sock,0,0)) == -1 ) {
		perror("vrpn_NetConnection::udp_request_connection_call: accept() failed");
		close(listen_sock);
		close(udp_sock);
		return -1;
	}

	{	struct	protoent	*p_entry;
		vrpn_int32	nonzero = 1;

		if ( (p_entry = getprotobyname("TCP")) == NULL ) {
			fprintf(stderr,
			  "vrpn_NetConnection::udp_request_connection_call: getprotobyname() failed.\n");
			close(accept_sock);
			close(listen_sock);
			close(udp_sock);
			return(-1);
		}

		if (setsockopt(accept_sock, p_entry->p_proto,
		    TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
			perror("vrpn_NetConnection::udp_request_connection_call: setsockopt() failed");
			close(accept_sock);
			close(listen_sock);
			close(udp_sock);
			return(-1);
		}
	}

	close(listen_sock);	/* Don't need this now */
	close(udp_sock);	/* Don't need this now */

	return accept_sock;
}



//---------------------------------------------------------------------------
// This routine opens a UDP socket with the requested port number.
// The socket is not bound to any particular outgoing address.
// The routine returns -1 on failure and the file descriptor on success.
// The portno parameter is filled in with the actual port that is opened
// (this is useful when the address INADDR_ANY is used, since it tells
// what port was opened).

static	vrn_int32 open_udp_socket (vrpn_uint16 * portno)
{
   vrpn_int32 sock;
   struct sockaddr_in server_addr;
   struct sockaddr_in udp_name;
   vrpn_int32	udp_namelen = sizeof(udp_name);

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)  {
	perror("vrpn: vrpn_NetConnection: can't open socket");
	return -1;
   }

   // bind to local address
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(*portno);
   server_addr.sin_addr.s_addr = htonl((u_long)0); // don't care which client
   if (bind(sock, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0){
	perror("vrpn: vrpn_NetConnection: can't bind address");
	return -1;
   }

   // Find out which port was actually bound
   if (getsockname(sock, (struct sockaddr *) &udp_name,
                   GSN_CAST &udp_namelen)) {
	fprintf(stderr, "vrpn: open_udp_socket: cannot get socket name.\n");
	return -1;
   }
   *portno = ntohs(udp_name.sin_port);

   return sock;
}


// respond to system message vrpn_CONNECTION_MCAST_DESCRIPTION
// if mcast capable, create a mcast recvr w/ info from server
// then call pack_mcast_reply
vrpn_int32 NetConnection::handle_mcast_description(char* message){
	
	if( message ){ // if message is not NULL i.e. server is mcast capable
		McastGroupDescrp descrp = new McastGroupDescrp();
		memcpy(descrp,message,sizeof(McastGroupDescrp));
		mcast_recvr = new UnreliableMulticastRecvr(descrp->d_mcast_group,descrp->d_mcast_port);
	

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
vrpn_int32 NetConnection::pack_mcast_reply(){

	// pack value of d_mcast_capable into buffer
}

//**************************************************************************
//**************************************************************************
//
// vrpn_NetConnection: public: connection setup: common functions
//
//**************************************************************************
//**************************************************************************

// i think that these are common functons. not sure yet though

// called when new client is trying to connect
void vrpn_NetConnection::handle_connection(void)
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
		status = BROKEN;
		return;
	}

	if (setsockopt(tcp_sock, p_entry->p_proto,
		TCP_NODELAY, &nonzero, sizeof(nonzero))==-1) {
		perror("vrpn: vrpn_Connection: setsockopt() failed");
		close(tcp_sock); tcp_sock = -1;
		status = BROKEN;
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

  // Message needs to be dispatched *locally only*, so we do_callbacks_for()
  // and never pack_message()
  struct timeval now;
  gettimeofday(&now, NULL);

  if (!num_live_connections) {
    do_callbacks_for(register_message_type(vrpn_got_first_connection),
                     register_sender(vrpn_CONTROL),
                     now, 0, NULL);
  }

  do_callbacks_for(register_message_type(vrpn_got_connection),
                   register_sender(vrpn_CONTROL),
                   now, 0, NULL);

  num_live_connections++;
}

// network initialization
// i think this is called by both the client and server sides
// i could be wrong though. it handles aspects of a new connection
// like checking version numbers, telling the other side what udp
// port to send to, and transfering sender and types to the other
// side
int vrpn_NetConnection::setup_new_connection
         (vrpn_int32 remote_log_mode, const char * remote_logfile_name)
{
	char	sendbuf [501];  // HACK
	char	recvbuf [501];  // HACK
	vrpn_int32	received_logmode;
	vrpn_int32	sendlen;
	vrpn_int32	i;
	vrpn_int retval;
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
  if (retval < 0)
    return -1;

	// Find out what log mode they want us to be in BEFORE we pack
	// type, sender, and udp descriptions!  If it's nonzero, the
	// filename to use should come in a log_description message later.

	received_logmode = recvbuf[vrpn_MAGICLEN + 2] - '0';
	if ((received_logmode < 0) ||
            (received_logmode > (vrpn_LOG_INCOMING | vrpn_LOG_OUTGOING))) {
	  fprintf(stderr, "vrpn_Connection::setup_new_connection:  "
                          "Got invalid log mode %d\n", received_logmode);
          return -1;
        }
	d_logmode |= received_logmode;

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
	  if ((udp_inbound = open_udp_socket(&udp_portnum)) == -1)  {
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

	// Pack messages that describe the types of messages and sender
	// ID mappings that have been described to this connection.  These
	// messages use special IDs (negative ones).
	for (i = 0; i < num_my_senders; i++) {
		pack_sender_description(i);
	}
	for (i = 0; i < num_my_types; i++) {
		pack_type_description(i);
	}

	// pack multicast capability descriptons

	// Send the messages
	if (send_pending_reports() == -1) {
	  fprintf(stderr,
	    "vrpn_Connection::setup_new_connection: Can't send UDP msg\n");
	  return -1;
	}

	return 0;
}

// drop a connection with another machine
void vrpn_NetConnection::drop_connection(void)
{
	//XXX Store name when opening, say which dropped here
	printf("vrpn: Connection dropped\n");
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
	if (listen_udp_sock != INVALID_SOCKET) {
		status = LISTEN;	// We're able to accept more
	} else {
		status = BROKEN;	// We're unable to accept more
	}

  num_live_connections--;

  // Message needs to be dispatched *locally only*, so we do_callbacks_for()
  // and never pack_message()
  struct timeval now;
  gettimeofday(&now, NULL);

  do_callbacks_for(register_message_type(vrpn_dropped_connection),
                   register_sender(vrpn_CONTROL),
                   now, 0, NULL);

  if (!num_live_connections) {
    do_callbacks_for(register_message_type(vrpn_dropped_last_connection),
                     register_sender(vrpn_CONTROL),
                     now, 0, NULL);
  }

  if (d_logname) {
      close_log();
  }

  if( mcast_capable() ){
	  delete mcast_recvr;
  }

}

// pack type description into a message buffer. used in the 
// initialization of a connection when the two sides exchange
// sender and type information
vrpn_int32 vrpn_NetConnection::pack_type_description(vrpn_int32 which)
{
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32	len = strlen(my_types[which].name) + 1;
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_TYPE_DESCRIPTION
   // whose sender ID is the ID of the type that is being
   // described and whose body contains the length of the name
   // and then the name of the type.

#ifdef	VERBOSE
   printf("  vrpn_NetConnection: Packing type '%s'\n",my_types[which].name);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   memcpy(&buffer[sizeof(len)], my_types[which].name, (vrpn_int32) len);
   gettimeofday(&now,NULL);

   return pack_message((vrpn_uint32) (len + sizeof(len)), now,
   	vrpn_CONNECTION_TYPE_DESCRIPTION, which, buffer,
	vrpn_CONNECTION_RELIABLE, vrpn_false);
}


// pack sender description into a message buffer. used in the 
// initialization of a connection when the two sides exchange
// sender and type information
vrpn_int32 vrpn_NetConnection::pack_sender_description(vrpn_int32 which)
{
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32	len = strlen(my_senders[which]) + 1;
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_SENDER_DESCRIPTION
   // whose sender ID is the ID of the sender that is being
   // described and whose body contains the length of the name
   // and then the name of the sender.

#ifdef	VERBOSE
	printf("  vrpn_NetConnection: Packing sender '%s'\n", my_senders[which]);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   memcpy(&buffer[sizeof(len)], my_senders[which], (vrpn_int32) len);
   gettimeofday(&now,NULL);

   return pack_message((vrpn_uint32)(len + sizeof(len)), now,
   	vrpn_CONNECTION_SENDER_DESCRIPTION, which, buffer,
	vrpn_CONNECTION_RELIABLE,vrpn_false);
}


// pack udp description into a message buffer. used in the 
// initialization of a connection when the two sides exchange
// what upd port the other side should send messages to
vrpn_int32 vrpn_NetConnection::pack_udp_description(vrpn_int32 portno)
{
   struct timeval now;
   vrpn_uint32	portparam = portno;
   char myIPchar[1000];

   // Find the local host name
   if (vrpn_getmyIP(myIPchar, sizeof(myIPchar))) {
	perror("vrpn_NetConnection::pack_udp_description: can't get host name");
	return -1;
   }

   // Pack a message with type vrpn_CONNECTION_UDP_DESCRIPTION
   // whose sender ID is the ID of the port that is to be
   // used and whose body holds the zero-terminated string
   // name of the host to contact.

#ifdef	VERBOSE
	printf("  vrpn_NetConnection: Packing UDP %s:%d\n", myIPchar,
		portno);
#endif
   gettimeofday(&now, NULL);

   return pack_message(strlen(myIPchar) + 1, now,
        vrpn_CONNECTION_UDP_DESCRIPTION,
	portparam, myIPchar, vrpn_CONNECTION_RELIABLE, vrpn_false);
}


// pack log description into a message buffer. used in the 
// initialization of a connection when the two sides exchange
// what type of logging to do. Not sure if this will maek it
// into the final wersion.
vrpn_int32 vrpn_NetConnection::pack_log_description (vrpn_int32 logmode,
                                           const char * logfile) {
  struct timeval now;

  // Pack a message with type vrpn_CONNECTION_LOG_DESCRIPTION whose
  // sender ID is the logging mode to be used by the remote connection
  // and whose body holds the zero-terminated string name of the file
  // to write to.

  gettimeofday(&now, NULL);
  return pack_message(strlen(logfile) + 1, now,
                      vrpn_CONNECTION_LOG_DESCRIPTION, logmode, logfile,
                      vrpn_CONNECTION_RELIABLE, vrpn_false);
}
