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

protected:  // c'tors

    // constructors ...XXX...
    vrpn_ServerConnectionController();

    // helper function
    // XXX if this doesn't go away, give a more descriptive comment
    virtual int connect_to_client( const char *machine, int port );
    

private: // the connections
    //list <vrpn_BaseConnection *> connection_list;
};

#endif
