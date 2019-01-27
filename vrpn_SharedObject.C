#include <stdio.h>  // for fprintf, stderr, sprintf
#include <string.h> // for NULL, strcpy, strlen, etc

#include "vrpn_Connection.h"   // for vrpn_Connection, etc
#include "vrpn_LamportClock.h" // for vrpn_LamportTimestamp, etc
#include "vrpn_SharedObject.h"

struct timeval;

// We can't put d_lastUpdate in the message header timestamps;  it must
// go in the body.
// This is because we're (probably) using a synchronized connection,
// and VRPN clock sync munges these timestamps.
// If we send a message from A to B and it gets sent back to A, attempting
// to preserve the timestamp at both ends, but sending it over a synchronized
// connection, its timestamp will end up equal to the original timestamp
// PLUS the network RTT (round-trip time).
// The original design intent of a vrpn_SharedObject was to be able to use
// timestamps as message identifiers/sequencers, so they need to be invariant
// over several network hops.

// Note on Lamport clocks:  the implementation was never completed
// (see vrpn_LamportClock.[C,h]), so it's use here has been disabled in the
// following ways.  First, the code that considers the Lamport clock
// in vrpn_Shared_int32::shouldAcceptUpdates(...) is commented out
// (the other shared objects, float64 and String, never had code to
// consider the Lamport clock).  Second, the body of the method
// vrpn_SharedObject::useLamportClock is commented out so that a
// Lamport clock is never started.

vrpn_SharedObject::vrpn_SharedObject(const char *name, const char *tname,
                                     vrpn_int32 mode)
    : d_name(NULL)
    , d_mode(mode)
    , d_typename(NULL)
    , d_connection(NULL)
    , d_serverId(-1)
    , d_remoteId(-1)
    , d_myId(-1)
    , d_peerId(-1)
    , d_update_type(-1)
    , d_requestSerializer_type(-1)
    , d_grantSerializer_type(-1)
    , d_assumeSerializer_type(-1)
    , d_lamportUpdate_type(-1)
    , d_isSerializer(vrpn_TRUE)
    , d_isNegotiatingSerializer(vrpn_FALSE)
    , d_queueSets(vrpn_FALSE)
    , d_lClock(NULL)
    , d_lastLamportUpdate(NULL)
    , d_deferredUpdateCallbacks(NULL)
{
    if (name) {
        d_name = new char[1 + strlen(name)];
        strcpy(d_name, name);
    }
    if (tname) {
        d_typename = new char[1 + strlen(tname)];
        strcpy(d_typename, tname);
    }
    vrpn_gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_SharedObject::~vrpn_SharedObject(void)
{
    vrpn_int32 gotConnection_type;

    if (d_name) {
      try {
        delete[] d_name;
      } catch (...) {
        fprintf(stderr, "vrpn_SharedObject::~vrpn_SharedObject(): delete failed\n");
        return;
      }
    }
    if (d_typename) {
      try {
        delete[] d_typename;
      } catch (...) {
        fprintf(stderr, "vrpn_SharedObject::~vrpn_SharedObject(): delete failed\n");
        return;
      }
    }
    if (d_connection) {
        d_connection->unregister_handler(d_update_type, handle_update, this,
                                         d_peerId);
        d_connection->unregister_handler(
            d_requestSerializer_type, handle_requestSerializer, this, d_peerId);
        d_connection->unregister_handler(
            d_grantSerializer_type, handle_grantSerializer, this, d_peerId);
        d_connection->unregister_handler(
            d_assumeSerializer_type, handle_assumeSerializer, this, d_peerId);

        gotConnection_type =
            d_connection->register_message_type(vrpn_got_connection);
        d_connection->unregister_handler(gotConnection_type,
                                         handle_gotConnection, this, d_myId);

        d_connection->removeReference();
    }
}

const char *vrpn_SharedObject::name(void) const { return d_name; }

vrpn_bool vrpn_SharedObject::isSerializer(void) const { return VRPN_TRUE; }

// virtual
void vrpn_SharedObject::bindConnection(vrpn_Connection *c)
{
    char buffer[101];
    if (c == NULL) {
        // unbind the connection
        if (d_connection) {
            d_connection->removeReference();
        }
        d_connection = NULL;
        return;
    }

    if (c && d_connection) {
        fprintf(stderr, "vrpn_SharedObject::bindConnection:  "
                        "Tried to rebind a connection to %s.\n",
                d_name);
        return;
    }

    d_connection = c;
    c->addReference();
    sprintf(buffer, "vrpn Shared server %s %s", d_typename, d_name);
    d_serverId = c->register_sender(buffer);
    sprintf(buffer, "vrpn Shared peer %s %s", d_typename, d_name);
    d_remoteId = c->register_sender(buffer);
    // d_updateFromServer_type = c->register_message_type
    //("vrpn_Shared update_from_server");
    // d_updateFromRemote_type = c->register_message_type
    //("vrpn_Shared update_from_remote");
    d_update_type = c->register_message_type("vrpn_Shared update");

    // fprintf (stderr, "My name is %s;  myId %d, ufs type %d, ufr type %d.\n",
    // buffer, d_myId, d_updateFromServer_type, d_updateFromRemote_type);

    d_requestSerializer_type =
        c->register_message_type("vrpn_Shared request_serializer");
    d_grantSerializer_type =
        c->register_message_type("vrpn_Shared grant_serializer");
    d_assumeSerializer_type =
        c->register_message_type("vrpn_Shared assume_serializer");
}

void vrpn_SharedObject::useLamportClock(vrpn_LamportClock *)
{

    // NOTE:  this is disabled
    // d_lClock = l;
}

void vrpn_SharedObject::becomeSerializer(void)
{
    timeval now;

    // Make sure only one request is outstanding

    if (d_isNegotiatingSerializer) {
        return;
    }

    d_isNegotiatingSerializer = vrpn_TRUE;

    // send requestSerializer

    if (d_connection) {
        vrpn_gettimeofday(&now, NULL);
        d_connection->pack_message(0, d_lastUpdate, d_requestSerializer_type,
                                   d_myId, NULL, vrpn_CONNECTION_RELIABLE);
    }

    // fprintf(stderr, "sent requestSerializer\n");
}

vrpn_bool vrpn_SharedObject::registerDeferredUpdateCallback(
    vrpnDeferredUpdateCallback cb, void *userdata)
{
    deferredUpdateCallbackEntry *x;

    try { x = new deferredUpdateCallbackEntry; }
    catch (...) {
        fprintf(stderr, "vrpn_SharedObject::registerDeferredUpdateCallback:  "
                        "Out of memory!\n");
        return false;
    }

    x->handler = cb;
    x->userdata = userdata;
    x->next = d_deferredUpdateCallbacks;
    d_deferredUpdateCallbacks = x;
    return true;
}

// virtual
vrpn_bool vrpn_SharedObject::shouldSendUpdate(vrpn_bool isLocalSet,
                                              vrpn_bool acceptedUpdate)
{

    // fprintf(stderr, "In vrpn_SharedObject::shouldSendUpdate(%s).\n", d_name);

    // Should handle all modes other than VRPN_SO_DEFER_UPDATES.
    if (acceptedUpdate && isLocalSet) {
        // fprintf(stderr, "..... accepted;  local set => broadcast it.\n");
        return vrpn_TRUE;
    }

    // Not a local set or (not accepted and not serializing)
    if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
        // fprintf(stderr, "..... rejected (NOT serialized).\n");
        return vrpn_FALSE;
    }

    if (!d_isSerializer && isLocalSet) {
        // fprintf(stderr, "..... not serializer;  local set "
        //"=> send it to the serializer.\n");
        return vrpn_TRUE;
    }

    if (d_isSerializer && !isLocalSet && acceptedUpdate) {
        // fprintf(stderr, "..... serializer;  remote set;  accepted =>
        // broadcast it.\n");
        return vrpn_TRUE;
    }

    // fprintf(stderr,"..... rejected (under serialization).\n");
    return vrpn_FALSE;
}

