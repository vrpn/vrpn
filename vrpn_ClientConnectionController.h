#ifndef VRPN_CLIENTCONNECTIONCONTROLLER_INCLUDED
#define VRPN_CLIENTCONNECTIONCONTROLLER_INCLUDED

////////////////////////////////////////////////////////
//
// class vrpn_ClientConnectionController
//
// this class HAS-A single BaseConnection object that it
// communicates through
//
// beyond BaseConnectionController, it implements
//  * initiates a connection (active)
//  * reconnect attempts (server not yet running or restarting)
//  * client-side logging
//

#include "vrpn_BaseConnectionController.h"

class vrpn_ClientConnectionController
    : vrpn_BaseConnectionController
{
public:  // c'tors and d'tors

    // destructor ...XXX...
    virtual ~vrpn_ClientConnectionController();

protected:  // c'tors

    // constructors ...XXX...
    vrpn_ClientConnectionController();

    // helper function
    // XXX if this doesn't go away, give a more descriptive comment
    virtual int connect_to_server( const char *machine, int port );
    
private: // the connection
    //list <vrpn_BaseConnection *> connection_list;
};

#endif
