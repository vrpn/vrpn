#include "vrpn_SharedObject.h"

#include <string.h>

#include <vrpn_Connection.h>

vrpn_Shared_int32::vrpn_Shared_int32 (const char * name,
                                      vrpn_int32 defaultValue,
                                      vrpn_int32 mode) :
  d_value (defaultValue),
  d_name (name ? new char [1 + strlen(name)] : NULL),
  d_mode (mode),
  d_connection (NULL),
  d_myId (-1),
  d_updateFromServer_type (-1),
  d_updateFromRemote_type (-1),
  d_becomeSerializer_type (-1),
  d_callbacks (NULL),
  d_timedCallbacks (NULL),
  d_policy (ACCEPT),
  d_policyCallback (NULL),
  d_policyUserdata (NULL),
  d_isSerializer (vrpn_FALSE) {

  if (name) {
    strcpy(d_name, name);
  }
  gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_Shared_int32::~vrpn_Shared_int32 (void) {
  if (d_name) {
    delete [] d_name;
  }
  if (d_connection) {
    d_connection->unregister_handler(d_becomeSerializer_type,
                                     handle_becomeSerializer, this, d_myId);
  }
}

vrpn_int32 vrpn_Shared_int32::value (void) const {
  return d_value;
}

vrpn_Shared_int32::operator vrpn_int32 () const {
  return value();
}

vrpn_Shared_int32 & vrpn_Shared_int32::operator =
                                            (vrpn_int32 newValue) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return set(newValue, now);
}

vrpn_Shared_int32 & vrpn_Shared_int32::set (vrpn_int32 newValue,
                                            timeval when) {
  return set(newValue, when, vrpn_TRUE);
}

void vrpn_Shared_int32::bindConnection (vrpn_Connection * c) {
  char buffer [101];
  if (c == NULL) {
      // unbind the connection
      d_connection = NULL;
      return;
  }

  if (c && d_connection) {
    fprintf(stderr, "vrpn_Shared_int32::bindConnection:  "
                    "Tried to rebind a connection to %s.\n", d_name);
    return;
  }

  d_connection = c;
  sprintf(buffer, "vrpn Shared int32 %s", d_name);
  d_myId = c->register_sender(buffer);
  d_updateFromServer_type = c->register_message_type
          ("vrpn Shared update from server");
  d_updateFromRemote_type = c->register_message_type
          ("vrpn Shared update from remote");
  d_becomeSerializer_type = c->register_message_type
          ("vrpn Shared become serializer");

//printf ("My name is %s;  myId %d, ufs type %d, ufr type %d.\n",
//buffer, d_myId, d_updateFromServer_type, d_updateFromRemote_type);

  d_connection->register_handler(d_becomeSerializer_type,
                                 handle_becomeSerializer,
                                 this, d_myId);

}

void vrpn_Shared_int32::register_handler (vrpnSharedIntCallback cb,
                                          void * userdata) {
  callbackEntry * e = new callbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_int32::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_callbacks;
  d_callbacks = e;
}

void vrpn_Shared_int32::unregister_handler (vrpnSharedIntCallback cb,
                                            void * userdata) {
  callbackEntry * e, ** snitch;

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
  delete e;
}

void vrpn_Shared_int32::register_handler (vrpnTimedSharedIntCallback cb,
                                          void * userdata) {
  timedCallbackEntry * e = new timedCallbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_int32::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_timedCallbacks;
  d_timedCallbacks = e;
}

void vrpn_Shared_int32::unregister_handler (vrpnTimedSharedIntCallback cb,
                                            void * userdata) {
  timedCallbackEntry * e, ** snitch;

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
  delete e;
}

void vrpn_Shared_int32::setSerializerPolicy
              (vrpn_SerializerPolicy policy,
               vrpnSharedIntSerializerPolicy f,
               void * userdata) {
  d_policy = policy;
  d_policyCallback = f;
  d_policyUserdata = userdata;
}