// static
int vrpn_SharedObject::handle_requestSerializer(void *userdata,
                                                vrpn_HANDLERPARAM)
{
    vrpn_SharedObject *s = (vrpn_SharedObject *)userdata;
    timeval now;

    if (!s->isSerializer() || s->d_isNegotiatingSerializer) {
        // ignore this
        // we should probably return failure or error?
        return 0;
    }

    s->d_isNegotiatingSerializer = vrpn_TRUE;

    if (s->d_connection) {

        // Don't set d_isSerializer to FALSE until they've assumed it.
        // Until then, retain the serializer status but queue all of our
        // messages;  when they finish becoming the serializer, we
        // set our flag to false and send the queue of set()s we've
        // received to them.

        // send grantSerializer

        vrpn_gettimeofday(&now, NULL);
        s->d_connection->pack_message(0, s->d_lastUpdate,
                                      s->d_grantSerializer_type, s->d_myId,
                                      NULL, vrpn_CONNECTION_RELIABLE);
    }

    // start queueing set()s

    s->d_queueSets = vrpn_TRUE;

    // fprintf(stderr, "sent grantSerializer\n");
    return 0;
}

// static
int vrpn_SharedObject::handle_grantSerializer(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_SharedObject *s = (vrpn_SharedObject *)userdata;
    timeval now;

    s->d_isSerializer = vrpn_TRUE;
    s->d_isNegotiatingSerializer = vrpn_FALSE;

    // send assumeSerializer

    if (s->d_connection) {
        vrpn_gettimeofday(&now, NULL);
        s->d_connection->pack_message(0, s->d_lastUpdate,
                                      s->d_assumeSerializer_type, s->d_myId,
                                      NULL, vrpn_CONNECTION_RELIABLE);
    }

    // fprintf(stderr, "sent assumeSerializer\n");
    return 0;
}

// static
int vrpn_SharedObject::handle_assumeSerializer(void *userdata,
                                               vrpn_HANDLERPARAM)
{
    vrpn_SharedObject *s = (vrpn_SharedObject *)userdata;

    s->d_isSerializer = vrpn_FALSE;
    s->d_isNegotiatingSerializer = vrpn_FALSE;

    // TODO:  send queued set()s

    return 0;
}

int vrpn_SharedObject::yankDeferredUpdateCallbacks(void)
{
    deferredUpdateCallbackEntry *x;

    for (x = d_deferredUpdateCallbacks; x; x = x->next) {
        if ((x->handler)(x->userdata)) {
            return -1;
        }
    }

    return 0;
}

void vrpn_SharedObject::serverPostBindCleanup(void)
{
    d_myId = d_serverId;
    d_peerId = d_remoteId;
    postBindCleanup();
}

void vrpn_SharedObject::remotePostBindCleanup(void)
{
    d_myId = d_remoteId;
    d_peerId = d_serverId;
    postBindCleanup();
}

// static
int vrpn_SharedObject::handle_gotConnection(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_SharedObject *s = (vrpn_SharedObject *)userdata;

    // Send an update to the remote so that they have the correct
    // initial value for our state.  Otherwise an attempt to synchronize
    // something will assume we're at our initial value until our value
    // changes, which is an error.

    if (s->d_isSerializer) {
        s->sendUpdate();
        // fprintf(stderr, "%s: set client's value.\n", s->d_name);
    }
    else if (!(s->d_mode & VRPN_SO_DEFER_UPDATES) &&
             (s->d_myId == s->d_serverId)) {
        s->sendUpdate();
        // fprintf(stderr, "%s: set remote's value.\n", s->d_name);
    }

    return 0;
}

// static
int vrpn_SharedObject::handle_update(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_SharedObject *s = (vrpn_SharedObject *)userdata;

    return s->handleUpdate(p);
}

