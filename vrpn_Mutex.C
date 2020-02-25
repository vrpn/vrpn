#include <stdio.h>  // for fprintf, stderr, sprintf
#include <string.h> // for NULL, memcpy, strlen, etc

#include "vrpn_Connection.h" // for vrpn_Connection, etc
#include "vrpn_Mutex.h"
#include "vrpn_Shared.h" // for vrpn_buffer, vrpn_unbuffer, etc

#ifndef VRPN_USE_WINSOCK_SOCKETS
#include <arpa/inet.h> // for inet_addr
#include <netdb.h>     // for gethostbyname, hostent, etc
//#include <sys/socket.h>
#include <netinet/in.h> // for in_addr, ntohl, INADDR_NONE
#include <unistd.h>     // for getpid, gethostname
#endif

#ifdef _WIN32
#include <process.h> // for _getpid()
#endif

#ifdef sparc
#define INADDR_NONE -1
#endif

/*

  Server State diagram:

                handle_release()
          +-------------------------+
          |                         |
          v                         |
               handle_request()
        FREE  ------------------>  HELD

  Peer State diagram:

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

//#define VERBOSE

// static const char * myID = "vrpn_Mutex";
static const char *requestIndex_type = "vrpn_Mutex Request Index";
static const char *requestMutex_type = "vrpn_Mutex Request Mutex";
static const char *release_type = "vrpn_Mutex Release";
static const char *releaseNotification_type = "vrpn_Mutex Release_Notification";
static const char *grantRequest_type = "vrpn_Mutex Grant_Request";
static const char *denyRequest_type = "vrpn_Mutex Deny_Request";
// static const char * losePeer_type = "vrpn_Mutex Lose_Peer";
static const char *initialize_type = "vrpn_Mutex Initialize";

struct losePeerData {
    vrpn_Connection *connection;
    vrpn_PeerMutex *mutex;
};

// Mostly copied from vrpn_Connection.C's vrpn_getmyIP() and some man pages.
// Should return in host order.
// Returns 0 on error.
static vrpn_uint32 getmyIP(const char *NICaddress = NULL)
{
    struct hostent *host;
    char myname[100];
    in_addr in;
    int retval;

    if (NICaddress) {
        // First, see if NIC is specified as IP address.
        // inet_addr() comes back in network order
        in.s_addr = inet_addr(NICaddress);
        if (in.s_addr != INADDR_NONE) {
            return ntohl(in.s_addr);
        }
        // Second, try a name.
        // gethostbyname() comes back in network order
        host = gethostbyname(NICaddress);
        if (host) {
            memcpy(&in.s_addr, host->h_addr, host->h_length);
            return ntohl(in.s_addr);
        }

        fprintf(stderr, "getmyIP:  Can't get host entry for %s.\n", NICaddress);
        return 0;
    }

    retval = gethostname(myname, sizeof(myname));
    if (retval) {
        fprintf(stderr, "getmyIP:  Couldn't determine local hostname.\n");
        return 0;
    }

    // gethostname() is guaranteed to produce something gethostbyname() can
    // parse.
    // gethostbyname() comes back in network order
    host = gethostbyname(myname);
    if (!host) {
        fprintf(stderr, "getmyIP:  Couldn't find host by name (%s).\n", myname);
        return 0;
    }

    memcpy(&in.s_addr, host->h_addr, host->h_length);
    return ntohl(in.s_addr);
}

vrpn_Mutex::vrpn_Mutex(const char *name, vrpn_Connection *c)
    : d_connection(c)
{
    char *servicename;

    servicename = vrpn_copy_service_name(name);

    if (c) {
        c->addReference();
        d_myId = c->register_sender(servicename);
        d_requestIndex_type = c->register_message_type(requestIndex_type);
        d_requestMutex_type = c->register_message_type(requestMutex_type);
        d_release_type = c->register_message_type(release_type);
        d_releaseNotification_type =
            c->register_message_type(releaseNotification_type);
        d_grantRequest_type = c->register_message_type(grantRequest_type);
        d_denyRequest_type = c->register_message_type(denyRequest_type);
        d_initialize_type = c->register_message_type(initialize_type);
    }

    if (servicename) {
      try {
        delete[] servicename;
      } catch (...) {
        fprintf(stderr, "vrpn_Mutex::vrpn_Mutex(): delete failed\n");
        return;
      }
    }
}

// virtual
vrpn_Mutex::~vrpn_Mutex(void)
{
    if (d_connection) {
        d_connection->removeReference(); // possibly destroy connection.
    }
}

void vrpn_Mutex::mainloop(void)
{

    if (d_connection) {
        d_connection->mainloop();
    }
}

void vrpn_Mutex::sendRequest(vrpn_int32 index)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    if (!d_connection) return;
    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, index);
    d_connection->pack_message(32 - bl, now, d_requestMutex_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::sendRelease(void)
{
    timeval now;

    if (!d_connection) return;

    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(0, now, d_release_type, d_myId, NULL,
                               vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::sendReleaseNotification(void)
{
    timeval now;

    if (!d_connection) return;
    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(0, now, d_releaseNotification_type, d_myId, NULL,
                               vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::sendGrantRequest(vrpn_int32 index)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    if (!d_connection) return;
    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, index);
    d_connection->pack_message(32 - bl, now, d_grantRequest_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
}

void vrpn_Mutex::sendDenyRequest(vrpn_int32 index)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    if (!d_connection) return;
    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, index);
    d_connection->pack_message(32 - bl, now, d_denyRequest_type, d_myId, buffer,
                               vrpn_CONNECTION_RELIABLE);
}

vrpn_Mutex_Server::vrpn_Mutex_Server(const char *name, vrpn_Connection *c)
    : vrpn_Mutex(name, c)
    , d_state(FREE)
    , d_remoteIndex(0)
{
    vrpn_int32 got;
    vrpn_int32 droppedLast;

    if (c) {
        c->register_handler(d_requestIndex_type, handle_requestIndex, this);
        c->register_handler(d_requestMutex_type, handle_requestMutex, this);
        c->register_handler(d_release_type, handle_release, this);

        got = c->register_message_type(vrpn_got_connection);
        c->register_handler(got, handle_gotConnection, this);
        droppedLast = c->register_message_type(vrpn_dropped_last_connection);
        c->register_handler(droppedLast, handle_dropLastConnection, this);
    }
}

// virtual
vrpn_Mutex_Server::~vrpn_Mutex_Server(void)
{
    if (d_connection) {
        vrpn_int32 got, droppedLast;
        got = d_connection->register_message_type(vrpn_got_connection);
        droppedLast =
            d_connection->register_message_type(vrpn_dropped_last_connection);
        d_connection->unregister_handler(d_requestIndex_type,
                                         handle_requestIndex, this);
        d_connection->unregister_handler(d_requestMutex_type,
                                         handle_requestMutex, this);
        d_connection->unregister_handler(d_release_type, handle_release, this);
        d_connection->unregister_handler(got, handle_gotConnection, this);
        d_connection->unregister_handler(droppedLast, handle_dropLastConnection,
                                         this);
    }
}

// static
int vrpn_Mutex_Server::handle_requestMutex(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Mutex_Server *me = (vrpn_Mutex_Server *)userdata;
    const char *b = p.buffer;
    vrpn_int32 remoteId;

    vrpn_unbuffer(&b, &remoteId);

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Server::handle_request from %d.\n", remoteId);
#endif

    if (me->d_state == FREE) {
        me->d_state = HELD;

        // BUG BUG BUG - how does the Mutex_Remote recognize that this grant
        // is for it, not for some other?
        me->sendGrantRequest(remoteId);

        return 0;
    }
    else {
        me->sendDenyRequest(remoteId);

        return 0;
    }
}

// static
int vrpn_Mutex_Server::handle_release(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Mutex_Server *me = (vrpn_Mutex_Server *)userdata;

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Server::handle_release.\n");
#endif

    me->d_state = FREE;
    me->sendReleaseNotification();

    return 0;
}

// static
int vrpn_Mutex_Server::handle_requestIndex(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Mutex_Server *me = (vrpn_Mutex_Server *)userdata;

    timeval now;
    vrpn_int32 msg_len = sizeof(vrpn_int32) + p.payload_len;
    char *buffer = NULL;
    try { buffer = new char[msg_len]; }
    catch (...) { return -1; }
    char *b = buffer;
    vrpn_int32 bl = msg_len;

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Server::handle_requestIndex:  "
                    "Initializing client %d (%lu %d).\n",
            me->d_remoteIndex, ntohl(*(vrpn_uint32 *)p.buffer),
            ntohl(*(vrpn_int32 *)(p.buffer + sizeof(vrpn_uint32))));
#endif

    if (me->d_connection) {
        vrpn_gettimeofday(&now, NULL);
        // echo back whatever the client gave us as a unique identifier
        vrpn_buffer(&b, &bl, p.buffer, p.payload_len);
        vrpn_buffer(&b, &bl, (me->d_remoteIndex));
        me->d_connection->pack_message(msg_len, now, me->d_initialize_type,
                                       me->d_myId, buffer,
                                       vrpn_CONNECTION_RELIABLE);
    }

    me->d_remoteIndex++;
    try {
      delete[] buffer;
    } catch (...) {
      fprintf(stderr, "vrpn_Mutex_Server::handle_requestIndex(): delete failed\n");
      return -1;
    }
    return 0;
}

// static
int vrpn_Mutex_Server::handle_gotConnection(void *, vrpn_HANDLERPARAM)
{
    return 0;
    // return handle_requestIndex(userdata, p);
}

// static
int vrpn_Mutex_Server::handle_dropLastConnection(void *userdata,
                                                 vrpn_HANDLERPARAM)
{
    vrpn_Mutex_Server *me = (vrpn_Mutex_Server *)userdata;

    // Force the lock to release, to avoid deadlock.
    if (me->d_state == HELD) {
        fprintf(stderr, "vrpn_Mutex_Server::handle_dropLastConnection:  "
                        "Forcing the state to FREE to avoid deadlock.\n");
    }

    me->d_state = FREE;

    return 0;
}

vrpn_Mutex_Remote::vrpn_Mutex_Remote(const char *name, vrpn_Connection *c)
    : vrpn_Mutex(name, c ? c : ((strcmp(name, "null") == 0)
                                    ? (vrpn_Connection *)NULL
                                    : vrpn_get_connection_by_name(name)))
    , d_state(AVAILABLE)
    , d_myIndex(-1)
    , d_requestBeforeInit(vrpn_FALSE)
    , d_reqGrantedCB(NULL)
    , d_reqDeniedCB(NULL)
    , d_takeCB(NULL)
    , d_releaseCB(NULL)
{

    if (d_connection) {
        d_connection->register_handler(d_grantRequest_type, handle_grantRequest,
                                       this);
        d_connection->register_handler(d_denyRequest_type, handle_denyRequest,
                                       this);
        d_connection->register_handler(d_releaseNotification_type,
                                       handle_releaseNotification, this);
        d_connection->register_handler(d_initialize_type, handle_initialize,
                                       this);
        if (d_connection->connected()) {
            requestIndex();
        }
        vrpn_int32 got =
            d_connection->register_message_type(vrpn_got_connection);
        d_connection->register_handler(got, handle_gotConnection, this);
    }
}

// virtual
vrpn_Mutex_Remote::~vrpn_Mutex_Remote(void)
{

    // Make sure we don't deadlock things
    release();

    if (d_connection) {
        d_connection->unregister_handler(d_grantRequest_type,
                                         handle_grantRequest, this);
        d_connection->unregister_handler(d_denyRequest_type, handle_denyRequest,
                                         this);
        d_connection->unregister_handler(d_releaseNotification_type,
                                         handle_releaseNotification, this);
        d_connection->unregister_handler(d_initialize_type, handle_initialize,
                                         this);
        vrpn_int32 got =
            d_connection->register_message_type(vrpn_got_connection);
        d_connection->unregister_handler(got, handle_gotConnection, this);
    }
}

vrpn_bool vrpn_Mutex_Remote::isAvailable(void) const
{
    return (d_state == AVAILABLE);
}

vrpn_bool vrpn_Mutex_Remote::isHeldLocally(void) const
{
    return (d_state == OURS);
}

vrpn_bool vrpn_Mutex_Remote::isHeldRemotely(void) const
{
    return (d_state == HELD_REMOTELY);
}

void vrpn_Mutex_Remote::requestIndex(void)
{
    timeval now;
    vrpn_int32 buflen = sizeof(vrpn_int32) + sizeof(vrpn_uint32);
    char *buf = NULL;
    try { buf = new char[buflen]; }
    catch (...) { return; }
    char *bufptr = buf;
    vrpn_int32 len = buflen;
    vrpn_uint32 ip_addr = getmyIP();
#ifdef _WIN32
    vrpn_int32 pid = _getpid();
#else
    vrpn_int32 pid = getpid();
#endif
    vrpn_buffer(&bufptr, &len, ip_addr);
    vrpn_buffer(&bufptr, &len, pid);
#ifdef VERBOSE
    printf("requesting index for %lu, %d\n", ip_addr, pid);
#endif
    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(buflen, now, d_requestIndex_type, d_myId, buf,
                               vrpn_CONNECTION_RELIABLE);
    try {
      delete[] buf;
    } catch (...) {
      fprintf(stderr, "vrpn_Mutex_Remote::requestIndex(): delete failed\n");
      return;
    }
    return;
}

void vrpn_Mutex_Remote::request(void)
{
    if (!isAvailable()) {

#ifdef VERBOSE
        fprintf(stderr, "Requested unavailable mutex.\n");
#endif
        triggerDenyCallbacks();
        return;
    }
    else if (d_myIndex == -1) {
#ifdef VERBOSE
        fprintf(stderr, "Requested mutex before initialization; deferring.\n");
#endif
        d_requestBeforeInit = vrpn_TRUE;
        return;
    }

#ifdef VERBOSE
    fprintf(stderr, "Requesting mutex\n");
#endif

    d_state = REQUESTING;
    sendRequest(d_myIndex);

    return;
}

void vrpn_Mutex_Remote::release(void)
{
    if (!isHeldLocally()) {
        return;
    }

#ifdef VERBOSE
    fprintf(stderr, "Releasing mutex.\n");
#endif

    d_state = AVAILABLE;
    sendRelease();
    triggerReleaseCallbacks();
}

vrpn_bool vrpn_Mutex_Remote::addRequestGrantedCallback(void *userdata,
                                                  int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_Mutex_Remote::addRequestGrantedCallback:  "
                        "Out of memory.\n");
        return false;
    }

    cb->userdata = userdata;
    cb->f = f;
    cb->next = d_reqGrantedCB;
    d_reqGrantedCB = cb;
    return true;
}

vrpn_bool vrpn_Mutex_Remote::addRequestDeniedCallback(void *userdata,
                                                 int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_Mutex_Remote::addRequestDeniedCallback:  "
                        "Out of memory.\n");
        return false;
    }

    cb->userdata = userdata;
    cb->f = f;
    cb->next = d_reqDeniedCB;
    d_reqDeniedCB = cb;
    return true;
}

vrpn_bool vrpn_Mutex_Remote::addTakeCallback(void *userdata, int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr,
                "vrpn_Mutex_Remote::addTakeCallback:  Out of memory.\n");
        return false;
    }

    cb->userdata = userdata;
    cb->f = f;
    cb->next = d_takeCB;
    d_takeCB = cb;
    return true;
}

// static
vrpn_bool vrpn_Mutex_Remote::addReleaseCallback(void *userdata, int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_Mutex_Remote::addReleaseCallback:  "
                        "Out of memory.\n");
        return false;
    }

    cb->userdata = userdata;
    cb->f = f;
    cb->next = d_releaseCB;
    d_releaseCB = cb;
    return true;
}

// static
int vrpn_Mutex_Remote::handle_grantRequest(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Mutex_Remote *me = (vrpn_Mutex_Remote *)userdata;
    const char *b = p.buffer;
    vrpn_int32 index;

    vrpn_unbuffer(&b, &index);

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Remote::handle_grantRequest for %d.\n", index);
#endif

    if (me->d_myIndex != index) {
        me->d_state = HELD_REMOTELY;
        me->triggerTakeCallbacks();
        return 0;
    }

    me->d_state = OURS;
    me->triggerGrantCallbacks();
    me->triggerTakeCallbacks();

    return 0;
}

// static
int vrpn_Mutex_Remote::handle_denyRequest(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Mutex_Remote *me = (vrpn_Mutex_Remote *)userdata;
    const char *b = p.buffer;
    vrpn_int32 index;

    vrpn_unbuffer(&b, &index);

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Remote::handle_denyRequest for %d.\n", index);
#endif

    if (me->d_myIndex != index) {
        return 0;
    }

    me->d_state = HELD_REMOTELY;
    me->triggerDenyCallbacks();

    return 0;
}

// static
int vrpn_Mutex_Remote::handle_releaseNotification(void *userdata,
                                                  vrpn_HANDLERPARAM)
{
    vrpn_Mutex_Remote *me = (vrpn_Mutex_Remote *)userdata;

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Remote::handle_releaseNotification.\n");
#endif

    me->d_state = AVAILABLE;
    me->triggerReleaseCallbacks();

    return 0;
}

// static
int vrpn_Mutex_Remote::handle_initialize(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Mutex_Remote *me = (vrpn_Mutex_Remote *)userdata;
    const char *b = p.buffer;

    // Only pay attention to the first initialize() message we get
    // after startup.
    if (me->d_myIndex != -1) {
        return 0;
    }

    vrpn_int32 expected_payload_len =
        2 * sizeof(vrpn_int32) + sizeof(vrpn_uint32);
    if (p.payload_len != expected_payload_len) {
        fprintf(stderr,
                "vrpn_Mutex_Remote::handle_initialize: "
                "Warning: Ignoring message with length %d, expected %d\n",
                p.payload_len, expected_payload_len);
        return 0;
    }

    vrpn_uint32 ip_addr;
    vrpn_int32 pid;
    vrpn_unbuffer(&b, &ip_addr);
    vrpn_unbuffer(&b, &pid);
    vrpn_int32 my_pid = 0;
#ifdef _WIN32
    my_pid = _getpid();
#else
    my_pid = getpid();
#endif
    if ((ip_addr != getmyIP()) || (pid != my_pid)) {
        fprintf(
            stderr,
            "vrpn_Mutex_Remote::handle_initialize: "
            "Warning: Ignoring message that doesn't match ip/pid identifier\n");
#ifdef VERBOSE
        fprintf(stderr, "Got %lu %d, expected %lu %d.\n", ip_addr, pid,
                getmyIP(), my_pid);
#endif
        return 0;
    }
    vrpn_unbuffer(&b, &(me->d_myIndex));

#ifdef VERBOSE
    fprintf(stderr, "vrpn_Mutex_Remote::handle_initialize:  "
                    "Got assigned index %d.\n",
            me->d_myIndex);
#endif

    if (me->d_requestBeforeInit) {
#ifdef VERBOSE
        fprintf(stderr, "vrpn_Mutex_Remote::handle_initialize:  "
                        "Sending request\n");
#endif
        me->request();
    }

    return 0;
}

// static
int vrpn_Mutex_Remote::handle_gotConnection(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Mutex_Remote *me = (vrpn_Mutex_Remote *)userdata;
    if (me->d_myIndex == -1) {
        me->requestIndex();
    }
    return 0;
}

void vrpn_Mutex_Remote::triggerGrantCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_reqGrantedCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_Mutex_Remote::triggerDenyCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_reqDeniedCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_Mutex_Remote::triggerTakeCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_takeCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_Mutex_Remote::triggerReleaseCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_releaseCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

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

vrpn_PeerMutex::vrpn_PeerMutex(const char *name, int port,
                               const char *NICaddress)
    : d_state(AVAILABLE)
    , d_server(NULL)
    , d_peer(NULL)
    , d_numPeers(0)
    , d_numConnectionsAllocated(0)
    , d_myIP(getmyIP(NICaddress))
    , d_myPort(port)
    , d_holderIP(0)
    , d_holderPort(-1)
    , d_reqGrantedCB(NULL)
    , d_reqDeniedCB(NULL)
    , d_takeCB(NULL)
    , d_releaseCB(NULL)
    , d_peerData(NULL)
{

    if (!name) {
        fprintf(stderr, "vrpn_PeerMutex:  NULL name!\n");
        return;
    }
    // XXX Won't work with non-IP connections (MPI, for example)
    char con_name[512];
    sprintf(con_name, "%s:%d", NICaddress, port);
    d_server = vrpn_create_server_connection(con_name);
    if (d_server) {
        d_server->addReference();
        d_server->setAutoDeleteStatus(true);
    }
    else {
        fprintf(stderr, "vrpn_PeerMutex:  "
                        "Couldn't open connection on port %d!\n",
                port);
        return;
    }

    init(name);
}

vrpn_PeerMutex::vrpn_PeerMutex(const char *name, vrpn_Connection *server)
    : d_state(AVAILABLE)
    , d_server(server)
    , d_peer(NULL)
    , d_numPeers(0)
    , d_numConnectionsAllocated(0)
    , d_myIP(getmyIP(NULL))
    , d_myPort(0)
    , d_holderIP(0)
    , d_holderPort(-1)
    , d_reqGrantedCB(NULL)
    , d_reqDeniedCB(NULL)
    , d_takeCB(NULL)
    , d_releaseCB(NULL)
    , d_peerData(NULL)
{

    if (!name) {
        fprintf(stderr, "vrpn_PeerMutex:  NULL name!\n");
        return;
    }
    if (server) {
        d_server->addReference();
    }
    else {
        fprintf(stderr, "vrpn_PeerMutex:  NULL connection!\n");
        return;
    }

    init(name);
}

vrpn_PeerMutex::~vrpn_PeerMutex(void)
{

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
      try {
        delete[] d_mutexName;
      } catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::~vrpn_PeerMutex(): delete failed\n");
        return;
      }
    }
    for (int i = 0; i < d_numPeers; ++i) {
        if (d_peer[i]) {
            d_peer[i]->removeReference();
        }
    }
    if (d_peer) {
      try {
        delete[] d_peer;
      } catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::~vrpn_PeerMutex(): delete failed\n");
        return;
      }
    }

    if (d_server) {
        d_server->removeReference();
    }
}

vrpn_bool vrpn_PeerMutex::isAvailable(void) const
{
    return (d_state == AVAILABLE);
}

vrpn_bool vrpn_PeerMutex::isHeldLocally(void) const
{
    return (d_state == OURS);
}

vrpn_bool vrpn_PeerMutex::isHeldRemotely(void) const
{
    return (d_state == HELD_REMOTELY);
}

int vrpn_PeerMutex::numPeers(void) const { return d_numPeers; }

void vrpn_PeerMutex::mainloop(void)
{
    int i;

    d_server->mainloop();
    for (i = 0; i < d_numPeers; i++) {
        d_peer[i]->mainloop();
    }

    checkGrantMutex();
}

void vrpn_PeerMutex::request(void)
{
    int i;

    // No point in sending requests if it's not currently available.
    // However, we need to trigger any local denial callbacks;  otherwise
    // this looks like a silent failure.
    if (!isAvailable()) {
        triggerDenyCallbacks();

#ifdef VERBOSE
        fprintf(stderr,
                "vrpn_PeerMutex::request:  the mutex isn't available.\n");
#endif

        return;
    }

    d_state = REQUESTING;
    d_numPeersGrantingLock = 0;
    for (i = 0; i < d_numPeers; i++) {
        // d_peerData[i].grantedLock = VRPN_FALSE;
        sendRequest(d_peer[i]);
    }

    // If somebody else sends a request before we get all our grants,
    // and their IP/port# is lower than ours, we have to yield to them.

    d_holderIP = d_myIP;
    d_holderPort = d_myPort;

    // We used to wait until the next pass through mainloop() to check
    // this (our request could be trivially granted if we have no
    // peers), but that leads to bad things (TM), like having to
    // insert extra calls to mainloop() in client code just to guarantee
    // that we have the mutex.

    checkGrantMutex();

#ifdef VERBOSE
    fprintf(stderr, "vrpn_PeerMutex::request:  requested the mutex "
                    "(from %d peers).\n",
            d_numPeers);
#endif
}

void vrpn_PeerMutex::release(void)
{
    int i;

    // Can't release it if we don't already have it.
    // There aren't any appropriate callbacks to trigger here.  :)
    if (!isHeldLocally()) {

#ifdef VERBOSE
        fprintf(stderr, "vrpn_PeerMutex::release:  we don't hold the mutex.\n");
#endif

        return;
    }

    d_state = AVAILABLE;
    d_holderIP = 0;
    d_holderPort = -1;
    for (i = 0; i < d_numPeers; i++) {
        sendRelease(d_peer[i]);
    }

    triggerReleaseCallbacks();

#ifdef VERBOSE
    fprintf(stderr, "vrpn_PeerMutex::release:  released the mutex.\n");
#endif
}

vrpn_bool vrpn_PeerMutex::addPeer(const char *stationName)
{
    vrpn_Connection **newc;
    peerData *newg;
    losePeerData *d;
    int i;

    // complex
    while (d_numPeers >= d_numConnectionsAllocated) {

        // reallocate arrays
        d_numConnectionsAllocated = 2 * (d_numConnectionsAllocated + 1);
        try {
          newc = new vrpn_Connection *[d_numConnectionsAllocated];
          newg = new peerData[d_numConnectionsAllocated];
        } catch (...) {
            fprintf(stderr, "vrpn_PeerMutex::addPeer:  Out of memory.\n");
            return false;
        }
        for (i = 0; i < d_numPeers; i++) {
            newc[i] = d_peer[i];
            newg[i] = d_peerData[i];
        }
        if (d_peer) {
            try {
              delete[] d_peer;
            } catch (...) {
              fprintf(stderr, "vrpn_PeerMutex::addPeer(): delete failed\n");
              return false;
            }
        }
        if (d_peerData) {
          try {
            delete[] d_peerData;
          } catch (...) {
            fprintf(stderr, "vrpn_PeerMutex::addPeer(): delete failed\n");
            return false;
          }
        }
        d_peer = newc;
        d_peerData = newg;
    }
    d_peer[d_numPeers] = vrpn_get_connection_by_name(stationName);
    // d_peerData[d_numPeers].grantedLock = vrpn_false;

    try { d = new losePeerData; }
    catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::addPeer:  Out of memory.\n");
        return false;
    }
    d->connection = d_peer[d_numPeers];
    d->mutex = this;
    vrpn_int32 control;
    vrpn_int32 drop;
    control = d_peer[d_numPeers]->register_sender(vrpn_CONTROL);
    drop = d_peer[d_numPeers]->register_message_type(vrpn_dropped_connection);
    d_peer[d_numPeers]->register_handler(drop, handle_losePeer, d, control);

#ifdef VERBOSE
    fprintf(stderr, "vrpn_PeerMutex::addPeer:  added peer named %s.\n",
            stationName);
#endif

    d_numPeers++;
    return true;
}

vrpn_bool vrpn_PeerMutex::addRequestGrantedCallback(void *ud, int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::addRequestGrantedCallback:  "
                        "Out of memory.\n");
        return false;
    }

    cb->userdata = ud;
    cb->f = f;
    cb->next = d_reqGrantedCB;
    d_reqGrantedCB = cb;
    return true;
}

vrpn_bool vrpn_PeerMutex::addRequestDeniedCallback(void *ud, int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::addRequestDeniedCallback:  "
                        "Out of memory.\n");
        return false;
    }

    cb->userdata = ud;
    cb->f = f;
    cb->next = d_reqDeniedCB;
    d_reqDeniedCB = cb;
    return true;
}

vrpn_bool vrpn_PeerMutex::addTakeCallback(void *ud, int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::addTakeCallback:  Out of memory.\n");
        return false;
    }

    cb->userdata = ud;
    cb->f = f;
    cb->next = d_takeCB;
    d_takeCB = cb;
    return true;
}

// static
vrpn_bool vrpn_PeerMutex::addReleaseCallback(void *ud, int (*f)(void *))
{
    mutexCallback *cb = NULL;
    try { cb = new mutexCallback; }
    catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::addReleaseCallback:  "
                        "Out of memory.\n");
        return false;
    }

    cb->userdata = ud;
    cb->f = f;
    cb->next = d_releaseCB;
    d_releaseCB = cb;
    return true;
}

// static
int vrpn_PeerMutex::handle_request(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_PeerMutex *me = (vrpn_PeerMutex *)userdata;
    const char *b = p.buffer;
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
    fprintf(stderr,
            "vrpn_PeerMutex::handle_request:  got one from %s port %d.\n",
            inet_ntoa(nad), senderPort);
#endif

    // If several nodes request the lock at once, ties are broken in favor
    // of the node with the lowest IP number & port.

    if ((me->d_state == AVAILABLE) ||
        (((me->d_state == HELD_REMOTELY) || (me->d_state == REQUESTING)) &&
         ((senderIP < me->d_holderIP) ||
          ((senderIP == me->d_holderIP) &&
           ((vrpn_int32)senderPort < me->d_holderPort))))) {

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
int vrpn_PeerMutex::handle_release(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_PeerMutex *me = (vrpn_PeerMutex *)userdata;
    const char *b = p.buffer;
    vrpn_uint32 senderIP;
    vrpn_uint32 senderPort;

    vrpn_unbuffer(&b, &senderIP);
    vrpn_unbuffer(&b, &senderPort);

#ifdef VERBOSE
    in_addr nad;

    nad.s_addr = senderIP;
    fprintf(stderr,
            "vrpn_PeerMutex::handle_release:  got one from %s port %d.\n",
            inet_ntoa(nad), senderPort);
#endif

    if ((senderIP != me->d_holderIP) ||
        ((vrpn_int32)senderPort != me->d_holderPort)) {
        fprintf(stderr, "vrpn_PeerMutex::handle_release:  Got a release from "
                        "somebody who didn't have the lock!?\n");
    }

    me->d_state = AVAILABLE;
    me->d_holderIP = 0;
    me->d_holderPort = -1;

    me->triggerReleaseCallbacks();

    return 0;
}

// static
int vrpn_PeerMutex::handle_grantRequest(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_PeerMutex *me = (vrpn_PeerMutex *)userdata;
    const char *b = p.buffer;
    vrpn_uint32 senderIP;
    vrpn_uint32 senderPort;

    vrpn_unbuffer(&b, &senderIP);
    vrpn_unbuffer(&b, &senderPort);

#ifdef VERBOSE
    in_addr nad;
    nad.s_addr = senderIP;
    fprintf(stderr, "vrpn_PeerMutex::handle_grantRequest:  "
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
int vrpn_PeerMutex::handle_denyRequest(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_PeerMutex *me = (vrpn_PeerMutex *)userdata;
    const char *b = p.buffer;
    vrpn_uint32 senderIP;
    vrpn_uint32 senderPort;

    vrpn_unbuffer(&b, &senderIP);
    vrpn_unbuffer(&b, &senderPort);

#ifdef VERBOSE
    in_addr nad;
    nad.s_addr = senderIP;
    fprintf(stderr, "vrpn_PeerMutex::handle_denyRequest:  "
                    "got one for %s port %d.\n",
            inet_ntoa(nad), senderPort);
#endif

    if ((senderIP != me->d_myIP) || (senderPort != me->d_myPort)) {
        return 0;
    }

    me->d_numPeersGrantingLock = 0;

    me->triggerDenyCallbacks();

    me->d_state = HELD_REMOTELY;

    return 0;
}

// static
int vrpn_PeerMutex::handle_losePeer(void *userdata, vrpn_HANDLERPARAM)
{
    losePeerData *data = (losePeerData *)userdata;
    vrpn_PeerMutex *me = data->mutex;
    vrpn_Connection *c = data->connection;
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
        fprintf(stderr,
                "vrpn_PeerMutex::handle_losePeer:  Can't find lost peer.\n");
        return 0; // -1?
    }

    fprintf(stderr, "vrpn_PeerMutex::handle_losePeer:  lost peer #%d.\n", i);

    if (me->d_peer[i]) {
        me->d_peer[i]->removeReference();
    }
    me->d_numPeers--;
    me->d_peer[i] = me->d_peer[me->d_numPeers];

    try {
      delete data;
    } catch (...) {
      fprintf(stderr, "vrpn_PeerMutex::handle_losePeer(): delete failed\n");
      return -1;
    }

    return 0;
}

void vrpn_PeerMutex::sendRequest(vrpn_Connection *c)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, d_myIP);
    vrpn_buffer(&b, &bl, d_myPort);
    c->pack_message(32 - bl, now, c->register_message_type(requestMutex_type),
                    c->register_sender(d_mutexName), buffer,
                    vrpn_CONNECTION_RELIABLE);
}

void vrpn_PeerMutex::sendRelease(vrpn_Connection *c)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, d_myIP);
    vrpn_buffer(&b, &bl, d_myPort);
    c->pack_message(32 - bl, now, c->register_message_type(release_type),
                    c->register_sender(d_mutexName), buffer,
                    vrpn_CONNECTION_RELIABLE);
}

void vrpn_PeerMutex::sendGrantRequest(vrpn_Connection *c, vrpn_uint32 IP,
                                      vrpn_uint32 port)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, IP);
    vrpn_buffer(&b, &bl, port);
    c->pack_message(32 - bl, now, c->register_message_type(grantRequest_type),
                    c->register_sender(d_mutexName), buffer,
                    vrpn_CONNECTION_RELIABLE);
}

void vrpn_PeerMutex::sendDenyRequest(vrpn_Connection *c, vrpn_uint32 IP,
                                     vrpn_uint32 port)
{
    timeval now;
    char buffer[32];
    char *b = buffer;
    vrpn_int32 bl = 32;

    vrpn_gettimeofday(&now, NULL);
    vrpn_buffer(&b, &bl, IP);
    vrpn_buffer(&b, &bl, port);
    c->pack_message(32 - bl, now, c->register_message_type(denyRequest_type),
                    c->register_sender(d_mutexName), buffer,
                    vrpn_CONNECTION_RELIABLE);
}

void vrpn_PeerMutex::triggerGrantCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_reqGrantedCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_PeerMutex::triggerDenyCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_reqDeniedCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_PeerMutex::triggerTakeCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_takeCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_PeerMutex::triggerReleaseCallbacks(void)
{
    mutexCallback *cb;

    // trigger callbacks
    for (cb = d_releaseCB; cb; cb = cb->next) {
        (cb->f)(cb->userdata);
    }
}

void vrpn_PeerMutex::checkGrantMutex(void)
{

    if ((d_state == REQUESTING) && (d_numPeersGrantingLock == d_numPeers)) {
        d_state = OURS;

        triggerTakeCallbacks();
        triggerGrantCallbacks();
    }
}

void vrpn_PeerMutex::init(const char *name)
{
    d_mutexName = NULL;
    try {
      size_t n = 1 + strlen(name);
      d_mutexName = new char[n];
      // This is guaranteed to fit because of the new allocation above.
      strncpy(d_mutexName, name, n);
    } catch (...) {
        fprintf(stderr, "vrpn_PeerMutex::init:  Out of memory.\n");
    }

    d_myId = d_server->register_sender(name);
    d_request_type = d_server->register_message_type(requestMutex_type);
    d_release_type = d_server->register_message_type(release_type);
    d_grantRequest_type = d_server->register_message_type(grantRequest_type);
    d_denyRequest_type = d_server->register_message_type(denyRequest_type);

    d_server->register_handler(d_request_type, handle_request, this, d_myId);
    d_server->register_handler(d_release_type, handle_release, this, d_myId);
    d_server->register_handler(d_grantRequest_type, handle_grantRequest, this,
                               d_myId);
    d_server->register_handler(d_denyRequest_type, handle_denyRequest, this,
                               d_myId);
}
