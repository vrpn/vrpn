#ifndef VRPN_CONNECTION_TYPES_H
#define VRPN_CONNECTION_TYPES_H

/// @file vrpn_ConnectionTypes.h
/// This file contains types and structs needed by vrpn_Connection
/// and its helper classes.


struct vrpn_HANDLERPARAM {
        vrpn_int32      type;
        vrpn_int32      sender;
        struct timeval  msg_time;
        vrpn_int32      payload_len;
        const char      *buffer;
};
typedef int  (*vrpn_MESSAGEHANDLER)(void *userdata, vrpn_HANDLERPARAM p);


// vrpn_ANY_SENDER can be used to register callbacks on a given message
// type from any sender.

#define vrpn_ANY_SENDER (-1)

// vrpn_ANY_TYPE can be used to register callbacks for any USER type of
// message from a given sender.  System messages are handled separately.

#define vrpn_ANY_TYPE (-1)

typedef char cName [100];

// Description of a callback entry for a user type.
struct vrpnMsgCallbackEntry {
  vrpn_MESSAGEHANDLER   handler;        // Routine to call
  void                  * userdata;     // Passed along
  vrpn_int32            sender;         // Only if from sender
  vrpnMsgCallbackEntry  * next;         // Next handler
};


// Types now have their storage dynamically allocated, so we can afford
// to have large tables.  We need at least 150-200 for the microscope
// project as of Jan 98, and will eventually need two to three times that
// number.
#define vrpn_CONNECTION_MAX_SENDERS     (2000)
#define vrpn_CONNECTION_MAX_TYPES       (2000)





#endif  // VRPN_CONNECTION_TYPES_H


