#include "vrpn_Mutex.h"

#include <string.h>  // for memcpy(), strlen(), ...

#ifndef VRPN_USE_WINSOCK_SOCKETS
#include <unistd.h>  // for gethostname()
//#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef sparc
#define INADDR_NONE -1
#endif

// Implementation notes:
//
// We broadcast messages over d_peer[] tagged with our IP number in
// network format.
// We broadcast REPLIES over d_peer[] tagged with the sender's IP number
// in network format.  When we get a reply, we check the IP number it
// contains against our own and discard it if it doesn't match.
//
// This is an ugly n^2 implementation, when we could do it in 2n, but the
// data structures would be a little more complex - we'd have to have a
// mapping from IP numbers to vrpn_Connections in d_peer[].  What I'd probably
// do would be abandon IP numbers and ports and just use lexographical
// ordering of station names.  But then we'd have to ensure that every 
// instance used the same station name (evans vs. evans.cs.unc.edu vs.
// evans.cs.unc.edu:4500) for each peer, which is ugly.



/*

  State diagram:

                             release()
          +--------------------------------------------+
          |                                            |
          v                                            |
                     request()
       AVAILABLE  -------------->  REQUESTING  ---->  OURS

          | ^                         |
          v |                         |
                                      |
      HELD_REMOTELY   <---------------+

*/




#define VERBOSE

//static const char * myID = "vrpn Mutex";
static const char * request_type = "vrpn Mutex Request";
static const char * release_type = "vrpn Mutex Release";
static const char * grantRequest_type = "vrpn Mutex Grant Request";
static const char * denyRequest_type = "vrpn Mutex Deny Request";
//static const char * losePeer_type = "vrpn Mutex Lose Peer";

struct losePeerData {
  vrpn_Connection * connection;
  vrpn_Mutex * mutex;
};

// Mostly copied from vrpn_Connection.C's vrpn_getmyIP() and some man pages.
// Should return in host order.
// Returns 0 on error.
static vrpn_uint32 getmyIP (const char * NICaddress = NULL) {
  struct hostent * host;
  char myname [100];
  in_addr in;
  int retval;


  if (NICaddress) {
    // gethostbyname() comes back in network order
    host = gethostbyname(NICaddress);
    if (host) {
      memcpy(&in.s_addr, host->h_addr, host->h_length);
      return ntohl(in.s_addr);
    }

    // inet_addr() comes back in machine order
    in.s_addr = inet_addr(NICaddress);
    if (in.s_addr != INADDR_NONE) {
      return in.s_addr;
    }

    fprintf(stderr, "getmyIP:  Can't get host entry for %s.\n", NICaddress);
    return 0;
  }

  retval = gethostname(myname, sizeof(myname));
  if (retval) {
    fprintf(stderr, "getmyIP:  Couldn't determine local hostname.\n");
    return 0;
  }

  // gethostbyname() comes back in network order
  host = gethostbyname(myname);
  if (!host) {
    fprintf(stderr, "getmyIP:  Couldn't find host by name (%s).\n", myname);
    return 0;
  }

  memcpy(&in.s_addr, host->h_addr, host->h_length);
  return ntohl(in.s_addr);
}





vrpn_Mutex::vrpn_Mutex (const char * name, int port, const char * NICaddress) :
    d_state (AVAILABLE),
    d_server (NULL),
    d_peer (NULL),
    d_numPeers (0),
    d_numConnectionsAllocated (0),
    d_myIP (getmyIP(NICaddress)),
    d_myPort (port),
    d_holderIP (0),
    d_holderPort (-1),
    d_reqGrantedCB (NULL),
    d_reqDeniedCB (NULL),
    d_takeCB (NULL),
    d_releaseCB (NULL),
    d_peerData (NULL)
{

  if (!name) {
    fprintf(stderr, "vrpn_Mutex:  NULL name!\n");
    return;
  }
  d_server = new vrpn_Synchronized_Connection (port, NULL, vrpn_LOG_NONE,
                                               NICaddress);
  if (!d_server) {
    fprintf(stderr, "vrpn_Mutex:  "
                    "Couldn't open connection on port %d!\n", port);
    return;
  }

  init(name);
}

