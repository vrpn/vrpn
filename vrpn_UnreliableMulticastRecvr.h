
#include "vrpn_BaseMulticast.h"

//=========================================================================
//
// class vrpn_UnreliableMulticasrRecvr
//
// this class is responsible for receiving multicast messages
//
//==========================================================================
class vrpn_UnreliableMulticastRecvr: public vrpn_BaseMulticastChannel {

public:

  //-------------------------------------------------------------------
  // constructors and destructors
  //-------------------------------------------------------------------
  vrpn_UnreliableMulticastRecvr( char* group_name,
								 vrpn_uint16 port);
  ~vrpn_UnreliableMulticastRecvr();


  //-------------------------------------------------------------------
  // get functions
  //-------------------------------------------------------------------
  vrpn_int32 get_mcast_info(char *info_buffer);


  //-------------------------------------------------------------------
  // change group membership functions
  //-------------------------------------------------------------------
  vrpn_int32 join_mcast_group(char*);
  vrpn_int32 leave_mcast_group(char*);

  //-------------------------------------------------------------------
  // receive messages
  //-------------------------------------------------------------------
  vrpn_int32 handle_incoming_message(char*, const struct timeval*);

protected:
  void init_mcast_channel();

private:

  //----------------------------------------------------------------
  // data members
  //----------------------------------------------------------------
  struct sockaddr_in d_mcast_addr;
  vrpn_float64 d_MCASTinbufToAlignRight
	[vrpn_CONNECTION_UDP_BUFLEN / sizeof(vrpn_float64) + 1];
  char d_mcast_inbuf[vrpn_CONNECTION_UDP_BUFLEN];
};