void vrpn_SharedObject::postBindCleanup(void)
{
    vrpn_int32 gotConnection_type;

    if (!d_connection) {
        return;
    }

    // listen for update

    d_connection->register_handler(d_update_type, handle_update, this,
                                   d_peerId);

    // listen for request/grant/assumeSerializer

    d_connection->register_handler(d_requestSerializer_type,
                                   handle_requestSerializer, this, d_peerId);
    d_connection->register_handler(d_grantSerializer_type,
                                   handle_grantSerializer, this, d_peerId);
    d_connection->register_handler(d_assumeSerializer_type,
                                   handle_assumeSerializer, this, d_peerId);

    gotConnection_type =
        d_connection->register_message_type(vrpn_got_connection);
    d_connection->register_handler(gotConnection_type, handle_gotConnection,
                                   this, d_myId);
}

vrpn_Shared_int32::vrpn_Shared_int32(const char *name, vrpn_int32 defaultValue,
                                     vrpn_int32 mode)
    : vrpn_SharedObject(name, "int32", mode)
    , d_value(defaultValue)
    , d_callbacks(NULL)
    , d_timedCallbacks(NULL)
    , d_policy(vrpn_ACCEPT)
    , d_policyCallback(NULL)
    , d_policyUserdata(NULL)
{
}

// virtual
vrpn_Shared_int32::~vrpn_Shared_int32(void)
{
    // if (d_connection) {
    // d_connection->unregister_handler(d_becomeSerializer_type,
    // handle_becomeSerializer, this, d_myId);
    //}
}

vrpn_int32 vrpn_Shared_int32::value(void) const { return d_value; }

vrpn_Shared_int32::operator vrpn_int32() const { return value(); }

vrpn_Shared_int32 &vrpn_Shared_int32::operator=(vrpn_int32 newValue)
{
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    return set(newValue, now);
}

vrpn_Shared_int32 &vrpn_Shared_int32::set(vrpn_int32 newValue, timeval when)
{
    return set(newValue, when, vrpn_TRUE);
}
vrpn_bool vrpn_Shared_int32::register_handler(vrpnSharedIntCallback cb,
                                         void *userdata)
{
    callbackEntry *e;
    try { e = new callbackEntry; }
    catch (...) {
        fprintf(stderr, "vrpn_Shared_int32::register_handler:  "
                        "Out of memory.\n");
        return false;
    }
    e->handler = cb;
    e->userdata = userdata;
    e->next = d_callbacks;
    d_callbacks = e;
    return true;
}

void vrpn_Shared_int32::unregister_handler(vrpnSharedIntCallback cb,
                                           void *userdata)
{
    callbackEntry *e, **snitch;

    snitch = &d_callbacks;
    e = *snitch;
    while (e && (e->handler != cb) && (e->userdata != userdata)) {
        snitch = &(e->next);
        e = *snitch;
    }
    if (!e) {
        fprintf(stderr, "vrpn_Shared_int32::unregister_handler:  "
                        "Handler not found.\n");
        return;
    }

    *snitch = e->next;
    try {
      delete e;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_int32::unregister_handler(): delete failed\n");
      return;
    }
}

vrpn_bool vrpn_Shared_int32::register_handler(vrpnTimedSharedIntCallback cb,
                                         void *userdata)
{
    timedCallbackEntry *e;
    try { e = new timedCallbackEntry; }
    catch (...) {
        fprintf(stderr, "vrpn_Shared_int32::register_handler:  "
                        "Out of memory.\n");
        return false;
    }
    e->handler = cb;
    e->userdata = userdata;
    e->next = d_timedCallbacks;
    d_timedCallbacks = e;
    return true;
}

void vrpn_Shared_int32::unregister_handler(vrpnTimedSharedIntCallback cb,
                                           void *userdata)
{
    timedCallbackEntry *e, **snitch;

    snitch = &d_timedCallbacks;
    e = *snitch;
    while (e && (e->handler != cb) && (e->userdata != userdata)) {
        snitch = &(e->next);
        e = *snitch;
    }
    if (!e) {
        fprintf(stderr, "vrpn_Shared_int32::unregister_handler:  "
                        "Handler not found.\n");
        return;
    }

    *snitch = e->next;
    try {
      delete e;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_int32::unregister_handler(): delete failed\n");
      return;
    }
}

void vrpn_Shared_int32::setSerializerPolicy(vrpn_SerializerPolicy policy,
                                            vrpnSharedIntSerializerPolicy f,
                                            void *userdata)
{
    d_policy = policy;
    d_policyCallback = f;
    d_policyUserdata = userdata;
}

vrpn_Shared_int32 &vrpn_Shared_int32::set(vrpn_int32 newValue, timeval when,
                                          vrpn_bool isLocalSet,
                                          vrpn_LamportTimestamp *t)
{
    vrpn_bool acceptedUpdate;

    acceptedUpdate = shouldAcceptUpdate(newValue, when, isLocalSet, t);
    if (acceptedUpdate) {
        d_value = newValue;
        d_lastUpdate = when;
        // yankCallbacks(isLocalSet);
    }

    if (shouldSendUpdate(isLocalSet, acceptedUpdate)) {
        sendUpdate(newValue, when);
    }

    // yankCallbacks is placed after sendUpdate so that the update goes on the
    // network before the updates due to local callbacks.
    if (acceptedUpdate) yankCallbacks(isLocalSet);

    return *this;
}