vrpn_Mutex::vrpn_Mutex (const char * name, vrpn_Connection * server) :
    d_state (AVAILABLE),
    d_server (server),
    d_peer (NULL),
    d_numPeers (0),
    d_numConnectionsAllocated (0),
    d_myIP (getmyIP(NULL)),
    d_myPort (0),
    d_holderIP (0),
    d_holderPort (-1),
    d_reqGrantedCB (NULL),
    d_reqDeniedCB (NULL),
    d_takeCB (NULL),
    d_releaseCB (NULL),
    d_peerData (NULL)
{

  if (!name) {
    fprintf(stderr, "vrpn_Mutex:  NULL name!\n");
    return;
  }
  if (!server) {
    fprintf(stderr, "vrpn_Mutex:  NULL connection!\n");
    return;
  }

  init(name);
}

vrpn_Mutex::~vrpn_Mutex (void) {

  // Send an explicit message so they know we're shutting down, not
  // just disconnecting temporarily and will be back.

  // Probably a safer way to do it would be to do addPeer and losePeer
  // implicitly through dropped_connection/got_connection on d_server
  // and d_peer?

  // There is no good solution!

  // Possible improvement:  if we lose our connection and regain it,
  // send some sort of announcement message to everybody who used to
  // be our peer so they add us back into their tables.

  if (isHeldLocally()) {
    release();
  }

  if (d_mutexName) {
    delete [] d_mutexName;
  }
  if (d_peer) {
    delete [] d_peer;
  }
}







vrpn_bool vrpn_Mutex::isAvailable (void) const {
  return (d_state == AVAILABLE);
}

vrpn_bool vrpn_Mutex::isHeldLocally (void) const {
  return (d_state == OURS);
}

vrpn_bool vrpn_Mutex::isHeldRemotely (void) const {
  return (d_state == HELD_REMOTELY);
}

int vrpn_Mutex::numPeers (void) const {
  return d_numPeers;
}







void vrpn_Mutex::mainloop (void) {
  int i;

  d_server->mainloop();
  for (i = 0; i < d_numPeers; i++) {
    d_peer[i]->mainloop();
  }

  checkGrantMutex();
}

void vrpn_Mutex::request (void) {
  int i;

  // No point in sending requests if it's not currently available.
  // However, we need to trigger any local denial callbacks;  otherwise
  // this looks like a silent failure.
  if (!isAvailable()) {
    triggerDenyCallbacks();
    return;
  }

  // We used to wait until the next pass through mainloop() to check
  // this (our request could be trivially granted if we have no
  // peers), but that leads to bad things (TM), like having to
  // insert extra calls to mainloop() in client code just to guarantee
  // that we have the mutex.

  checkGrantMutex();

  d_state = REQUESTING;
  d_numPeersGrantingLock = 0;
  for (i = 0; i < d_numPeers; i++) {
    //d_peerData[i].grantedLock = VRPN_FALSE;
    sendRequest(d_peer[i]);
  }

  // If somebody else sends a request before we get all our grants,
  // and their IP/port# is lower than ours, we have to yield to them.

  d_holderIP = d_myIP;
  d_holderPort = d_myPort;
}

void vrpn_Mutex::release (void) {
  int i;

  // Can't release it if we don't already have it.
  // There aren't any appropriate callbacks to trigger here.  :)
  if (!isHeldLocally()) {
    return;
  }

  d_state = AVAILABLE;
  d_holderIP = 0;
  d_holderPort = -1;
  for (i = 0; i < d_numPeers; i++) {
    sendRelease(d_peer[i]);
  }

  triggerReleaseCallbacks();
}

