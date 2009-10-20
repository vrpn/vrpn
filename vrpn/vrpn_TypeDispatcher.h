#ifndef VRPN_TYPE_DISPATCHER_H
#define VRPN_TYPE_DISPATCHER_H

#include "vrpn_Shared.h"
#include "vrpn_ConnectionTypes.h"

/// @class vrpn_TypeDispatcher
/// Handles types, senders, and callbacks.
/// Also handles system messages through the same non-orthogonal technique 
/// as always.  This class is an implementation detail of vrpn_Connection
/// that is exposed so that it can be used by other classes that need to
/// maintain their own lists of callbacks.

class VRPN_API vrpn_TypeDispatcher {

  public:

    vrpn_TypeDispatcher (void);
    ~vrpn_TypeDispatcher (void);


    // ACCESSORS


    int numTypes (void) const;
    const char * typeName (int which) const;

    vrpn_int32 getTypeID (const char * name);
      ///< Returns -1 if not found.

    int numSenders (void) const;
    const char * senderName (int which) const;

    vrpn_int32 getSenderID (const char * name);
      ///< Returns -1 if not found.


    // MANIPULATORS


    vrpn_int32 addType (const char * name);
    vrpn_int32 addSender (const char * name);

    vrpn_int32 registerType (const char * name);
      ///< Calls addType() iff getTypeID() returns -1.
      ///< vrpn_Connection doesn't call this because it needs to know
      ///< whether or not to send a type message.

    vrpn_int32 registerSender (const char * name);
      ///< Calls addSender() iff getSenderID() returns -1.
      ///< vrpn_Connection doesn't call this because it needs to know
      ///< whether or not to send a sender message.


    vrpn_int32 addHandler (vrpn_int32 type, vrpn_MESSAGEHANDLER handler,
                           void * userdata, vrpn_int32 sender);
    vrpn_int32 removeHandler (vrpn_int32 type, vrpn_MESSAGEHANDLER handler,
                              void * userdata, vrpn_int32 sender);
    void setSystemHandler (vrpn_int32 type, vrpn_MESSAGEHANDLER handler);


    // It'd make a certain amount of sense to unify these next few, but
    // there are some places in the code that depend on the side effect of
    // do_callbacks_for() NOT dispatching system messages.

    int doCallbacksFor (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer);
    int doSystemCallbacksFor
                       (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer,
                        void * userdata);
    int doSystemCallbacksFor
                       (vrpn_HANDLERPARAM p,
                        void * userdata);

    void clear (void);

  protected:

    struct vrpnLocalMapping {
      char                      * name;         // Name of type
      vrpnMsgCallbackEntry      * who_cares;    // Callbacks
      vrpn_int32                cCares;         // TCH 28 Oct 97
    };

    int d_numTypes;
    vrpnLocalMapping d_types [vrpn_CONNECTION_MAX_TYPES];

    int d_numSenders;
    char * d_senders [vrpn_CONNECTION_MAX_SENDERS];

    vrpn_MESSAGEHANDLER d_systemMessages [vrpn_CONNECTION_MAX_TYPES];

    vrpnMsgCallbackEntry * d_genericCallbacks;
};



#endif  // VRPN_TYPE_DISPATCHER_H