vrpn_Shared_int32 & vrpn_Shared_int32::set (vrpn_int32 newValue,
                                            timeval when,
                                            vrpn_bool isLocalSet) {
  vrpn_bool acceptedUpdate;

  acceptedUpdate = shouldAcceptUpdate(newValue, when, isLocalSet);
  if (acceptedUpdate) {
    d_value = newValue;
    d_lastUpdate = when;
    yankCallbacks();
  }

  if (shouldSendUpdate(isLocalSet, acceptedUpdate)) {
    sendUpdate(d_myUpdate_type, newValue, when);
  }

  return *this;
}

//virtual
vrpn_bool vrpn_Shared_int32::shouldAcceptUpdate
                     (vrpn_int32 newValue, timeval when,
                      vrpn_bool isLocalSet) {

//fprintf(stderr, "In vrpn_Shared_int32::shouldAcceptUpdate().\n");

  // Is this "new" change idempotent?
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
//fprintf(stderr, "... was idempotent.\n");
    return vrpn_FALSE;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
//fprintf(stderr, "... was outdated.\n");
    return vrpn_FALSE;
  }

  // All other clauses of shouldAcceptUpdate depend on serialization
  if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
    return vrpn_TRUE;
  }

//fprintf(stderr, "... serializing:  ");

  // If we're not the serializer, don't accept local set() calls -
  // forward those to the serializer.  Non-local set() calls are
  // messages from the serializer that we should accept.
  if (!d_isSerializer) {
    if (isLocalSet) {
//fprintf(stderr, "local update, not serializer, so reject.\n");
      return vrpn_FALSE;
    } else {
//fprintf(stderr, "remote update, not serializer, so accept.\n");
      return vrpn_TRUE;
    }
  }

  // We are the serializer.
//fprintf(stderr, "serializer:  ");

  if (isLocalSet) {
//fprintf(stderr, "local update.\n");
    return vrpn_TRUE;
  }

  // Are we accepting all updates?
  if (d_policy == ACCEPT) {
//fprintf(stderr, "policy is to accept.\n");
    return vrpn_TRUE;
  }

  // Does the user want to accept this one?
  if ((d_policy == CALLBACK) && d_policyCallback &&
      (*d_policyCallback)(d_policyUserdata, newValue, when, this)) {
//fprintf(stderr, "user callback accepts.\n");
    return vrpn_TRUE;
  }

//fprintf(stderr, "rejected.\n");
  return vrpn_FALSE;
}

// virtual
vrpn_bool vrpn_Shared_int32::shouldSendUpdate
                     (vrpn_bool isLocalSet, vrpn_bool acceptedUpdate) {

//fprintf(stderr, "In vrpn_Shared_int32::shouldSendUpdate().\n");

  // should handle all modes other than VRPN_SO_DEFER_UPDATES
  if (acceptedUpdate && isLocalSet) {
//fprintf(stderr, "... accepted;  local set => broadcast it.\n");
    return vrpn_TRUE;
  }

  if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
    return vrpn_FALSE;
  }

  if (!d_isSerializer && isLocalSet) {
//fprintf(stderr, "... not serializer;  local set "
//"=> send it to the serializer.\n");
    return vrpn_TRUE;
  }

  if (d_isSerializer && !isLocalSet && acceptedUpdate) {
//fprintf(stderr, "... serializer;  remote set;  accepted => broadcast it.\n");
    return vrpn_TRUE;
  }

  return vrpn_FALSE;
}

void vrpn_Shared_int32::encode (char ** buffer, vrpn_int32 * len,
                                vrpn_int32 newValue, timeval when) const {
  vrpn_buffer(buffer, len, newValue);
  vrpn_buffer(buffer, len, when);
}
void vrpn_Shared_int32::decode (const char ** buffer, vrpn_int32 *,
                                vrpn_int32 * newValue, timeval * when) const {
  vrpn_unbuffer(buffer, newValue);
  vrpn_unbuffer(buffer, when);
}

