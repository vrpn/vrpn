#include "vrpn_UnreliableMulticastRecvr.h"


//=========================================================================
//
// class vrpn_UnreliableMulticasrRecvr
//
// this class is responsible for receiving multicast messages
//
//==========================================================================

//**************************************************************************
//**************************************************************************
//
// vrpn_UnreliableMulticastRecvr: public functions
//
//**************************************************************************
//**************************************************************************

//-------------------------------------------------------------------
// constructors and destructors
//-------------------------------------------------------------------
vrpn_UnreliableMulticastRecvr::vrpn_UnreliableMulticastRecvr( char* group_name,
									  vrpn_uint16 port)
	:vrpn_BaseMulticastChannel(group_name,port){
	init_mcast_channel();
}
		
vrpn_UnreliableMulticastRecvr::~vrpn_UnreliableMulticastRecvr(){}


//-----------------------------------------------------------------------
// get functions
//------------------------------------------------------------------------

// pack all relevant info about multicast group into a McastGroupDescrp class
// the McastGroupDescrp class then gets packed into a char buffer so it can
// be passed over the network so the client knows which port and group to 
// listen to. 
vrpn_int32 vrpn_UnreliableMulticastRecvr::get_mcast_info(char *info_buffer){

  vrpn_McastGroupDescrp *descrp;
  descrp = new vrpn_McastGroupDescrp();
  if( descrp == NULL )
	return -1;

  strcpy(descrp->d_mcast_group,this->get_mcast_group_name());
  descrp->d_mcast_addr = this->get_mcast_addr();
  descrp->d_mcast_port = this->get_mcast_port_num();
  descrp->d_mcast_ttl = 0;
  memcpy(info_buffer,descrp,sizeof(vrpn_McastGroupDescrp));

  return 0;
}


//-------------------------------------------------------------------
// receive messages
//-------------------------------------------------------------------

// read one incoming message from multicast channel
// return number of bytes read, 0 for no message there, or -1 for error
vrpn_int32 vrpn_UnreliableMulticastRecvr::handle_incoming_message(char* inbuf_ptr, const struct timeval* timeout){

    vrpn_int32	sel_ret;
    fd_set  readfds, exceptfds;
	struct  timeval localTimeout;
	vrpn_uint32	inbuf_len;

#ifdef	VERBOSE2
	printf("vrpn_UnreliabelMulticastRecvr::handle_incoming_message() called\n");
#endif

	if (timeout) {
		localTimeout.tv_sec = timeout->tv_sec;
		localTimeout.tv_usec = timeout->tv_usec;
    }
    else {
		localTimeout.tv_sec = 0L;
		localTimeout.tv_usec = 0L;
    }

	// Read one incoming packet
	// Each packet may have more than one
	// message in it. Pass packet back to caller

	// Select to see if ready to hear from server, or exception
	FD_ZERO(&readfds);              /* Clear the descriptor sets */
	FD_ZERO(&exceptfds);
	FD_SET(get_mcast_sock(), &readfds);     /* Check for read */
	FD_SET(get_mcast_sock(), &exceptfds);   /* Check for exceptions */
	sel_ret = select(32, &readfds, NULL, &exceptfds, &localTimeout);
	if (sel_ret == -1) {
		if (errno == EINTR) { /* Ignore interrupt */
			//continue;
		}
		else {
			perror("vrpn: vrpn_NetConnection::handle_udp_messages: select failed()");
			return(-1);
	    }
	}

	// See if exceptional condition on socket
	if (FD_ISSET(get_mcast_sock(), &exceptfds)) {
		fprintf(stderr, "vrpn: vrpn_NetConnection::handle_udp_messages: Exception on socket\n");
		return(-1);
	}

	// If there is anything to read, get the next message
	if (FD_ISSET(get_mcast_sock(), &readfds)) {
	
#ifdef	VERBOSE2
	printf("vrpn_UnreliableMulticastRecvr::handle_incoming_message() something to read\n");
#endif
		// Read the buffer
		inbuf_ptr = d_mcast_inbuf;
		if ( (inbuf_len = recv(get_mcast_sock(), d_mcast_inbuf, sizeof(d_MCASTinbufToAlignRight), 0)) == -1) {
		   perror("vrpn: vrpn_UnreliableMulticastrecvr::handle_incoming_message: recv() failed");
		   return -1;
		}

		return inbuf_len;
	}

	// no message to read
	return 0;
	
}

