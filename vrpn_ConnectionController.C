#include "vrpn_ConnectionController.h"



////////////////////////////////////////////////////////
// vrpn_ConnectionController constructors and destructor
//

vrpn_ConnectionController::vrpn_ConnectionController()
{
    // ...XXX...
}

vrpn_ConnectionController::~vrpn_ConnectionController()
{
    //...XXX...
}


////////////////////////////////////////////////////////
// status
//

int vrpn_ConnectionController::at_least_one_open_connection() const
{
    // ...XXX...
    return -1;
}


////////////////////////////////////////////////////////
// handle incomming and outgoing messages
//

int vrpn_ConnectionController::mainloop( const timeval * timeout )
{
    // ...XXX...
    return -1;
}

int vrpn_ConnectionController::send_pending_reports()
{
    // ...XXX...
    return -1;
}

int vrpn_ConnectionController::handle_outgoing_message(
        vrpn_uint32 len, 
        timeval time,
        vrpn_int32 type,
        vrpn_int32 sender,
        const char * buffer,
        vrpn_uint32 class_of_service )
{
    // ...XXX...
    return -1;
}


////////////////////////////////////////////////////////
//


////////////////////////////////////////////////////////
//


////////////////////////////////////////////////////////
//