void vrpn_Shared_int32::sendUpdate (vrpn_int32 msgType,
                                    vrpn_int32 newValue, timeval when) {
  char buffer [32];
  vrpn_int32 buflen = 32;
  char * bp = buffer;

  if (d_connection) {
    encode(&bp, &buflen, newValue, when);
    d_connection->pack_message(32 - buflen, d_lastUpdate,
                               msgType, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_int32::sendUpdate:  packed message of %d "
//"at %d:%d.\n", d_value, d_lastUpdate.tv_sec, d_lastUpdate.tv_usec);
  }

}


int vrpn_Shared_int32::yankCallbacks (void) {
  callbackEntry * e;
  timedCallbackEntry * te;

  for (e = d_callbacks;  e;  e = e->next) {
    if ((*e->handler)(e->userdata, d_value)) {
      return -1;
    }
  }
  for (te = d_timedCallbacks;  te;  te = te->next) {
    if ((*te->handler)(te->userdata, d_value, d_lastUpdate)) {
      return -1;
    }
  }

  return 0;
}

// static
int vrpn_Shared_int32::handle_becomeSerializer (void * userdata,
                                                vrpn_HANDLERPARAM p) {
  vrpn_Shared_int32 * s = (vrpn_Shared_int32 *) userdata;

  s->d_isSerializer = vrpn_TRUE;

  return 0;
}

// static
int vrpn_Shared_int32::handle_update (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_Shared_int32 * s = (vrpn_Shared_int32 *) ud;
  vrpn_int32 newValue;
  timeval when;

  s->decode(&p.buffer, &p.payload_len, &newValue, &when);

//fprintf(stderr, "vrpn_Shared_int32::handle_update to %d at %d:%d.\n",
//newValue, when.tv_sec, when.tv_usec);

  s->set(newValue, when, vrpn_FALSE);

//fprintf(stderr, "vrpn_Shared_int32::handle_update done\n");
  return 0;
}






vrpn_Shared_int32_Server::vrpn_Shared_int32_Server (const char * name,
                                                    vrpn_int32 defaultValue,
                                                    vrpn_int32 defaultMode) :
    vrpn_Shared_int32 (name, defaultValue, defaultMode) {

  d_isSerializer = vrpn_TRUE;

}

// virtual
vrpn_Shared_int32_Server::~vrpn_Shared_int32_Server (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromRemote_type, handle_update, this, d_myId);
  }

}

vrpn_Shared_int32_Server & vrpn_Shared_int32_Server::operator =
  (vrpn_int32 c) {
  vrpn_Shared_int32::operator = (c);
  return *this;
}

// virtual
void vrpn_Shared_int32_Server::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_int32::bindConnection(c);

  d_myUpdate_type = d_updateFromServer_type;

  if (d_connection) {
    d_connection->register_handler(d_updateFromRemote_type,
                                   handle_update, this, d_myId);
  }
}


vrpn_Shared_int32_Remote::vrpn_Shared_int32_Remote (const char * name,
                                                    vrpn_int32 defaultValue,
                                                    vrpn_int32 defaultMode) :
    vrpn_Shared_int32 (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_int32_Remote::~vrpn_Shared_int32_Remote (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromServer_type, handle_update, this, d_myId);
  }

}


vrpn_Shared_int32_Remote & vrpn_Shared_int32_Remote::operator =
  (vrpn_int32 c) {
  vrpn_Shared_int32::operator = (c);
  return *this;
}

void vrpn_Shared_int32_Remote::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_int32::bindConnection(c);

  d_myUpdate_type = d_updateFromRemote_type;

  if (d_connection) {
    d_connection->register_handler(d_updateFromServer_type,
                                   handle_update, this, d_myId);
  }
}