void vrpn_Mutex::addPeer (const char * stationName) {
  vrpn_Connection ** newc;
  peerData * newg;
  losePeerData * d;
  int i;

  // complex
  if (d_numPeers >= d_numConnectionsAllocated) {

    // reallocate arrays
    newc = new vrpn_Connection * [2 + 2 * d_numConnectionsAllocated];
    newg = new peerData [2 + 2 * d_numConnectionsAllocated];
    if (!newc || !newg) {
      fprintf(stderr, "vrpn_Mutex::addPeer:  Out of memory.\n");
      return;
    }
    for (i = 0; i < d_numPeers; i++) {
      newc[i] = d_peer[i];
      newg[i] = d_peerData[i];
    }
    if (d_peer) {
      delete [] d_peer;
    }
    if (d_peerData) {
      delete [] d_peerData;
    }
    d_peer = newc;
    d_peerData = newg;
  }
  d_peer[d_numPeers] = vrpn_get_connection_by_name(stationName);
  //d_peerData[d_numPeers].grantedLock = vrpn_false;

  d = new losePeerData;
  if (!d) {
    fprintf(stderr, "vrpn_Mutex::addPeer:  Out of memory.\n");
    return;
  }
  d->connection = d_peer[d_numPeers];
  d->mutex = this;
  vrpn_int32 control;
  vrpn_int32 drop;
  control = d_peer[d_numPeers]->register_sender(vrpn_CONTROL);
  drop = d_peer[d_numPeers]->register_message_type(vrpn_dropped_connection);
  d_peer[d_numPeers]->register_handler(drop, handle_losePeer, d, control);

  d_numPeers++;
}

void vrpn_Mutex::addRequestGrantedCallback (void * ud, int (* f) (void *)) {
  mutexCallback * cb = new mutexCallback;
  if (!cb) {
    fprintf(stderr, "vrpn_Mutex::addRequestGrantedCallback:  "
                    "Out of memory.\n");
    return;
  }

  cb->userdata = ud;
  cb->f = f;
  cb->next = d_reqGrantedCB;
  d_reqGrantedCB = cb;
}

void vrpn_Mutex::addRequestDeniedCallback (void * ud, int (* f) (void *)) {
  mutexCallback * cb = new mutexCallback;
  if (!cb) {
    fprintf(stderr, "vrpn_Mutex::addRequestDeniedCallback:  "
                    "Out of memory.\n");
    return;
  }

  cb->userdata = ud;
  cb->f = f;
  cb->next = d_reqDeniedCB;
  d_reqDeniedCB = cb;
}

void vrpn_Mutex::addTakeCallback (void * ud, int (* f) (void *)) {
  mutexCallback * cb = new mutexCallback;
  if (!cb) {
    fprintf(stderr, "vrpn_Mutex::addTakeCallback:  Out of memory.\n");
    return;
  }

  cb->userdata = ud;
  cb->f = f;
  cb->next = d_takeCB;
  d_takeCB = cb;
}

// static
void vrpn_Mutex::addReleaseCallback (void * ud, int (* f) (void *)) {
  mutexCallback * cb = new mutexCallback;
  if (!cb) {
    fprintf(stderr, "vrpn_Mutex::addReleaseCallback:  "
                    "Out of memory.\n");
    return;
  }

  cb->userdata = ud;
  cb->f = f;
  cb->next = d_releaseCB;
  d_releaseCB = cb;
}

// static
int vrpn_Mutex::handle_request (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Mutex * me = (vrpn_Mutex *) userdata;
  const char * b = p.buffer;
  vrpn_uint32 senderIP;
  vrpn_uint32 senderPort;
  int i;

  vrpn_unbuffer(&b, &senderIP);
  vrpn_unbuffer(&b, &senderPort);

  // This function is where we're n^2.  If we could look up the peer
  // given their IP number we could be 2n instead.

#ifdef VERBOSE
  in_addr nad;
  nad.s_addr = htonl(senderIP);
  fprintf(stderr, "vrpn_Mutex::handle_request:  got one from %s port %d.\n",
          inet_ntoa(nad), senderPort);
#endif

  // If several nodes request the lock at once, ties are broken in favor
  // of the node with the lowest IP number & port.

  if ((me->d_state == AVAILABLE) ||
      (((me->d_state == HELD_REMOTELY) ||
        (me->d_state == REQUESTING)) &&
       ((senderIP < me->d_holderIP) ||
        (senderIP == me->d_holderIP) &&
        (senderPort < me->d_holderPort)))) {

    me->d_holderIP = senderIP;
    me->d_holderPort = senderPort;

    if (me->d_state != HELD_REMOTELY) {
      me->triggerTakeCallbacks();
    }

    me->d_state = HELD_REMOTELY;
    for (i = 0; i < me->d_numPeers; i++) {
      me->sendGrantRequest(me->d_peer[i], senderIP, senderPort);
    }
    return 0;
  }

  for (i = 0; i < me->d_numPeers; i++) {
    me->sendDenyRequest(me->d_peer[i], senderIP, senderPort);
  }

  return 0;
}