// virtual
vrpn_bool vrpn_Shared_int32::shouldAcceptUpdate(vrpn_int32 newValue,
                                                timeval when,
                                                vrpn_bool isLocalSet,
                                                vrpn_LamportTimestamp *)
{

    // fprintf(stderr, "In vrpn_Shared_int32::shouldAcceptUpdate(%s).\n",
    // d_name);

    /*
      // this commented-out code uses Lamport logical clocks, and is
      // disabled now b/c the implementation of Lamport clocks was
      // never complete.  We use standard timestamps instead.
    vrpn_bool old, equal;
    if (t) {
      old = d_lastLamportUpdate && (*t < *d_lastLamportUpdate);
      equal = d_lastLamportUpdate && (*t == *d_lastLamportUpdate);
    } else {
      old = !vrpn_TimevalGreater(when, d_lastUpdate);
      equal = vrpn_TimevalEqual( when, d_lastUpdate );
    }
    */
    vrpn_bool old = !vrpn_TimevalGreater(when, d_lastUpdate);
    vrpn_bool equal = vrpn_TimevalEqual(when, d_lastUpdate);

    // Is this "new" change idempotent?
    if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) && (newValue == d_value)) {
        // fprintf(stderr, "... was idempotent.\n");
        return vrpn_FALSE;
    }

    // Is this "new" change older than the previous change?
    if ((d_mode & VRPN_SO_IGNORE_OLD) && old) {
        // fprintf(stderr, "... was outdated.\n");
        return vrpn_FALSE;
    }
    // Is this "new" change older than the previous change?
    if ((d_mode & VRPN_SO_IGNORE_OLD) && old) {

        // If the timestamps of the new & previous changes are equal:
        //  - if we are the serializer, we can accept the local change
        //  - if we are not the serializer, local updates are to be rejected
        if (equal) {
            if (!d_isSerializer && isLocalSet) {
                return vrpn_FALSE;
            }
        }
        else
            return vrpn_FALSE;
    }

    // All other clauses of shouldAcceptUpdate depend on serialization
    if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
        return vrpn_TRUE;
    }

    // fprintf(stderr, "... serializing:  ");

    // If we're not the serializer, don't accept local set() calls -
    // forward those to the serializer.  Non-local set() calls are
    // messages from the serializer that we should accept.
    if (!d_isSerializer) {
        if (isLocalSet) {
            // fprintf(stderr, "local update, not serializer, so reject.\n");
            yankDeferredUpdateCallbacks();
            return vrpn_FALSE;
        }
        else {
            // fprintf(stderr, "remote update, not serializer, so accept.\n");
            return vrpn_TRUE;
        }
    }

    // We are the serializer.
    // fprintf(stderr, "serializer:  ");

    if (isLocalSet) {
        // fprintf(stderr, "local update.\n");
        if (d_policy == vrpn_DENY_LOCAL) {
            return vrpn_FALSE;
        }
        else {
            return vrpn_TRUE;
        }
    }

    // Are we accepting all updates?
    if (d_policy == vrpn_ACCEPT) {
        // fprintf(stderr, "policy is to accept.\n");
        return vrpn_TRUE;
    }

    // Does the user want to accept this one?
    if ((d_policy == vrpn_CALLBACK) && d_policyCallback &&
        (*d_policyCallback)(d_policyUserdata, newValue, when, this)) {
        // fprintf(stderr, "user callback accepts.\n");
        return vrpn_TRUE;
    }

    // fprintf(stderr, "rejected.\n");
    return vrpn_FALSE;
}

void vrpn_Shared_int32::encode(char **buffer, vrpn_int32 *len,
                               vrpn_int32 newValue, timeval when) const
{
    vrpn_buffer(buffer, len, newValue);
    vrpn_buffer(buffer, len, when);
}
void vrpn_Shared_int32::encodeLamport(char **buffer, vrpn_int32 *len,
                                      vrpn_int32 newValue, timeval when,
                                      vrpn_LamportTimestamp *t) const
{
    int i;
    vrpn_buffer(buffer, len, newValue);
    vrpn_buffer(buffer, len, when);
    vrpn_buffer(buffer, len, t->size());
    for (i = 0; i < t->size(); i++) {
        vrpn_buffer(buffer, len, (*t)[i]);
    }
}

void vrpn_Shared_int32::decode(const char **buffer, vrpn_int32 *,
                               vrpn_int32 *newValue, timeval *when) const
{
    vrpn_unbuffer(buffer, newValue);
    vrpn_unbuffer(buffer, when);
}
void vrpn_Shared_int32::decodeLamport(const char **buffer, vrpn_int32 *,
                                      vrpn_int32 *newValue, timeval *when,
                                      vrpn_LamportTimestamp **t) const
{
    vrpn_uint32 size;
    vrpn_uint32 *array;
    unsigned int i;

    vrpn_unbuffer(buffer, newValue);
    vrpn_unbuffer(buffer, when);
    vrpn_unbuffer(buffer, &size);
    try { array = new vrpn_uint32[size]; }
    catch (...) {
      *t = NULL;
      return;
    }
    for (i = 0; i < size; i++) {
        vrpn_unbuffer(buffer, &array[i]);
    }
    vrpn_LamportTimestamp *ret = NULL;
    try { ret = new vrpn_LamportTimestamp(size, array); }
    catch (...) {}
    *t = ret;
    try {
      delete[] array;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_int32::decodeLamport(): delete failed\n");
      return;
    }
}

void vrpn_Shared_int32::sendUpdate(void) { sendUpdate(d_value, d_lastUpdate); }

void vrpn_Shared_int32::sendUpdate(vrpn_int32 newValue, timeval when)
{
    char buffer[32];
    vrpn_int32 buflen = 32;
    char *bp = buffer;

    if (d_connection) {
        if (d_lClock) {
            vrpn_LamportTimestamp *t;
            t = d_lClock->getTimestampAndAdvance();
            encodeLamport(&bp, &buflen, newValue, when, t);
            try {
              delete t;
            } catch (...) {
              fprintf(stderr, "vrpn_Shared_int32::sendUpdate(): delete failed\n");
              return;
            }
        } else {
            encode(&bp, &buflen, newValue, when);
        }
        d_connection->pack_message(32 - buflen, d_lastUpdate, d_update_type,
                                   d_myId, buffer, vrpn_CONNECTION_RELIABLE);
        // fprintf(stderr, "vrpn_Shared_int32::sendUpdate:  packed message of %d
        // "
        //"at %d:%d.\n", d_value, d_lastUpdate.tv_sec, d_lastUpdate.tv_usec);
    }
}

int vrpn_Shared_int32::yankCallbacks(vrpn_bool isLocal)
{
    callbackEntry *e;
    timedCallbackEntry *te;

    for (e = d_callbacks; e; e = e->next) {
        if ((*e->handler)(e->userdata, d_value, isLocal)) {
            return -1;
        }
    }
    for (te = d_timedCallbacks; te; te = te->next) {
        if ((*te->handler)(te->userdata, d_value, d_lastUpdate, isLocal)) {
            return -1;
        }
    }

    return 0;
}