vrpn_Shared_float64::vrpn_Shared_float64 (const char * name,
                                      vrpn_float64 defaultValue,
                                      vrpn_int32 mode) :
  d_value (defaultValue),
  d_name (name ? new char [1 + strlen(name)] : NULL),
  d_mode (mode),
  d_connection (NULL),
  d_myId (-1),
  d_updateFromServer_type (-1),
  d_updateFromRemote_type (-1),
  d_becomeSerializer_type (-1),
  d_callbacks (NULL),
  d_timedCallbacks (NULL),
  d_policy (ACCEPT),
  d_policyCallback (NULL),
  d_policyUserdata (NULL),
  d_isSerializer (vrpn_FALSE) {

  if (name) {
    strcpy(d_name, name);
  }
  gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_Shared_float64::~vrpn_Shared_float64 (void) {
  if (d_name) {
    delete [] d_name;
  }
  if (d_connection) {
    d_connection->unregister_handler(d_becomeSerializer_type,
                                     handle_becomeSerializer, this, d_myId);
  }
}

vrpn_float64 vrpn_Shared_float64::value (void) const {
  return d_value;
}

vrpn_Shared_float64::operator vrpn_float64 () const {
  return value();
}

vrpn_Shared_float64 & vrpn_Shared_float64::operator =
                                            (vrpn_float64 newValue) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return set(newValue, now);
}

vrpn_Shared_float64 & vrpn_Shared_float64::set (vrpn_float64 newValue,
                                                timeval when) {
  return set(newValue, when, vrpn_TRUE);
}

void vrpn_Shared_float64::bindConnection (vrpn_Connection * c) {
  char buffer [101];
  if (c == NULL) {
      // unbind the connection
      d_connection = NULL;
      return;
  }

  if (c && d_connection) {
    fprintf(stderr, "vrpn_Shared_float64::bindConnection:  "
                    "Tried to rebind a connection to %s.\n", d_name);
    return;
  }

  d_connection = c;
  if (!c) {
    return;
  }

  sprintf(buffer, "vrpn Shared float64 %s", d_name);
  d_myId = c->register_sender(buffer);
  d_updateFromServer_type = c->register_message_type
          ("vrpn Shared update from server");
  d_updateFromRemote_type = c->register_message_type
          ("vrpn Shared update from remote");
  d_becomeSerializer_type = c->register_message_type
          ("vrpn Shared become serializer");

//printf ("My name is %s;  myId %d, ufs type %d, ufr type %d.\n",
//buffer, d_myId, d_updateFromServer_type, d_updateFromRemote_type);

  d_connection->register_handler(d_becomeSerializer_type,
                                 handle_becomeSerializer,
                                 this, d_myId);

}

void vrpn_Shared_float64::register_handler (vrpnSharedFloatCallback cb,
                                            void * userdata) {
  callbackEntry * e = new callbackEntry;
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

void vrpn_Shared_float64::unregister_handler (vrpnSharedFloatCallback cb,
                                              void * userdata) {
  callbackEntry * e, ** snitch;

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
  delete e;
}

void vrpn_Shared_float64::register_handler (vrpnTimedSharedFloatCallback cb,
                                            void * userdata) {
  timedCallbackEntry * e = new timedCallbackEntry;
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

void vrpn_Shared_float64::unregister_handler (vrpnTimedSharedFloatCallback cb,
                                              void * userdata) {
  timedCallbackEntry * e, ** snitch;

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
  delete e;
}

void vrpn_Shared_float64::setSerializerPolicy
              (vrpn_SerializerPolicy policy,
               vrpnSharedFloatSerializerPolicy f,
               void * userdata) {
  d_policy = policy;
  d_policyCallback = f;
  d_policyUserdata = userdata;
}

vrpn_Shared_float64 & vrpn_Shared_float64::set (vrpn_float64 newValue,
                                                timeval when,
                                                vrpn_bool isLocalSet) {
  vrpn_bool acceptedUpdate;

  acceptedUpdate = shouldAcceptUpdate(newValue, when, isLocalSet);
  if (acceptedUpdate) {
    d_value = newValue;
    d_lastUpdate = when;
    yankCallbacks();
  }

  if (shouldSendUpdate(isLocalSet, acceptedUpdate)) {
    sendUpdate(d_myUpdate_type, newValue, when);
  }

  return *this;
}

//virtual
vrpn_bool vrpn_Shared_float64::shouldAcceptUpdate
                     (vrpn_float64 newValue, timeval when,
                      vrpn_bool isLocalSet) {

  // Is this "new" change idempotent?
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
    return vrpn_FALSE;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
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
      return vrpn_FALSE;
    } else {
      return vrpn_TRUE;
    }
  }

  // We are the serializer.

  // Are we accepting all updates?
  if (d_policy == ACCEPT) {
    return vrpn_TRUE;
  }

  // Does the user want to accept this one?
  if ((d_policy == CALLBACK) && d_policyCallback &&
      (*d_policyCallback)(d_policyUserdata, newValue, when, this)) {
    return vrpn_TRUE;
  }

  return vrpn_FALSE;
}

