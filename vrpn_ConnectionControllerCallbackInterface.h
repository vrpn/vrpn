// moved this to separate file after all. i got more conflicts after
// the original one was solved by means other than separating this
// declaration from BaseConnectionController.h. 10/99 sain

#ifndef VRPN_CONNECTIONCONTROLLER_CALLBACK_INTERFACE_INCLUDED
#define VRPN_CONNECTIONCONTROLLER_CALLBACK_INTERFACE_INCLUDED
// please excuse our ugly hack.  we did it this way to avoid the need
// to create yet another file for just this one class
//
// This is the interface that we provide to the Connection classes so
// that they can invoke callbacks. The only functions of the
// ConnectionControllers that we want the Connection classes to have
// access to is do_callbacks_for and {got,lost}_a_connection so that
// Connections can inform the controller when a connection has been
// lost or completed correctly.
//
#include "vrpn_Shared.h"

struct vrpn_ConnectionControllerCallbackInterface
{
    virtual vrpn_int32 do_callbacks_for( 
		vrpn_int32 type, 
		vrpn_int32 sender,
		timeval time, 
		vrpn_uint32 len,
		const char * buffer) = 0;

    virtual void got_a_connection(void *) = 0;
    virtual void dropped_a_connection(void *) = 0; 

    // * set up a handler for messages of a given
    //   type_id and service_id
    // * may use vrpn_ANY_TYPE or vrpn_ANY_SERVICE or both
    // * handlers will be called during mainloop()
    // * your handler should return 0, or a communication error
    //   is assumed and the connection will be shut down
    virtual vrpn_int32 register_handler(
        vrpn_int32 type,
        vrpn_MESSAGEHANDLER handler,
        void *userdata,
        vrpn_int32 service = vrpn_ANY_SERVICE ) = 0;

    // used in vrpn_NetConnection to synchronize clocks before any
    // other user messages are sent
    virtual void synchronize_clocks();

};
//
// end of our ugly hack
//
#endif  // VRPN_CONNECTIONCONTROLLER_CALLBACK_INTERFACE_INCLUDED

