
#include "vrpn_BaseMulticast.h"

//==========================================================================
//
// class vrpn_UnreliableMulticastSender
//
// this class is responsible for setting up the sending side of the
// multicast socket and for sending multicast messages through this socket
//
//==========================================================================
class vrpn_UnreliableMulticastSender: public vrpn_BaseMulticastChannel {

public:

  //-------------------------------------------------------------------
  // constructors and destuctors
  //-------------------------------------------------------------------
  vrpn_UnreliableMulticastSender( char* group_name, vrpn_uint16 port,
				       vrpn_int32 ttl = 32);
  ~vrpn_UnreliableMulticastSender(void);

  //-------------------------------------------------------------------

  // get functions

  //-------------------------------------------------------------------

  vrpn_int32 get_mcast_info(char *info_buffer);

  vrpn_int32 get_mcast_ttl(void);


  //-------------------------------------------------------------------
  // set functions
  //-------------------------------------------------------------------
  vrpn_int32 set_mcast_ttl(vrpn_int32);
  vrpn_int32 set_mcast_loopback(vrpn_int32);


  //-------------------------------------------------------------------
  // send functions
  //-------------------------------------------------------------------
  vrpn_int32 send_message(char* message);
  vrpn_int32 marshall_message(char* outbuf, vrpn_uint32 outbuf_size, 
							  vrpn_uint32 initial_size, vrpn_uint32 len, timeval time,
							  vrpn_int32 type, vrpn_int32 sender, const char* buffer);
  vrpn_int32 send_pending_reports();

protected:

  void init_mcast_channel(void);

private:

  
  //-------------------------------------------------------------------
  // data members
  //-------------------------------------------------------------------

  struct sockaddr_in d_mcast_addr;
  vrpn_uint32 d_mcast_ttl;
  char d_mcast_outbuf[vrpn_CONNECTION_UDP_BUFLEN];
  vrpn_uint32 d_mcast_num_out; 
};