// virtual
vrpn_bool vrpn_Shared_float64::shouldSendUpdate
                     (vrpn_bool isLocalSet, vrpn_bool acceptedUpdate) {
  if (acceptedUpdate && isLocalSet) {
    return vrpn_TRUE;
  }

  if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
    return vrpn_FALSE;
  }

  if (!d_isSerializer && isLocalSet) {
    return vrpn_TRUE;
  }

  if (d_isSerializer && !isLocalSet && acceptedUpdate) {
    return vrpn_TRUE;
  }

  return vrpn_FALSE;
}



void vrpn_Shared_float64::encode (char ** buffer, vrpn_int32 * len,
                                  vrpn_float64 newValue, timeval when) const {
  vrpn_buffer(buffer, len, newValue);
  vrpn_buffer(buffer, len, when);
}
void vrpn_Shared_float64::decode (const char ** buffer, vrpn_int32 *,
                                vrpn_float64 * newValue, timeval * when) const {
  vrpn_unbuffer(buffer, newValue);
  vrpn_unbuffer(buffer, when);
}

void vrpn_Shared_float64::sendUpdate (vrpn_int32 msgType,
                                      vrpn_float64 newValue, timeval when) {
  char buffer [32];
  vrpn_int32 buflen = 32;
  char * bp = buffer;

  if (d_connection) {
    encode(&bp, &buflen, newValue, when);
    d_connection->pack_message(32 - buflen, d_lastUpdate,
                               msgType, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_float64::sendUpdate:  packed message\n");
  }

}


int vrpn_Shared_float64::yankCallbacks (void) {
  callbackEntry * e;
  timedCallbackEntry * te;

  for (e = d_callbacks;  e;  e = e->next) {
    if ((*e->handler)(e->userdata, d_value)) {
      return -1;
    }
  }
  for (te = d_timedCallbacks;  te;  te = te->next) {
    if ((*te->handler)(te->userdata, d_value, d_lastUpdate)) {
      return -1;
    }
  }

  return 0;
}

// static
int vrpn_Shared_float64::handle_becomeSerializer (void * userdata,
                                                vrpn_HANDLERPARAM p) {
  vrpn_Shared_float64 * s = (vrpn_Shared_float64 *) userdata;

  s->d_isSerializer = vrpn_TRUE;

  return 0;
}

// static
int vrpn_Shared_float64::handle_update (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Shared_float64 * s = (vrpn_Shared_float64 *) userdata;
  vrpn_float64 newValue;
  timeval when;

  s->decode(&p.buffer, &p.payload_len, &newValue, &when);

  s->set(newValue, when, vrpn_FALSE);

  return 0;
}






vrpn_Shared_float64_Server::vrpn_Shared_float64_Server
                                     (const char * name,
                                      vrpn_float64 defaultValue,
                                      vrpn_int32 defaultMode) :
    vrpn_Shared_float64 (name, defaultValue, defaultMode) {

  d_isSerializer = vrpn_TRUE;

}

// virtual
vrpn_Shared_float64_Server::~vrpn_Shared_float64_Server (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromRemote_type, handle_update, this, d_myId);
  }

}