// static
int vrpn_Mutex::handle_release (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Mutex * me = (vrpn_Mutex *) userdata;
  const char * b = p.buffer;
  vrpn_uint32 senderIP;
  vrpn_uint32 senderPort;

  vrpn_unbuffer(&b, &senderIP);
  vrpn_unbuffer(&b, &senderPort);

#ifdef VERBOSE
  in_addr nad;

  nad.s_addr = senderIP;
  fprintf(stderr, "vrpn_Mutex::handle_release:  got one from %s port %d.\n",
          inet_ntoa(nad), senderPort);
#endif

  if ((senderIP != me->d_holderIP) ||
      (senderPort != me->d_holderPort)) {
    fprintf(stderr, "vrpn_Mutex::handle_release:  Got a release from "
                    "somebody who didn't have the lock!?\n");
  }

  me->d_state = AVAILABLE;
  me->d_holderIP = 0;
  me->d_holderPort = -1;

  me->triggerReleaseCallbacks();

  return 0;
}

// static
int vrpn_Mutex::handle_grantRequest (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Mutex * me = (vrpn_Mutex *) userdata;
  const char * b = p.buffer;
  vrpn_uint32 senderIP;
  vrpn_uint32 senderPort;

  vrpn_unbuffer(&b, &senderIP);
  vrpn_unbuffer(&b, &senderPort);

#ifdef VERBOSE
  in_addr nad;
  nad.s_addr = senderIP;
  fprintf(stderr, "vrpn_Mutex::handle_grantRequest:  "
                  "got one for %s port %d.\n",
                  inet_ntoa(nad), senderPort);
#endif

  if ((senderIP != me->d_myIP) || (senderPort != me->d_myPort)) {
    // we can treat this as a denial -
    // but we'll get a denial eventually,
    // so we need not do so.
    return 0;
  }

  me->d_numPeersGrantingLock++;

  me->checkGrantMutex();

  return 0;
}

// static
int vrpn_Mutex::handle_denyRequest (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Mutex * me = (vrpn_Mutex *) userdata;
  const char * b = p.buffer;
  vrpn_uint32 senderIP;
  vrpn_uint32 senderPort;

  vrpn_unbuffer(&b, &senderIP);
  vrpn_unbuffer(&b, &senderPort);

#ifdef VERBOSE
  in_addr nad;
  nad.s_addr = senderIP;
  fprintf(stderr, "vrpn_Mutex::handle_denyRequest:  "
                  "got one for %s port %d.\n",
                  inet_ntoa(nad), senderPort);
#endif

  if ((senderIP != me->d_myIP) || (senderPort != me->d_myPort)) {
    return 0;
  }
  
  me->d_numPeersGrantingLock = 0;

  me->triggerDenyCallbacks();

  return 0;
}

// static
int vrpn_Mutex::handle_losePeer (void * userdata, vrpn_HANDLERPARAM p) {
  losePeerData * data = (losePeerData *) userdata;
  vrpn_Mutex * me = data->mutex;
  vrpn_Connection * c = data->connection;
  int i;

  // Need to abort a request since we don't have enough data to correctly
  // compensate for losing a peer mid-request.

  if (me->d_state == REQUESTING) {
    me->release();
  }

  for (i = 0; i < me->d_numPeers; i++) {
    if (c == me->d_peer[i]) {
      break;
    }
  }
  if (i == me->d_numPeers) {
    fprintf(stderr, "vrpn_Mutex::handle_losePeer:  Can't find lost peer.\n");
    return 0;  // -1?
  }

fprintf(stderr, "vrpn_Mutex::handle_losePeer:  lost peer #%d.\n", i);

  me->d_numPeers--;
  me->d_peer[i] = me->d_peer[me->d_numPeers];

  delete data;

  return 0;
}

