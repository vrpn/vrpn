// vrpn_ConnectionCommonStuff.h
// 
// this is a place to put stuff that is shared between the different
// connection classes like BaseConnectionController and BaseConnection
// and their subclasses

#ifndef VRPN_CONNECTIONCOMMONSTUFF_INCLUDED
#define VRPN_CONNECTIONCOMMONSTUFF_INCLUDED

#include "vrpn_Shared.h"


// bufs are aligned on 8 byte boundaries
#define vrpn_ALIGN                       (8)


// Types now have their storage dynamically allocated, so we can afford
// to have large tables.  We need at least 150-200 for the microscope
// project as of Jan 98, and will eventually need two to three times that
// number.
#define	vrpn_CONNECTION_MAX_SENDERS   (2000) /*XXX*/
#define	vrpn_CONNECTION_MAX_SERVICES  (2000)
#define	vrpn_CONNECTION_MAX_TYPES     (2000)


// vrpn_ANY_SERVICE can be used to register callbacks on a given message
// type from any service.

#define	vrpn_ANY_SENDER  (-1) /*XXX*/
#define	vrpn_ANY_SERVICE (-1)

// vrpn_ANY_TYPE can be used to register callbacks for any USER type of
// message from a given sender.  System messages are handled separately.

#define vrpn_ANY_TYPE (-1)


// Classes of service for messages, specify multiple by ORing them together
// Priority of satisfying these should go from the top down (RELIABLE will
// override all others).
// Most of these flags may be ignored, but RELIABLE is guaranteed
// to be available.

#define vrpn_CONNECTION_RELIABLE        (1<<0)
#define vrpn_CONNECTION_FIXED_LATENCY       (1<<1)
#define vrpn_CONNECTION_LOW_LATENCY     (1<<2)
#define vrpn_CONNECTION_FIXED_THROUGHPUT    (1<<3)
#define vrpn_CONNECTION_HIGH_THROUGHPUT     (1<<4)

#define vrpn_got_first_connection "VRPN Connection Got First Connection"
#define vrpn_got_connection "VRPN Connection Got Connection"
#define vrpn_dropped_connection "VRPN Connection Dropped Connection"
#define vrpn_dropped_last_connection "VRPN Connection Dropped Last Connection"

// vrpn_CONTROL is used for notification messages sent to the user
// from the local VRPN implementation (got_first_connection, etc.)
// and for control messages sent by auxiliary services.  (Such as
// class vrpn_Controller, which will be introduced in a future revision.)

#define vrpn_CONTROL "VRPN Control"


struct vrpn_HANDLERPARAM {
    vrpn_int32  type;
    vrpn_int32  sender;
    struct timeval  msg_time;
    vrpn_int32  payload_len;
    const char  *buffer;
};
typedef int  (*vrpn_MESSAGEHANDLER)(void *userdata, vrpn_HANDLERPARAM p);

typedef char cName [100];



#endif