vrpn_Shared_float64_Server & vrpn_Shared_float64_Server::operator =
  (vrpn_float64 c) {
  vrpn_Shared_float64::operator = (c);
  return *this;
}



// virtual
void vrpn_Shared_float64_Server::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_float64::bindConnection(c);

  d_myUpdate_type = d_updateFromServer_type;

  if (d_connection) {
    d_connection->register_handler(d_updateFromRemote_type,
                                   handle_update,
                                   this, d_myId);
  }
}


vrpn_Shared_float64_Remote::vrpn_Shared_float64_Remote
                                  (const char * name,
                                   vrpn_float64 defaultValue,
                                   vrpn_int32 defaultMode) :
    vrpn_Shared_float64 (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_float64_Remote::~vrpn_Shared_float64_Remote (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromServer_type, handle_update, this, d_myId);
  }

}

vrpn_Shared_float64_Remote & vrpn_Shared_float64_Remote::operator =
  (vrpn_float64 c) {
  vrpn_Shared_float64::operator = (c);
  return *this;
}

// virtual
void vrpn_Shared_float64_Remote::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_float64::bindConnection(c);

  d_myUpdate_type = d_updateFromRemote_type;

  if (d_connection) {
    d_connection->register_handler(d_updateFromServer_type,
                                   handle_update,
                                   this, d_myId);
  }
}





vrpn_Shared_String::vrpn_Shared_String (const char * name,
                                        const char * defaultValue,
                                        vrpn_int32 mode) :
  d_value (defaultValue ? new char [1 + strlen(defaultValue)] : NULL),
  d_name (name ? new char [1 + strlen(name)] : NULL),
  d_mode (mode),
  d_connection (NULL),
  d_myId (-1),
  d_updateFromServer_type (-1),
  d_updateFromRemote_type (-1),
  d_becomeSerializer_type (-1),
  d_callbacks (NULL),
  d_timedCallbacks (NULL),
  d_policy (ACCEPT),
  d_policyCallback (NULL),
  d_policyUserdata (NULL),
  d_isSerializer (vrpn_FALSE) {

  if (defaultValue) {
    strcpy(d_value, defaultValue);
  }
  if (name) {
    strcpy(d_name, name);
  }
  gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_Shared_String::~vrpn_Shared_String (void) {
  if (d_value) {
    delete [] d_value;
  }
  if (d_name) {
    delete [] d_name;
  }
  if (d_connection) {
    d_connection->unregister_handler(d_becomeSerializer_type,
                                     handle_becomeSerializer, this, d_myId);
  }
}

const char * vrpn_Shared_String::value (void) const {
  return d_value;
}

vrpn_Shared_String::operator const char * () const {
  return value();
}

vrpn_Shared_String & vrpn_Shared_String::operator =
                                            (const char * newValue) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return set(newValue, now);
}

vrpn_Shared_String & vrpn_Shared_String::set (const char * newValue,
                                              timeval when) {
  return set(newValue, when, vrpn_TRUE);
}


