// FILENAME: vrpn_BaseMulticast.C
// AUTHOR: Stefan Sain
// DATE: 7/1/1999
//
// THIS IS A WORK IN PROGRESS
//
// This is a new class used in the redesigned version of vrpn_Connection. 
// This is the multicast channel that hangs off of the new 
// vrpn_ConnectionController class. 
//
// The current class hierarchy is:
// * BaseMulticastChannel - an abstract base class
// * UnreliableMulticastSender - inherits from BaseMulticastChannel
// * UnreliableMulticastRecvr - inherits from BaseMulticastChannel
//
// In the future someone might implement a reliable multicast class.
//

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef FreeBSD
// malloc.h is deprecated in FreeBSD;  all the functionality *should*
// be in stdlib.h
#include <malloc.h>
#endif

#ifdef _WIN32
#include <winsock.h>
// a socket in windows can not be closed like it can in unix-land
#define close closesocket
#else
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifdef	sgi
#include <bstring.h>
#endif

#ifdef hpux
#include <arpa/nameser.h>
#include <resolv.h>  // for herror() - but it isn't there?
#endif

#ifdef _WIN32
#include <winsock.h>
#include <sys/timeb.h>
#else
#include <sys/wait.h>
#include <sys/resource.h>  // for wait3() on sparc_solaris
#include <netinet/tcp.h>
#endif

#include "vrpn_cygwin_hack.h"

// cast fourth argument to setsockopt()
#ifdef _WIN32
#define SOCK_CAST (char *)
#else
 #ifdef sparc
#define SOCK_CAST (const char *)
 #else
#define SOCK_CAST
 #endif
#endif

#ifdef linux
#define GSN_CAST (unsigned int *)
#else
#define GSN_CAST
#endif

//  NOT SUPPORTED ON SPARC_SOLARIS
//  gethostname() doesn't seem to want to link out of stdlib
#ifdef sparc
extern "C" {
int gethostname (char *, int);
}
#endif

#include "vrpn_BaseMulticast.h"


//==========================================================================
//
// class vrpn_BaseMulticastChannel
//
// this is the abstract base class from which the other multicast classes 
// inherit, currently there are unreliable sender and receiver classes. in 
// the future there may be reliable sender and receiver classes inheriting
// from this one.
//
//==========================================================================

//**************************************************************************
//**************************************************************************
//
// vrpn_BaseMulticastChannel: public functions
//
//**************************************************************************
//**************************************************************************

//-------------------------------------------------------------------
// constructors and destructor
//-----------------------------------------------------------------
vrpn_BaseMulticastChannel::vrpn_BaseMulticastChannel( char* group_name, 
							   vrpn_uint16 port)
  :d_mcast_port(port){
  
  strcpy(d_mcast_group,group_name);

}


//-------------------------------------------------------------------
// status functions
//-------------------------------------------------------------------
vrpn_bool vrpn_BaseMulticastChannel::created_correctly(void){
	return d_created_correctly;
}


//-------------------------------------------------------------------
// get functions
//------------------------------------------------------------------


// get multicast group name as a string in dotted decimal form
char* vrpn_BaseMulticastChannel::get_mcast_group_name(void){
  return d_mcast_group;
}

// get port number used of multicast messages
vrpn_uint16 vrpn_BaseMulticastChannel::get_mcast_port_num(void){
  return d_mcast_port;
}

// get socket number of multicast socket

vrpn_int32 vrpn_BaseMulticastChannel::get_mcast_sock(void){

	return d_mcast_sock;

}


//**************************************************************************
//**************************************************************************
//
// vrpn_BaseMulticastChannel: protected functions
//
//**************************************************************************
//**************************************************************************

//-------------------------------------------------------------------
// set functions
//------------------------------------------------------------------

// set multicast group name
// name is string in dotted decimal format
void vrpn_BaseMulticastChannel::set_mcast_group_name(char *group){
	strcpy(d_mcast_group,group);
}

// set port number for multicast packets
void vrpn_BaseMulticastChannel::set_mcast_port_num(vrpn_uint16 port){
	d_mcast_port = port;
}

// set socket number for multicast socket
// is only called during initilization of mcast channel
vrpn_int32 vrpn_BaseMulticastChannel::set_mcast_sock(vrpn_int32 sock){
	d_mcast_sock = sock;
	return sock;
}

// return bool of whether everything was initialized ok
void vrpn_BaseMulticastChannel::set_created_correctly(vrpn_bool status){
	d_created_correctly = status;
}