int vrpn_Shared_int32::handleUpdate(vrpn_HANDLERPARAM p)
{
    vrpn_int32 newValue;
    timeval when;

    decode(&p.buffer, &p.payload_len, &newValue, &when);

    // fprintf(stderr, "%s::handleUpdate to %d at %d:%d.\n",
    // d_name, newValue, when.tv_sec, when.tv_usec);

    set(newValue, when, vrpn_FALSE);

    // fprintf(stderr, "vrpn_Shared_int32::handleUpdate done\n");
    return 0;
}

// static
int vrpn_Shared_int32::handle_lamportUpdate(void *ud, vrpn_HANDLERPARAM p)
{
    vrpn_Shared_int32 *s = (vrpn_Shared_int32 *)ud;
    vrpn_LamportTimestamp *t;
    vrpn_int32 newValue;
    timeval when;

    s->decodeLamport(&p.buffer, &p.payload_len, &newValue, &when, &t);

    // fprintf(stderr, "vrpn_Shared_int32::handleUpdate to %d at %d:%d.\n",
    // newValue, when.tv_sec, when.tv_usec);

    s->d_lClock->receive(*t);

    s->set(newValue, when, vrpn_FALSE, t);

    if (s->d_lastLamportUpdate) {
      try {
        delete s->d_lastLamportUpdate;
      } catch (...) {
        fprintf(stderr, "vrpn_Shared_int32::handle_lamportUpdate(): delete failed\n");
        return -1;
      }
    }
    s->d_lastLamportUpdate = t;

    // fprintf(stderr, "vrpn_Shared_int32::handleUpdate done\n");
    return 0;
}

vrpn_Shared_int32_Server::vrpn_Shared_int32_Server(const char *name,
                                                   vrpn_int32 defaultValue,
                                                   vrpn_int32 defaultMode)
    : vrpn_Shared_int32(name, defaultValue, defaultMode)
{

    d_isSerializer = vrpn_TRUE;
}

// virtual
vrpn_Shared_int32_Server::~vrpn_Shared_int32_Server(void) {}

vrpn_Shared_int32_Server &vrpn_Shared_int32_Server::operator=(vrpn_int32 c)
{
    vrpn_Shared_int32::operator=(c);
    return *this;
}

// virtual
void vrpn_Shared_int32_Server::bindConnection(vrpn_Connection *c)
{
    vrpn_Shared_int32::bindConnection(c);

    serverPostBindCleanup();
}

vrpn_Shared_int32_Remote::vrpn_Shared_int32_Remote(const char *name,
                                                   vrpn_int32 defaultValue,
                                                   vrpn_int32 defaultMode)
    : vrpn_Shared_int32(name, defaultValue, defaultMode)
{
}

// virtual
vrpn_Shared_int32_Remote::~vrpn_Shared_int32_Remote(void) {}

vrpn_Shared_int32_Remote &vrpn_Shared_int32_Remote::operator=(vrpn_int32 c)
{
    vrpn_Shared_int32::operator=(c);
    return *this;
}

void vrpn_Shared_int32_Remote::bindConnection(vrpn_Connection *c)
{

    vrpn_Shared_int32::bindConnection(c);

    remotePostBindCleanup();
}