void vrpn_Shared_String::bindConnection (vrpn_Connection * c) {
  char buffer [101];
  if (c == NULL) {
      // unbind the connection
      d_connection = NULL;
      return;
  }

  if (c && d_connection) {
    fprintf(stderr, "vrpn_Shared_String::bindConnection:  "
                    "Tried to rebind a connection to %s.\n", d_name);
    return;
  }

  d_connection = c;
  if (!c) {
    return;
  }

  sprintf(buffer, "vrpn Shared String %s", d_name);
  d_myId = c->register_sender(buffer);
  d_updateFromServer_type = c->register_message_type
          ("vrpn Shared update from server");
  d_updateFromRemote_type = c->register_message_type
          ("vrpn Shared update from remote");
  d_becomeSerializer_type = c->register_message_type
          ("vrpn Shared become serializer");

//printf ("My name is %s;  myId %d, ufs type %d, ufr type %d.\n",
//buffer, d_myId, d_updateFromServer_type, d_updateFromRemote_type);

  d_connection->register_handler(d_becomeSerializer_type,
                                 handle_becomeSerializer,
                                 this, d_myId);

}

void vrpn_Shared_String::register_handler (vrpnSharedStringCallback cb,
                                            void * userdata) {
  callbackEntry * e = new callbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_String::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_callbacks;
  d_callbacks = e;
}

void vrpn_Shared_String::unregister_handler (vrpnSharedStringCallback cb,
                                              void * userdata) {
  callbackEntry * e, ** snitch;

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
  delete e;
}

void vrpn_Shared_String::register_handler (vrpnTimedSharedStringCallback cb,
                                            void * userdata) {
  timedCallbackEntry * e = new timedCallbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_String::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_timedCallbacks;
  d_timedCallbacks = e;
}

void vrpn_Shared_String::unregister_handler (vrpnTimedSharedStringCallback cb,
                                              void * userdata) {
  timedCallbackEntry * e, ** snitch;

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
  delete e;
}

void vrpn_Shared_String::setSerializerPolicy
              (vrpn_SerializerPolicy policy,
               vrpnSharedStringSerializerPolicy f,
               void * userdata) {
  d_policy = policy;
  d_policyCallback = f;
  d_policyUserdata = userdata;
}

vrpn_Shared_String & vrpn_Shared_String::set (const char * newValue,
                                              timeval when,
                                              vrpn_bool isLocalSet) {
  vrpn_bool acceptedUpdate;

  acceptedUpdate = shouldAcceptUpdate(newValue, when, isLocalSet);
  if (acceptedUpdate) {
    if (d_value) {
      delete [] d_value;
    }
    d_value = new char [1 + strlen(newValue)];
    if (!d_value) {
      fprintf(stderr, "vrpn_Shared_String::set:  Out of memory.\n");
      return *this;
    }
    strcpy(d_value, newValue);

    d_lastUpdate = when;
    yankCallbacks();
  }

  if (shouldSendUpdate(isLocalSet, acceptedUpdate)) {
    sendUpdate(d_myUpdate_type, newValue, when);
  }

  return *this;
}

//virtual
vrpn_bool vrpn_Shared_String::shouldAcceptUpdate
                     (const char * newValue, timeval when,
                      vrpn_bool isLocalSet) {

  // Is this "new" change idempotent?
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
    return vrpn_FALSE;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
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
      return vrpn_FALSE;
    } else {
      return vrpn_TRUE;
    }
  }

  // We are the serializer.

  // Are we accepting all updates?
  if (d_policy == ACCEPT) {
    return vrpn_TRUE;
  }

  // Does the user want to accept this one?
  if ((d_policy == CALLBACK) && d_policyCallback &&
      (*d_policyCallback)(d_policyUserdata, newValue, when, this)) {
    return vrpn_TRUE;
  }

  return vrpn_FALSE;
}

// virtual
vrpn_bool vrpn_Shared_String::shouldSendUpdate
                     (vrpn_bool isLocalSet, vrpn_bool acceptedUpdate) {
  if (acceptedUpdate && isLocalSet) {
    return vrpn_TRUE;
  }

  if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
    return vrpn_FALSE;
  }

  if (!d_isSerializer && isLocalSet) {
    return vrpn_TRUE;
  }

  if (d_isSerializer && !isLocalSet && acceptedUpdate) {
    return vrpn_TRUE;
  }

  return vrpn_FALSE;
}



