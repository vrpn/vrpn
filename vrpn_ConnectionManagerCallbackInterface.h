// moved this to separate file after all. i got more conflicts after
// the original one was solved by means other than separating this
// declaration from BaseConnectionController.h. 10/99 sain

#ifndef VRPN_CONNECTIONMANAGER_CALLBACK_INTERFACE_INCLUDED
#define VRPN_CONNECTIONMANAGER_CALLBACK_INTERFACE_INCLUDED
// please excuse our ugly hack.  we did it this way to avoid the need
// to create yet another file for just this one class
//
// This is the interface that we provide to the Connection classes so
// that they can invoke callbacks. The only functions of the
// ConnectionManagers that we want the Connection classes to have
// access to is do_callbacks_for and {got,lost}_a_connection so that
// Connections can inform the manager when a connection has been
// lost or completed correctly.
//
#include "vrpn_Shared.h"

#if 1 // temporary hack
    struct vrpn_MsgCallbackEntry;
#endif

struct vrpn_ConnectionManagerCallbackInterface
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


    // The Connections need a way to get at the list of services/types in the
    // Manager.  We will use STL-style sequences, by returning
    // const_iterators to the front and end of the sequence.  The iterators
    // are simply const pointers.
    
    // iterator to the first service in sequence of local services
    virtual const char* service_begin();

    // iterator to one-past-the-end service in sequence of local services
    virtual const char* service_end();

#if 0 // doesn't work
    typedef vrpn_BaseConnectionManager::vrpnLocalMapping LM;
#else
    struct vrpnLocalMapping {
        char *                  name;       // Name of type
        vrpn_MsgCallbackEntry *  who_cares;  // Callbacks
        vrpn_int32              cCares;     // TCH 28 Oct 97
    };
#endif

    // iterator to the first type in sequence of local types
    virtual const vrpnLocalMapping* type_begin();

    // iterator to one-past-the-end type in sequence of local types
    virtual const vrpnLocalMapping* type_end();
};
//
// end of our ugly hack
//
#endif  // VRPN_CONNECTIONMANAGER_CALLBACK_INTERFACE_INCLUDED

