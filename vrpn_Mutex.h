#ifndef VRPN_MUTEX_H
#define VRPN_MUTEX_H


#include "vrpn_Shared.h"  // for timeval and timeval manipulation functions
#include "vrpn_Connection.h"  // for vrpn_HANDLERPARAM


// Possible bugs -
//   If on startup somebody else is holding the mutex we'll think it's
// available.  However, if we request it they'll deny it to us and
// we'll reach the appropriate state.
//   If sites don't execute the same set of addPeer() commands, they may
// implicitly partition the network and not get true mutual exclusion.
// This could be fixed by sending an addPeer message.
//   If sites execute addPeer() while the lock is held, or being requested,
// we'll break.

// Handling more than 2 sites in a mutex requires multiconnection servers.

// This is an n^2 implementation;  for details (and how to fix if it ever
// becomes a problem), see the implementation notes in vrpn_Mutex.C.

// This implementation trusts all hosts;  a machine with a low IP number
// (or willing to claim that it has a low ID number) can steal the mutex
// or break mutual exclusion.



class vrpn_Mutex {

  public:

    vrpn_Mutex (const char * name, int port, const char * NICaddress = NULL);
    ~vrpn_Mutex (void);

    vrpn_bool isAvailable (void) const;
    vrpn_bool isHeldLocally (void) const;
    vrpn_bool isHeldRemotely (void) const;
    int numPeers (void) const;

    void mainloop (void);

    void request (void);
    void release (void);

    void addPeer (const char * stationName);

    void addRequestGrantedCallback (void *, int (*) (void *));
    void addRequestDeniedCallback (void *, int (*) (void *));
    void addReleaseCallback (void *, int (*) (void *));

  protected:

    enum state { OURS, REQUESTING, AVAILABLE, HELD_REMOTELY};

    char * d_mutexName;

    state d_state;
    int d_numPeersGrantingLock;
      ///< Counts the number of "grants" we've received after issuing
      ///< a request;  when this reaches d_numPeers, the lock is ours.

    vrpn_Connection * d_server;
      ///< Receive on this connection.
    vrpn_Connection ** d_peer;
      ///< Send on these connections to other Mutex's well-known-ports.

    int d_numPeers;
    int d_numConnectionsAllocated;
      ///< Dynamic array size.

    vrpn_uint32 d_myIP;  // in native format
    vrpn_uint32 d_netIP;  // my IP in network format
    vrpn_uint32 d_holderIP;  // in native format
    vrpn_uint32 d_requesterIP;  // in network format
      ///< IP numbers are used for precise sender identification.

    vrpn_int32 d_myId;
    vrpn_int32 d_request_type;
    vrpn_int32 d_release_type;
    vrpn_int32 d_grantRequest_type;
    vrpn_int32 d_denyRequest_type;

    static int handle_request (void *, vrpn_HANDLERPARAM);
    static int handle_release (void *, vrpn_HANDLERPARAM);
    static int handle_grantRequest (void *, vrpn_HANDLERPARAM);
    static int handle_denyRequest (void *, vrpn_HANDLERPARAM);

    void sendRequest (vrpn_Connection *);
    void sendRelease (vrpn_Connection *);
    void sendGrantRequest (vrpn_Connection *, vrpn_uint32 IPnumber);
    void sendDenyRequest (vrpn_Connection *, vrpn_uint32 IPnumber);

    struct mutexCallback {
      int (* f) (void *);
      void * userdata;
      mutexCallback * next;
    };

    mutexCallback * d_reqGrantedCB;
    mutexCallback * d_reqDeniedCB;
    mutexCallback * d_releaseCB;

};

#endif  // VRPN_MUTEX_H