void vrpn_Shared_String::encode (char ** buffer, vrpn_int32 * len,
                                 const char * newValue, timeval when) const {
  // We reverse ordering from the other vrpn_SharedObject classes
  // so that the time value is guaranteed to be aligned.
  vrpn_buffer(buffer, len, when);
  vrpn_buffer(buffer, len, newValue, strlen(newValue));
}
void vrpn_Shared_String::decode (const char ** buffer, vrpn_int32 * len,
                                 char * newValue, timeval * when) const {
  vrpn_unbuffer(buffer, when);
  vrpn_unbuffer(buffer, newValue, *len - sizeof(*when));
  newValue[*len - sizeof(*when)] = 0;
}

void vrpn_Shared_String::sendUpdate (vrpn_int32 msgType,
                                     const char * newValue, timeval when) {
  char buffer [1024];  // HACK
  vrpn_int32 buflen = 1024;
  char * bp = buffer;

  if (d_connection) {
    encode(&bp, &buflen, newValue, when);
    d_connection->pack_message(1024 - buflen, d_lastUpdate,
                               msgType, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_String::sendUpdate:  packed message\n");
  }

}


int vrpn_Shared_String::yankCallbacks (void) {
  callbackEntry * e;
  timedCallbackEntry * te;

  for (e = d_callbacks;  e;  e = e->next) {
    if ((*e->handler)(e->userdata, d_value)) {
      return -1;
    }
  }
  for (te = d_timedCallbacks;  te;  te = te->next) {
    if ((*te->handler)(te->userdata, d_value, d_lastUpdate)) {
      return -1;
    }
  }

  return 0;
}

// static
int vrpn_Shared_String::handle_becomeSerializer (void * userdata,
                                                vrpn_HANDLERPARAM p) {
  vrpn_Shared_String * s = (vrpn_Shared_String *) userdata;

  s->d_isSerializer = vrpn_TRUE;

  return 0;
}

// static
int vrpn_Shared_String::handle_update (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_Shared_String * s = (vrpn_Shared_String *) ud;
  char newValue [1024];  // HACK
  timeval when;

  s->decode(&p.buffer, &p.payload_len, newValue, &when);

  s->set(newValue, when, vrpn_FALSE);

  return 0;
}








vrpn_Shared_String_Server::vrpn_Shared_String_Server
                                     (const char * name,
                                      const char * defaultValue,
                                      vrpn_int32 defaultMode) :
    vrpn_Shared_String (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_String_Server::~vrpn_Shared_String_Server (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromRemote_type, handle_update, this, d_myId);
  }

}

vrpn_Shared_String_Server & vrpn_Shared_String_Server::operator =
  (const char * c) {
  vrpn_Shared_String::operator = (c);
  return *this;
}


// virtual
void vrpn_Shared_String_Server::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_String::bindConnection(c);

  d_myUpdate_type = d_updateFromServer_type;

  if (d_connection) {
    d_connection->register_handler(d_updateFromRemote_type,
                                   handle_update,
                                   this, d_myId);
  }
}


vrpn_Shared_String_Remote::vrpn_Shared_String_Remote
                                  (const char * name,
                                   const char * defaultValue,
                                   vrpn_int32 defaultMode) :
    vrpn_Shared_String (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_String_Remote::~vrpn_Shared_String_Remote (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromServer_type, handle_update, this, d_myId);
  }

}

vrpn_Shared_String_Remote & vrpn_Shared_String_Remote::operator =
  (const char * c) {
  vrpn_Shared_String::operator = (c);
  return *this;
}

// virtual
void vrpn_Shared_String_Remote::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_String::bindConnection(c);

  d_myUpdate_type = d_updateFromRemote_type;

  if (d_connection) {
    d_connection->register_handler(d_updateFromServer_type,
                                   handle_update,
                                   this, d_myId);
  }
}