vrpn_Shared_float64::vrpn_Shared_float64(const char *name,
                                         vrpn_float64 defaultValue,
                                         vrpn_int32 mode)
    : vrpn_SharedObject(name, "float64", mode)
    , d_value(defaultValue)
    , d_callbacks(NULL)
    , d_timedCallbacks(NULL)
    , d_policy(vrpn_ACCEPT)
    , d_policyCallback(NULL)
    , d_policyUserdata(NULL)
{

    if (name) {
        strcpy(d_name, name);
    }
    vrpn_gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_Shared_float64::~vrpn_Shared_float64(void)
{
    // if (d_connection) {
    // d_connection->unregister_handler(d_becomeSerializer_type,
    // handle_becomeSerializer, this, d_myId);
    //}
}

vrpn_float64 vrpn_Shared_float64::value(void) const { return d_value; }

vrpn_Shared_float64::operator vrpn_float64() const { return value(); }

vrpn_Shared_float64 &vrpn_Shared_float64::operator=(vrpn_float64 newValue)
{
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    return set(newValue, now);
}

vrpn_Shared_float64 &vrpn_Shared_float64::set(vrpn_float64 newValue,
                                              timeval when)
{
    return set(newValue, when, vrpn_TRUE);
}

void vrpn_Shared_float64::register_handler(vrpnSharedFloatCallback cb,
                                           void *userdata)
{
    callbackEntry *e = new callbackEntry;
    if (!e) {
        fprintf(stderr, "vrpn_Shared_float64::register_handler:  "
                        "Out of memory.\n");
        return;
    }
    e->handler = cb;
    e->userdata = userdata;
    e->next = d_callbacks;
    d_callbacks = e;
}

void vrpn_Shared_float64::unregister_handler(vrpnSharedFloatCallback cb,
                                             void *userdata)
{
    callbackEntry *e, **snitch;

    snitch = &d_callbacks;
    e = *snitch;
    while (e && (e->handler != cb) && (e->userdata != userdata)) {
        snitch = &(e->next);
        e = *snitch;
    }
    if (!e) {
        fprintf(stderr, "vrpn_Shared_float64::unregister_handler:  "
                        "Handler not found.\n");
        return;
    }

    *snitch = e->next;
    try {
      delete e;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_float64::unregister_handler(): delete failed\n");
      return;
    }
}

void vrpn_Shared_float64::register_handler(vrpnTimedSharedFloatCallback cb,
                                           void *userdata)
{
    timedCallbackEntry *e = new timedCallbackEntry;
    if (!e) {
        fprintf(stderr, "vrpn_Shared_float64::register_handler:  "
                        "Out of memory.\n");
        return;
    }
    e->handler = cb;
    e->userdata = userdata;
    e->next = d_timedCallbacks;
    d_timedCallbacks = e;
}

void vrpn_Shared_float64::unregister_handler(vrpnTimedSharedFloatCallback cb,
                                             void *userdata)
{
    timedCallbackEntry *e, **snitch;

    snitch = &d_timedCallbacks;
    e = *snitch;
    while (e && (e->handler != cb) && (e->userdata != userdata)) {
        snitch = &(e->next);
        e = *snitch;
    }
    if (!e) {
        fprintf(stderr, "vrpn_Shared_float64::unregister_handler:  "
                        "Handler not found.\n");
        return;
    }

    *snitch = e->next;
    try {
      delete e;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_float64::unregister_handler(): delete failed\n");
      return;
    }
}

void vrpn_Shared_float64::setSerializerPolicy(vrpn_SerializerPolicy policy,
                                              vrpnSharedFloatSerializerPolicy f,
                                              void *userdata)
{
    d_policy = policy;
    d_policyCallback = f;
    d_policyUserdata = userdata;
}

vrpn_Shared_float64 &vrpn_Shared_float64::set(vrpn_float64 newValue,
                                              timeval when,
                                              vrpn_bool isLocalSet)
{
    vrpn_bool acceptedUpdate;

    acceptedUpdate = shouldAcceptUpdate(newValue, when, isLocalSet);
    if (acceptedUpdate) {
        d_value = newValue;
        d_lastUpdate = when;
        // yankCallbacks(isLocalSet);
    }

    if (shouldSendUpdate(isLocalSet, acceptedUpdate)) {
        sendUpdate(newValue, when);
    }

    // yankCallbacks is placed after sendUpdate so that the update goes on the
    // network before the updates due to local callbacks.
    if (acceptedUpdate) yankCallbacks(isLocalSet);

    return *this;
}

// virtual
vrpn_bool vrpn_Shared_float64::shouldAcceptUpdate(vrpn_float64 newValue,
                                                  timeval when,
                                                  vrpn_bool isLocalSet)
{

    // Is this "new" change idempotent?
    if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) && (newValue == d_value)) {
        return vrpn_FALSE;
    }

    // Is this "new" change older than the previous change?
    if ((d_mode & VRPN_SO_IGNORE_OLD) &&
        !vrpn_TimevalGreater(when, d_lastUpdate)) {

        // If the timestamps of the new & previous changes are equal:
        //  - if we are the serializer, we can accept the local change
        //  - if we are not the serializer, local updates are to be rejected
        if (vrpn_TimevalEqual(when, d_lastUpdate)) {
            if (!d_isSerializer && isLocalSet) {
                return vrpn_FALSE;
            }
        }
        else
            return vrpn_FALSE;
    }

    // All other clauses of shouldAcceptUpdate depend on serialization
    if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
        return vrpn_TRUE;
    }

    // If we're not the serializer, don't accept local set() calls -
    // forward those to the serializer.  Non-local set() calls are
    // messages from the serializer that we should accept.
    if (!d_isSerializer) {
        if (isLocalSet) {
            yankDeferredUpdateCallbacks();
            return vrpn_FALSE;
        }
        else {
            return vrpn_TRUE;
        }
    }

    // We are the serializer.

    if (isLocalSet) {
        if (d_policy == vrpn_DENY_LOCAL) {
            return vrpn_FALSE;
        }
        else {
            return vrpn_TRUE;
        }
    }

    // Are we accepting all updates?
    if (d_policy == vrpn_ACCEPT) {
        return vrpn_TRUE;
    }

    // Does the user want to accept this one?
    if ((d_policy == vrpn_CALLBACK) && d_policyCallback &&
        (*d_policyCallback)(d_policyUserdata, newValue, when, this)) {
        return vrpn_TRUE;
    }

    return vrpn_FALSE;
}

void vrpn_Shared_float64::encode(char **buffer, vrpn_int32 *len,
                                 vrpn_float64 newValue, timeval when) const
{
    vrpn_buffer(buffer, len, newValue);
    vrpn_buffer(buffer, len, when);
}
void vrpn_Shared_float64::decode(const char **buffer, vrpn_int32 *,
                                 vrpn_float64 *newValue, timeval *when) const
{
    vrpn_unbuffer(buffer, newValue);
    vrpn_unbuffer(buffer, when);
}

void vrpn_Shared_float64::sendUpdate(void)
{
    sendUpdate(d_value, d_lastUpdate);
}

void vrpn_Shared_float64::sendUpdate(vrpn_float64 newValue, timeval when)
{
    char buffer[32];
    vrpn_int32 buflen = 32;
    char *bp = buffer;

    if (d_connection) {
        encode(&bp, &buflen, newValue, when);
        d_connection->pack_message(32 - buflen, d_lastUpdate, d_update_type,
                                   d_myId, buffer, vrpn_CONNECTION_RELIABLE);
        // fprintf(stderr, "vrpn_Shared_float64::sendUpdate:  packed
        // message\n");
    }
}

int vrpn_Shared_float64::yankCallbacks(vrpn_bool isLocal)
{
    callbackEntry *e;
    timedCallbackEntry *te;

    for (e = d_callbacks; e; e = e->next) {
        if ((*e->handler)(e->userdata, d_value, isLocal)) {
            return -1;
        }
    }
    for (te = d_timedCallbacks; te; te = te->next) {
        if ((*te->handler)(te->userdata, d_value, d_lastUpdate, isLocal)) {
            return -1;
        }
    }

    return 0;
}

int vrpn_Shared_float64::handleUpdate(vrpn_HANDLERPARAM p)
{
    vrpn_float64 newValue;
    timeval when;

    decode(&p.buffer, &p.payload_len, &newValue, &when);

    set(newValue, when, vrpn_FALSE);

    return 0;
}

vrpn_Shared_float64_Server::vrpn_Shared_float64_Server(
    const char *name, vrpn_float64 defaultValue, vrpn_int32 defaultMode)
    : vrpn_Shared_float64(name, defaultValue, defaultMode)
{

    d_isSerializer = vrpn_TRUE;
}

// virtual
vrpn_Shared_float64_Server::~vrpn_Shared_float64_Server(void) {}

