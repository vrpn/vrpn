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
#include "vrpn_CommonSystemIncludes.h"
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
  :d_mcast_group(NULL),d_mcast_port(port)
{
  
  set_mcast_group_name(group_name);

}

vrpn_BaseMulticastChannel::~vrpn_BaseMulticastChannel()
{
	if( d_mcast_group )
		delete d_mcast_group;
}

//-------------------------------------------------------------------
// status functions
//-------------------------------------------------------------------
vrpn_bool vrpn_BaseMulticastChannel::created_correctly(){
	return d_created_correctly;
}


//-------------------------------------------------------------------
// get functions
//------------------------------------------------------------------


// get multicast group name as a string in dotted decimal form
char* vrpn_BaseMulticastChannel::get_mcast_group_name(){
  return d_mcast_group;
}

// get port number used of multicast messages
vrpn_uint16 vrpn_BaseMulticastChannel::get_mcast_port_num(){
  return d_mcast_port;
}

// get socket number of multicast socket

vrpn_int32 vrpn_BaseMulticastChannel::get_mcast_sock(){

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
void vrpn_BaseMulticastChannel::set_mcast_group_name(char *group)
{
	if( d_mcast_group )
		delete d_mcast_group;
	d_mcast_group = new char[strlen(group)+1];
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