void vrpn_Mutex::sendRequest (vrpn_Connection * c) {
  timeval now;
  char buffer [32];
  char * b = buffer;
  vrpn_int32 bl = 32;
  
  gettimeofday(&now, NULL);
  vrpn_buffer(&b, &bl, d_myIP);
  vrpn_buffer(&b, &bl, d_myPort);
  c->pack_message(32 - bl, now,
                  c->register_message_type(request_type),
                  c->register_sender(d_mutexName), buffer,
                  vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::sendRelease (vrpn_Connection * c) {
  timeval now;
  char buffer [32];
  char * b = buffer;
  vrpn_int32 bl = 32;

  gettimeofday(&now, NULL);
  vrpn_buffer(&b, &bl, d_myIP);
  vrpn_buffer(&b, &bl, d_myPort);
  c->pack_message(32 - bl, now,
                  c->register_message_type(release_type),
                  c->register_sender(d_mutexName), buffer,
                  vrpn_CONNECTION_RELIABLE);
}


void vrpn_Mutex::sendGrantRequest (vrpn_Connection * c, vrpn_uint32 IP,
                                   vrpn_uint32 port) {
  timeval now;
  char buffer [32];
  char * b = buffer;
  vrpn_int32 bl = 32;

  gettimeofday(&now, NULL);
  vrpn_buffer(&b, &bl, IP);
  vrpn_buffer(&b, &bl, port);
  c->pack_message(32 - bl, now,
                  c->register_message_type(grantRequest_type),
                  c->register_sender(d_mutexName), buffer,
                  vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::sendDenyRequest (vrpn_Connection * c, vrpn_uint32 IP,
                                  vrpn_uint32 port) {
  timeval now;
  char buffer [32];
  char * b = buffer;
  vrpn_int32 bl = 32;

  gettimeofday(&now, NULL);
  vrpn_buffer(&b, &bl, IP);
  vrpn_buffer(&b, &bl, port);
  c->pack_message(32 - bl, now,
                  c->register_message_type(denyRequest_type),
                  c->register_sender(d_mutexName), buffer,
                  vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::triggerGrantCallbacks (void) {
  mutexCallback * cb;

  // trigger callbacks
  for (cb = d_reqGrantedCB;  cb;  cb = cb->next) {
    (cb->f)(cb->userdata);
  }
}

void vrpn_Mutex::triggerDenyCallbacks (void) {
  mutexCallback * cb;

  // trigger callbacks
  for (cb = d_reqDeniedCB;  cb;  cb = cb->next) {
    (cb->f)(cb->userdata);
  }
}

void vrpn_Mutex::triggerTakeCallbacks (void) {
  mutexCallback * cb;

  // trigger callbacks
  for (cb = d_takeCB;  cb;  cb = cb->next) {
    (cb->f)(cb->userdata);
  }
}

void vrpn_Mutex::triggerReleaseCallbacks (void) {
  mutexCallback * cb;

  // trigger callbacks
  for (cb = d_releaseCB;  cb;  cb = cb->next) {
    (cb->f)(cb->userdata);
  }
}

void vrpn_Mutex::checkGrantMutex (void) {

  if ((d_state == REQUESTING) &&
      (d_numPeersGrantingLock == d_numPeers)) {
    d_state = OURS;

    triggerTakeCallbacks();
    triggerGrantCallbacks();

  }
}


void vrpn_Mutex::init (const char * name) {

  d_mutexName = new char [1 + strlen(name)];
  if (!d_mutexName) {
    fprintf(stderr, "vrpn_Mutex::init:  Out of memory.\n");
    return;
  }
  strncpy(d_mutexName, name, strlen(name));

  d_myId = d_server->register_sender(name);
  d_request_type = d_server->register_message_type(request_type);
  d_release_type = d_server->register_message_type(release_type);
  d_grantRequest_type = d_server->register_message_type(grantRequest_type);
  d_denyRequest_type = d_server->register_message_type(denyRequest_type);

  d_server->register_handler(d_request_type, handle_request, this, d_myId);
  d_server->register_handler(d_release_type, handle_release, this, d_myId);
  d_server->register_handler(d_grantRequest_type, handle_grantRequest,
                             this, d_myId);
  d_server->register_handler(d_denyRequest_type, handle_denyRequest,
                             this, d_myId);

}

