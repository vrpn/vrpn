#ifndef VRPN_MUTEX_H
#define VRPN_MUTEX_H


#include "vrpn_Shared.h"  // for timeval and timeval manipulation functions
#include "vrpn_Connection.h"  // for vrpn_HANDLERPARAM


/// vrpn_Mutex
///
///   This class provides distributed mutual exclusion between every instance
/// with the same name for which addPeer() has been called.
///   If a process calls request() when isAvailable() returns true,
/// the mutex will attempt to secure a lock to whatever resource it is
/// governing;  either RequestGranted or RequestDenied callbacks will
/// be triggered.  If RequestGranted callbacks are triggered, the process
/// has the lock until it explicitly calls release() (and can verify this
/// by checking isHeldLocally()).  Once the lock-owner calls release(),
/// Release callbacks at every peer will be triggered.
///
///   Like most vrpn classes, the mainloop() must be called frequently.
///
///   Note that none of isAvailable(), isHeldLocally(), and isHeldRemotely()
/// are true between when request() is called and either RequestGranted or
/// RequestDenied callbacks are triggered.

// Possible bugs -

//   If on startup somebody else is holding the mutex we'll think it's
// available.  However, if we request it they'll deny it to us and
// we won't break.
//   If sites don't execute the same set of addPeer() commands, they may
// implicitly partition the network and not get true mutual exclusion.
// This could be fixed by sending an addPeer message.
//   If sites execute addPeer() while the lock is held, or being requested,
// we'll break.

// Handling more than 2 sites in a mutex requires multiconnection servers.
// It's been tested with 1-3 sites, and works fine.

// This is an O(n^2) network traffic implementation;
// for details (and how to fix if it ever becomes a problem),
// see the implementation notes in vrpn_Mutex.C.

// The name is currently meaningless, since we can't multiplex multiple
// connections over a single port since we have to open a new port -
// there's no way to get vrpn_get_connection_by_name to give us a server
// port.



class vrpn_Mutex {

  public:

    vrpn_Mutex (const char * name, int port, const char * NICaddress = NULL);
    ~vrpn_Mutex (void);


    // ACCESSORS


    vrpn_bool isAvailable (void) const;
      ///< True from when release() is called or we receive a release
      ///< message from another process until request() is called or we
      ///< grant the lock to another process in response to its request
      ///< message.
    vrpn_bool isHeldLocally (void) const;
      ///< True from when RequestGranted callbacks are triggered until
      ///< release() is called.
    vrpn_bool isHeldRemotely (void) const;
      ///< True from when we grant the lock to another process in response
      ///< to its request message until we receive a release message from
      ///< another process.

    int numPeers (void) const;


    // MANIPULATORS


    void mainloop (void);

    void request (void);
      ///< Request the distributed lock.  Does nothing if !isAvailable().

    void release (void);
      ///< Release the distributed lock.  Does nothing if !isHeldLocally().

    void addPeer (const char * stationName);
      ///< Takes a VRPN station name of the form "<host>:<port>".

    void addRequestGrantedCallback (void * userdata, int (*) (void *));
    void addRequestDeniedCallback (void * userdata, int (*) (void *));
    void addReleaseCallback (void * userdata, int (*) (void *));

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
      ///< Dynamic array size for d_peer.

    vrpn_uint32 d_myIP;
    vrpn_uint32 d_myPort;
    vrpn_uint32 d_holderIP;
    vrpn_uint32 d_holderPort;

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
    void sendGrantRequest (vrpn_Connection *, vrpn_uint32 IPnumber,
                           vrpn_uint32 PortNumber);
    void sendDenyRequest (vrpn_Connection *, vrpn_uint32 IPnumber,
                          vrpn_uint32 PortNumber);

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


