#ifndef VRPN_SERVERCONNECTIONCONTROLLER_INCLUDED
#define VRPN_SERVERCONNECTIONCONTROLLER_INCLUDED

////////////////////////////////////////////////////////
//
// class vrpn_ServerConnectionController
//
// this class HAS-A list of BaseConnection objects that it
// communicates through
//
// beyond BaseConnectionController, it implements
//  * listens for connections (passive)
//  * multiple connections
//  * multicast sending
//

#include "vrpn_BaseConnectionController.h"

class vrpn_ServerConnectionController
    : public vrpn_BaseConnectionController
{
public:  // c'tors and d'tors

    // destructor ...XXX...
    virtual ~vrpn_ServerConnectionController();
	
protected:  // c'tors and init
	
    // constructors ...XXX...
    vrpn_ServerConnectionController();

	virtual void init(void);

public: // sending and receving mesages

    // * call each time through the program main loop
    // * handles receiving any incoming messages
    //   and sending any packed messages
    // * returns -1 when connection dropped due to error, 0 otherwise
    //   (only returns -1 once per connection drop)
    // * optional argument is TOTAL time to block on select() calls;
    //   there may be multiple calls to select() per call to mainloop(),
    //   and this timeout will be divided evenly between them.
    virtual vrpn_int32 mainloop( const timeval * timeout = NULL );


private: // the connections
    //list <vrpn_BaseConnection *> connection_list;

protected: // initializaton and connection setup

	// listen for and process incoming connections on well known port
	// might have equivalent in Connection
	void listen_for_incoming_connections(const struct timeval * pTimeout);

	// get mcast group info from mcast sender
	char* get_mcast_info();

    // helper function
    // XXX if this doesn't go away, give a more descriptive comment
    virtual vrpn_int32 connect_to_client( const char *machine, vrpn_int16 port );
    	
private: // data members

	char* d_mcast_info;			// stores mcast group info
	vrpn_int32	status;			// Status of the connection

	/*
	// perhaps we should move to enums
	enum ControllerStatus = { NOTCONNECTED, CONNECTED, ERROR };
	ControllerStatus status;
	*/

	vrpn_int32	num_live_connections;

	// Only used for a vrpn_Connection that awaits incoming connections
	vrpn_int32	listen_udp_sock;	// Connect requests come here

};

#endif
