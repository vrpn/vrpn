
#include "vrpn_UnreliableMulticastSender.h"

//==========================================================================
//
// class vrpn_UnreliableMulticastSender
//
// this class is responsible for setting up the sending side of the
// multicast socket and for sending multicast messages through this socket
//
//==========================================================================

//**************************************************************************
//**************************************************************************
//
// vrpn_UnreliableMulticastSender: public functions
//
//**************************************************************************
//**************************************************************************

//-------------------------------------------------------------------
// constructors and destuctors
//-------------------------------------------------------------------
vrpn_UnreliableMulticastSender::vrpn_UnreliableMulticastSender(
char* group_name, vrpn_uint16 port, vrpn_int32 ttl)
  :vrpn_BaseMulticastChannel(group_name,port),d_mcast_ttl(ttl),d_mcast_num_out(0){};

vrpn_UnreliableMulticastSender::~vrpn_UnreliableMulticastSender(void){};

//-------------------------------------------------------------------
// get functions
//-------------------------------------------------------------------


// pack all relevant info about multicast group into a McastGroupDescrp class
// the McastGroupDescrp class then gets packed into a char buffer so it can
// be passed over the network so the client knows which port and group to 
// listen to. 
vrpn_int32 vrpn_UnreliableMulticastSender::get_mcast_info(char *info_buffer){

  vrpn_McastGroupDescrp *descrp;
  descrp = new vrpn_McastGroupDescrp();
  if( descrp == NULL )
	return -1;

  strcpy(descrp->d_mcast_group,this->get_mcast_group_name());
  descrp->d_mcast_addr = this->get_mcast_addr();
  descrp->d_mcast_port = this->get_mcast_port_num();
  descrp->d_mcast_ttl = this->get_mcast_ttl();
  memcpy(info_buffer,descrp,sizeof(vrpn_McastGroupDescrp));

  return 0;
}


// get time-to-live for multicast packets
vrpn_int32 vrpn_UnreliableMulticastSender::get_mcast_ttl(void){
  return d_mcast_ttl;
}


//-------------------------------------------------------------------
// set functions
//-------------------------------------------------------------------

// set time-to-live for multicast packets
// 0 = restricted to the same host 
// 1 = restricted to the same subnet 
// 32 = restricted to the same site 
// 64 = restricted to the same region 
// 128 = restricted to the same continent 
// 255 = unrestricted 
vrpn_int32 vrpn_UnreliableMulticastSender::set_mcast_ttl(vrpn_int32 ttl){
  d_mcast_ttl = ttl;

  if( setsockopt(get_mcast_sock(),IPPROTO_IP,IP_MULTICAST_TTL,(char *) &ttl,
  		sizeof(vrpn_int32))== -1) {
    return -1;
  }

  return 0;
}


// sets whether the sender receives multicast messages it sends
// default is that messages loopback to senders interface
// loopback_on is 0 or 1.
// 1 = loopback is on
// 0 = loopback off
vrpn_int32 vrpn_UnreliableMulticastSender::set_mcast_loopback(vrpn_int32 loopback_on){

	if( setsockopt(get_mcast_sock(),IPPROTO_IP,IP_MULTICAST_LOOP,
		(char *) &loopback_on, sizeof(vrpn_int32)) == -1 ){
    	return -1;
    }

    return 0;
}


//-------------------------------------------------------------------
// send functions
//-------------------------------------------------------------------

// send a message out the multicast channel
// might not be in final version if we go with current model of packing
// multiple messages into a singe buffer.
vrpn_int32 vrpn_UnreliableMulticastSender::send_message(char* message){

	 if (sendto(get_mcast_sock(),message,sizeof(message),0,
	     (struct sockaddr *) &d_mcast_addr,sizeof(d_mcast_addr)) < 0) {
    	return -1;
  	}

	return 0;
}



// marshall a message into a char buffer for transmission across the network.
// calls vrpn_marshall_message() which was part of vrpn_Connection but got moved
// into a global function because this class needed access to it
vrpn_int32 vrpn_UnreliableMulticastSender::marshall_message(vrpn_uint32 len, 
															timeval time,
															vrpn_int32 type, 
															vrpn_int32 sender, 
															const char* buffer){
  vrpn_marshall_message(d_mcast_outbuf,sizeof(d_mcast_outbuf),d_mcast_num_out,len,time,type,sender,buffer);
}



// send current multicast message buffer out onto the network
// many reports are packed into the same multicast message 
vrpn_int32 vrpn_UnreliableMulticastSender::send_pending_reports(void) {

   vrpn_int32 ret, sent = 0;

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
   FD_SET (get_mcast_sock(), &f);

   vrpn_int32 connection = select(32, NULL, NULL, &f, &timeout);
   if (connection != 0) {
	 drop_connection();
	 return -1;
   }

   // Send all of the messages that have built
   // up in the Multicast buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
   if (d_mcast_num_out > 0) {
	 ret = send(get_mcast_sock(), d_mcast_outbuf, d_mcast_num_out, 0);
#ifdef	VERBOSE
	 printf("Mcast Sent %d bytes\n",ret);
#endif 
	 if (ret == -1) {
	   printf("vrpn: Mcast send failed\n");
	   drop_connection(); // XXX - what to do if send fails?
	   return -1;
	 }
   }

   d_mcast_num_out = 0;	// Clear the buffer for the next time
   return 0;
}


//**************************************************************************
//**************************************************************************
//
// vrpn_UnreliableMulticastSender: protected functions
//
//**************************************************************************
//**************************************************************************

// initialize multicast socket
// only called by constructor
void vrpn_UnreliableMulticastSender::init_mcast_channel(void){

	vrpn_int32 no_loopback = 0;
	
  	// create what looks like an ordinary UDP socket
  	if (set_mcast_sock(socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    	perror("error: socket failure in multicast sender");
    	exit(1);
  	}
  
  	// set up destination address 
  	memset(&d_mcast_addr,0,sizeof(d_mcast_addr));
  	d_mcast_addr.sin_family=AF_INET;
  	d_mcast_addr.sin_addr.s_addr=inet_addr(get_mcast_group_name());
  	d_mcast_addr.sin_port=htons(get_mcast_port_num());

	if( this->set_mcast_loopback(no_loopback) < 0 ){
		perror("error: set loopback failed in mcast sender init");
		exit(1);
	}
	
}