//--------------------------------------------------------------
// change group membership functions
//--------------------------------------------------------------

// join a multicast group.
// group name is a string in dotted decimal form
vrpn_int32 vrpn_UnreliableMulticastRecvr::join_mcast_group(char* group){

	struct sockaddr_in groupStruct;
	struct ip_mreq mreq;  // multicast group info structure

	if((groupStruct.sin_addr.s_addr = inet_addr(group))== -1){
		set_created_correctly(vrpn_false);
		return -1;
	}
	// check if group address is indeed a Class D address
	mreq.imr_multiaddr = groupStruct.sin_addr;
	mreq.imr_interface.s_addr = INADDR_ANY;

	if ( setsockopt(get_mcast_sock(),IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *) &mreq,
		sizeof(mreq)) == -1 ){
		set_created_correctly(vrpn_false);
		return -1;
	}
		
	return 0;
}
	

// leave a multicast group.
// group name is a string in dotted decimal form
vrpn_int32 vrpn_UnreliableMulticastRecvr::leave_mcast_group(char* group){

	struct sockaddr_in groupStruct;
	struct ip_mreq mreq;  // multicast group info structure

	if((groupStruct.sin_addr.s_addr = inet_addr(group))== -1) return -1;

	// check if group address is indeed a Class D address
	mreq.imr_multiaddr = groupStruct.sin_addr;
	mreq.imr_interface.s_addr = INADDR_ANY;

	if ( setsockopt(get_mcast_sock(),IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *) &mreq,
		sizeof(mreq)) == -1 ){
		set_created_correctly(vrpn_false);
		return -1;
	}
		
	return 0;
}
	

//**************************************************************************
//**************************************************************************
//
// vrpn_UnreliableMulticastRecvr: protected functions
//
//**************************************************************************
//**************************************************************************

// initialize multicast channel.
// only called by constructor
void vrpn_UnreliableMulticastRecvr::init_mcast_channel(){
	
  	vrpn_int32 one = 1; // used in first setsockopt, but not sure why

  	// create what looks like an ordinary UDP socket 
  	if (set_mcast_sock(socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("error:failed to create multicast recvr socket");
		set_created_correctly(vrpn_false);
  	}
  
  	// set up destination address
  	memset(&d_mcast_addr,0,sizeof(d_mcast_addr));
  	d_mcast_addr.sin_family=AF_INET;
  	d_mcast_addr.sin_addr.s_addr=htonl(INADDR_ANY); // N.B.: differs from sender
  	d_mcast_addr.sin_port=htons(get_mcast_port_num());

#ifdef WIN32
  	// set up socket so local addr can be reused
	// not sure why you want to do this 
  	if (setsockopt(get_mcast_sock(),SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one)) < 0) {
		perror("error: setsockopt failed in multicast recvr init");
		set_created_correctly(vrpn_false);
  	}
#else
  	// set up socket so multiple sockets can bind to the same port
	// useful for big machines like evans.cs.unc.edu, 
	// where there might be multiple clients 
  	if (setsockopt(get_mcast_sock(),SOL_SOCKET,SO_REUSEPORT,&one,sizeof(one)) < 0) {
		perror("error: setsockopt failed in multicast recvr init");
		set_created_correctly(vrpn_false);
  	}
#endif

	// bind to receive address 
  	if (bind(get_mcast_sock(),(struct sockaddr *) &d_mcast_addr,
		 sizeof(d_mcast_addr)) < 0) {
		perror("error: bind failed in multicast receiver init");
		set_created_correctly(vrpn_false);
  	}
  
	// Find out which port was actually bound
	vrpn_int32 addr_len = sizeof(d_mcast_addr);
	if (getsockname(get_mcast_sock(), (struct sockaddr *) &d_mcast_addr,(int *) &addr_len)) {
		fprintf(stderr, "vrpn: vrpn_UnreliableMulticastSender: cannot get socket name.\n");
		set_created_correctly(vrpn_false);
	}
	
	// if port actually bound different from requested
	// set created_correctly to false
	if( get_mcast_port_num() != ntohs(d_mcast_addr.sin_port) ){
		perror("error: failed to get requested port");
		set_created_correctly(vrpn_false);
	}

	if( this->join_mcast_group(get_mcast_group_name()) < 0 ){
		perror("error: failed to join multicast group");
		set_created_correctly(vrpn_false);
	}

}