vrpn_Shared_float64_Server &vrpn_Shared_float64_Server::
operator=(vrpn_float64 c)
{
    vrpn_Shared_float64::operator=(c);
    return *this;
}

// virtual
void vrpn_Shared_float64_Server::bindConnection(vrpn_Connection *c)
{
    vrpn_Shared_float64::bindConnection(c);

    serverPostBindCleanup();
}

vrpn_Shared_float64_Remote::vrpn_Shared_float64_Remote(
    const char *name, vrpn_float64 defaultValue, vrpn_int32 defaultMode)
    : vrpn_Shared_float64(name, defaultValue, defaultMode)
{
}

// virtual
vrpn_Shared_float64_Remote::~vrpn_Shared_float64_Remote(void) {}

vrpn_Shared_float64_Remote &vrpn_Shared_float64_Remote::
operator=(vrpn_float64 c)
{
    vrpn_Shared_float64::operator=(c);
    return *this;
}

// virtual
void vrpn_Shared_float64_Remote::bindConnection(vrpn_Connection *c)
{

    vrpn_Shared_float64::bindConnection(c);

    remotePostBindCleanup();
}

vrpn_Shared_String::vrpn_Shared_String(const char *name,
                                       const char *defaultValue,
                                       vrpn_int32 mode)
    : vrpn_SharedObject(name, "String", mode)
    , d_value(defaultValue ? new char[1 + strlen(defaultValue)] : NULL)
    , d_callbacks(NULL)
    , d_timedCallbacks(NULL)
    , d_policy(vrpn_ACCEPT)
    , d_policyCallback(NULL)
    , d_policyUserdata(NULL)
{

    if (defaultValue) {
        strcpy(d_value, defaultValue);
    }
    if (name) {
        strcpy(d_name, name);
    }
    vrpn_gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_Shared_String::~vrpn_Shared_String(void)
{
    if (d_value) {
      try {
        delete[] d_value;
      } catch (...) {
        fprintf(stderr, "vrpn_Shared_String::~vrpn_Shared_String(): delete failed\n");
        return;
      }
    }
    // if (d_connection) {
    // d_connection->unregister_handler(d_becomeSerializer_type,
    // handle_becomeSerializer, this, d_myId);
    //}
}

const char *vrpn_Shared_String::value(void) const { return d_value; }

vrpn_Shared_String::operator const char *() const { return value(); }

vrpn_Shared_String &vrpn_Shared_String::operator=(const char *newValue)
{
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    return set(newValue, now);
}

vrpn_Shared_String &vrpn_Shared_String::set(const char *newValue, timeval when)
{
    return set(newValue, when, vrpn_TRUE);
}

vrpn_bool vrpn_Shared_String::register_handler(vrpnSharedStringCallback cb,
                                          void *userdata)
{
    callbackEntry *e;
    try { e = new callbackEntry; }
    catch (...) {
        fprintf(stderr, "vrpn_Shared_String::register_handler:  "
                        "Out of memory.\n");
        return false;
    }
    e->handler = cb;
    e->userdata = userdata;
    e->next = d_callbacks;
    d_callbacks = e;
    return true;
}

void vrpn_Shared_String::unregister_handler(vrpnSharedStringCallback cb,
                                            void *userdata)
{
    callbackEntry *e, **snitch;

    snitch = &d_callbacks;
    e = *snitch;
    while (e && (e->handler != cb) && (e->userdata != userdata)) {
        snitch = &(e->next);
        e = *snitch;
    }
    if (!e) {
        fprintf(stderr, "vrpn_Shared_String::unregister_handler:  "
                        "Handler not found.\n");
        return;
    }

    *snitch = e->next;
    try {
      delete e;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_String::unregister_handler(): delete failed\n");
      return;
    }
}

vrpn_bool vrpn_Shared_String::register_handler(vrpnTimedSharedStringCallback cb,
                                          void *userdata)
{
    timedCallbackEntry *e;
    try { e = new timedCallbackEntry; }
    catch (...) {
        fprintf(stderr, "vrpn_Shared_String::register_handler:  "
                        "Out of memory.\n");
        return false;
    }
    e->handler = cb;
    e->userdata = userdata;
    e->next = d_timedCallbacks;
    d_timedCallbacks = e;
    return true;
}

void vrpn_Shared_String::unregister_handler(vrpnTimedSharedStringCallback cb,
                                            void *userdata)
{
    timedCallbackEntry *e, **snitch;

    snitch = &d_timedCallbacks;
    e = *snitch;
    while (e && (e->handler != cb) && (e->userdata != userdata)) {
        snitch = &(e->next);
        e = *snitch;
    }
    if (!e) {
        fprintf(stderr, "vrpn_Shared_String::unregister_handler:  "
                        "Handler not found.\n");
        return;
    }

    *snitch = e->next;
    try {
      delete e;
    } catch (...) {
      fprintf(stderr, "vrpn_Shared_String::unregister_handler(): delete failed\n");
      return;
    }
}

void vrpn_Shared_String::setSerializerPolicy(vrpn_SerializerPolicy policy,
                                             vrpnSharedStringSerializerPolicy f,
                                             void *userdata)
{
    d_policy = policy;
    d_policyCallback = f;
    d_policyUserdata = userdata;
}

vrpn_Shared_String &vrpn_Shared_String::set(const char *newValue, timeval when,
                                            vrpn_bool isLocalSet)
{
    vrpn_bool acceptedUpdate;

    acceptedUpdate = shouldAcceptUpdate(newValue, when, isLocalSet);
    if (acceptedUpdate) {

        if ((d_value == NULL) || (strcmp(d_value, newValue) != 0)) {
            if (d_value) {
              try {
                delete[] d_value;
              } catch (...) {
                fprintf(stderr, "vrpn_Shared_String::set(): delete failed\n");
                return *this;
              }
            }
            try { d_value = new char[1 + strlen(newValue)]; }
            catch (...) {
                fprintf(stderr, "vrpn_Shared_String::set:  Out of memory.\n");
                return *this;
            }
            strcpy(d_value, newValue);
        }

        // fprintf(stderr, "vrpn_Shared_String::set:  %s to \"%s\".\n", name(),
        // value());

        d_lastUpdate = when;
    }

    if (shouldSendUpdate(isLocalSet, acceptedUpdate)) {
        sendUpdate(newValue, when);
    }

    // yankCallbacks is placed after sendUpdate so that the update goes on the
    // network before the updates due to local callbacks.
    if (acceptedUpdate) yankCallbacks(isLocalSet);

    return *this;
}

// virtual
vrpn_bool vrpn_Shared_String::shouldAcceptUpdate(const char *newValue,
                                                 timeval when,
                                                 vrpn_bool isLocalSet)
{

    // Is this "new" change idempotent?
    if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) && (newValue == d_value)) {
        return vrpn_FALSE;
    }

    // Is this "new" change older than the previous change?
    if ((d_mode & VRPN_SO_IGNORE_OLD) &&
        !vrpn_TimevalGreater(when, d_lastUpdate)) {

        // If the timestamps of the new & previous changes are equal:
        //  - if we are the serializer, we can accept the local change
        //  - if we are not the serializer, local updates are to be rejected
        if (vrpn_TimevalEqual(when, d_lastUpdate)) {
            if (!d_isSerializer && isLocalSet) {
                return vrpn_FALSE;
            }
        }
        else
            return vrpn_FALSE;
    }

    // All other clauses of shouldAcceptUpdate depend on serialization
    if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
        return vrpn_TRUE;
    }

    // If we're not the serializer, don't accept local set() calls -
    // forward those to the serializer.  Non-local set() calls are
    // messages from the serializer that we should accept.
    if (!d_isSerializer) {
        if (isLocalSet) {
            yankDeferredUpdateCallbacks();
            return vrpn_FALSE;
        }
        else {
            return vrpn_TRUE;
        }
    }

    // We are the serializer.

    if (isLocalSet) {
        if (d_policy == vrpn_DENY_LOCAL) {
            return vrpn_FALSE;
        }
        else {
            return vrpn_TRUE;
        }
    }

    // Are we accepting all updates?
    if (d_policy == vrpn_ACCEPT) {
        return vrpn_TRUE;
    }

    // Does the user want to accept this one?
    if ((d_policy == vrpn_CALLBACK) && d_policyCallback &&
        (*d_policyCallback)(d_policyUserdata, newValue, when, this)) {
        return vrpn_TRUE;
    }

    return vrpn_FALSE;
}

void vrpn_Shared_String::encode(char **buffer, vrpn_int32 *len,
                                const char *newValue, timeval when) const
{
    // We reverse ordering from the other vrpn_SharedObject classes
    // so that the time value is guaranteed to be aligned.
    vrpn_buffer(buffer, len, when);
    vrpn_buffer(buffer, len, newValue,
                static_cast<vrpn_int32>(strlen(newValue)));
}
void vrpn_Shared_String::decode(const char **buffer, vrpn_int32 *len,
                                char *newValue, timeval *when) const
{
    vrpn_unbuffer(buffer, when);
    vrpn_unbuffer(buffer, newValue, *len - sizeof(*when));
    newValue[*len - sizeof(*when)] = 0;
}

void vrpn_Shared_String::sendUpdate(void) { sendUpdate(d_value, d_lastUpdate); }

void vrpn_Shared_String::sendUpdate(const char *newValue, timeval when)
{
    char buffer[1024]; // HACK
    vrpn_int32 buflen = 1024;
    char *bp = buffer;

    if (d_connection) {
        encode(&bp, &buflen, newValue, when);
        d_connection->pack_message(1024 - buflen, d_lastUpdate, d_update_type,
                                   d_myId, buffer, vrpn_CONNECTION_RELIABLE);
        // fprintf(stderr, "vrpn_Shared_String::sendUpdate:  packed message\n");
    }
}

int vrpn_Shared_String::yankCallbacks(vrpn_bool isLocal)
{
    callbackEntry *e;
    timedCallbackEntry *te;

    for (e = d_callbacks; e; e = e->next) {
        if ((*e->handler)(e->userdata, d_value, isLocal)) {
            return -1;
        }
    }
    for (te = d_timedCallbacks; te; te = te->next) {
        if ((*te->handler)(te->userdata, d_value, d_lastUpdate, isLocal)) {
            return -1;
        }
    }

    return 0;
}

// static
int vrpn_Shared_String::handleUpdate(vrpn_HANDLERPARAM p)
{
    char newValue[1024]; // HACK
    timeval when;

    decode(&p.buffer, &p.payload_len, newValue, &when);

    set(newValue, when, vrpn_FALSE);

    return 0;
}

vrpn_Shared_String_Server::vrpn_Shared_String_Server(const char *name,
                                                     const char *defaultValue,
                                                     vrpn_int32 defaultMode)
    : vrpn_Shared_String(name, defaultValue, defaultMode)
{

    d_isSerializer = vrpn_TRUE;
}

// virtual
vrpn_Shared_String_Server::~vrpn_Shared_String_Server(void) {}

vrpn_Shared_String_Server &vrpn_Shared_String_Server::operator=(const char *c)
{
    vrpn_Shared_String::operator=(c);
    return *this;
}

// virtual
void vrpn_Shared_String_Server::bindConnection(vrpn_Connection *c)
{
    vrpn_Shared_String::bindConnection(c);

    serverPostBindCleanup();
}

vrpn_Shared_String_Remote::vrpn_Shared_String_Remote(const char *name,
                                                     const char *defaultValue,
                                                     vrpn_int32 defaultMode)
    : vrpn_Shared_String(name, defaultValue, defaultMode)
{
}

// virtual
vrpn_Shared_String_Remote::~vrpn_Shared_String_Remote(void) {}

vrpn_Shared_String_Remote &vrpn_Shared_String_Remote::operator=(const char *c)
{
    vrpn_Shared_String::operator=(c);
    return *this;
}

// virtual
void vrpn_Shared_String_Remote::bindConnection(vrpn_Connection *c)
{

    vrpn_Shared_String::bindConnection(c);

    remotePostBindCleanup();
}
